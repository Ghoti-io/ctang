/*
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Copyright (C) 2024-2026 Corey Pennycuff
 *
 * This file is part of Ghoti.io Tang.
 *
 * Ghoti.io Tang is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Ghoti.io Tang is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <ghoti.io/cutil/array.h>
#include <ghoti.io/cutil/memory.h>
#include <ghoti.io/cutil/safemath.h>
#include <ghoti.io/tang/allocator.h>
#include <ghoti.io/unicode/break.h>
#include <ghoti.io/unicode/utf.h>
#include <ghoti.io/tang/macros.h>
#include <ghoti.io/tang/unicodeString.h>

/**
 * This pair will be punted into a uint64_t, for use in the string type vector.
 */
struct {
  uint32_t type;
  uint32_t grapheme_offset;
} GCU_Type_Offset_Pair;


bool gcu_unicode_string_get_grapheme_offsets(GCU_Vector32 * grapheme_offsets, const char * buffer, size_t length) {
  assert(grapheme_offsets);
  assert(buffer);

  // An empty string is one grapheme boundary at offset 0. The break iterator
  // reports no boundary on empty text, because there is nothing to fall between.
  if (!length) {
    return gcu_vector32_append(grapheme_offsets, GCU_TYPE32_UI32(0));
  }

  // Ill-formed UTF-8 used to fail the conversion into the break iterator's
  // encoding, and string creation failed with it. Refuse here for the same
  // reason: a boundary inside a bad sequence is not a grapheme.
  if (guni_utf8_validate(buffer, length, NULL) != GUNI_OK) {
    return false;
  }

  // The offsets are stored as uint32. A boundary past that cannot be recorded.
  if (length > UINT32_MAX) {
    return false;
  }

  // Worst case: every byte is its own grapheme, plus the sentinel at the end.
  // The vector may already hold entries, so the reserve covers those too.
  size_t cap = 0;
  size_t needed = 0;
  if (!gcu_safe_add_size(length, 1, &cap)
      || !gcu_safe_add_size(grapheme_offsets->count, cap, &needed)
      || !gcu_vector32_reserve(grapheme_offsets, needed)) {
    return false;
  }

  // guni_break_all() writes size_t. The vector's slots are uint32, so the
  // table is filled into a temporary buffer and then copied across.
  size_t bytes = 0;
  if (!gcu_safe_mul_size(cap, sizeof(size_t), &bytes)) {
    return false;
  }
  size_t * bounds = gcu_malloc(bytes);
  if (bounds == NULL) {
    return false;
  }
  size_t count = 0;
  GUNI_Result result = guni_break_all(NULL, buffer, length, bounds, cap, &count);
  if (result != GUNI_OK || count < 2 || bounds[count - 1] != length) {
    gcu_free(bounds);
    return false;
  }

  size_t base = grapheme_offsets->count;
  for (size_t i = 0; i < count; i++) {
    grapheme_offsets->data[base + i] = GCU_TYPE32_UI32((uint32_t)bounds[i]);
  }
  grapheme_offsets->count = base + count;
  gcu_free(bounds);
  return true;
}


GTA_Unicode_String * gta_unicode_string_create(const char * source, size_t length, GTA_String_Type type) {
  assert(source);

  char * buffer = gcu_malloc(length + 1);
  if (!buffer) {
    return NULL;
  }
  if (length) {
    memcpy(buffer, source, length);
  }
  buffer[length] = '\0';
  GTA_Unicode_String * string = gta_unicode_string_create_and_adopt(buffer, length, type);
  if (!string) {
    gcu_free(buffer);
  }
  return string;
}


GTA_Unicode_String * gta_unicode_string_create_and_adopt(const char * source, size_t length, GTA_String_Type type) {
  assert(source);

  // Allocate space for the string.
  GTA_Unicode_String * string = gcu_calloc(sizeof(GTA_Unicode_String), 1);
  if (string == NULL) {
    return NULL;
  }

  // Adopt the buffer.
  string->buffer = source;
  string->byte_length = length;

  // Create the grapheme offsets.
  // For ease of use, we will add an extra offset at the end of the string
  // that points to the end of the string.
  // If the string only contains ASCII characters, then the grapheme offsets
  // will be the same as the byte offsets and will require (length + 1)
  // entries as a worst case.  If the string contains non-ASCII characters,
  // then the grapheme offsets will be different from the byte offsets and
  // will require fewer than (length + 1) entries, so we will allocate the
  // worst-case number of entries to ensure that no allocation failures can
  // happen later.
  string->grapheme_offsets = gcu_vector32_create(length + 1);
  if (string->grapheme_offsets == NULL) {
    gcu_free(string);
    return NULL;
  }
  if (!gcu_unicode_string_get_grapheme_offsets(string->grapheme_offsets, source, length)) {
    gcu_vector32_destroy(string->grapheme_offsets);
    gcu_free(string);
    return NULL;
  }
  string->grapheme_length = gcu_vector32_count(string->grapheme_offsets) - 1;

  // Create the string type vector.
  // It will only contain one type, so we will allocate one entry.
  string->string_type = gcu_vector64_create(1);
  if (string->string_type == NULL) {
    gcu_vector32_destroy(string->grapheme_offsets);
    gcu_free(string);
    return NULL;
  }
  gcu_vector64_append(string->string_type, GTA_UC_MAKE_TYPE_OFFSET_PAIR(type, 0));

  return string;
}


void gta_unicode_string_destroy(GTA_Unicode_String * string) {
  assert(string);

  gcu_vector32_destroy(string->grapheme_offsets);
  gcu_vector64_destroy(string->string_type);
  gcu_free((void *)string->buffer);
  gcu_free(string);
}


GTA_Unicode_String * gta_unicode_string_concat(const GTA_Unicode_String * string1, const GTA_Unicode_String * string2) {
  assert(string1);
  assert(string2);

  // Allocate space for the new string.
  GTA_Unicode_String * newString = gcu_calloc(sizeof(GTA_Unicode_String), 1);
  if (newString == NULL) {
    return NULL;
  }

  // Copy the source strings to the buffer.
  newString->buffer = gcu_malloc(string1->byte_length + string2->byte_length + 1);
  if (newString->buffer == NULL) {
    gcu_free(newString);
    return NULL;
  }
  if (string1->byte_length) {
    memcpy((char *)newString->buffer, string1->buffer, string1->byte_length);
  }
  if (string2->byte_length) {
    memcpy((char *)newString->buffer + string1->byte_length, string2->buffer, string2->byte_length);
  }
  newString->byte_length = string1->byte_length + string2->byte_length;
  ((char *)newString->buffer)[newString->byte_length] = '\0';

  // Create the grapheme offsets.
  // For ease of use, we will add an extra offset at the end of the string
  // that points to the end of the string.
  // If the string only contains ASCII characters, then the grapheme offsets
  // will be the same as the byte offsets and will require (length + 1)
  // entries as a worst case.  If the string contains non-ASCII characters,
  // then the grapheme offsets will be different from the byte offsets and
  // will require fewer than (length + 1) entries, so we will allocate the
  // worst-case number of entries to ensure that no allocation failures can
  // happen later.
  newString->grapheme_offsets = gcu_vector32_create(string1->grapheme_length + string2->grapheme_length + 1);
  if (newString->grapheme_offsets == NULL) {
    gcu_free((void *)newString->buffer);
    gcu_free(newString);
    return NULL;
  }
  // Copy the grapheme offsets from the first string.
  // The space is already reserved.
  memcpy(newString->grapheme_offsets->data, string1->grapheme_offsets->data, string1->grapheme_length * sizeof(GCU_Type32_Union));
  // Copy the grapheme offsets from the second string, adding the byte and
  // grapheme offsets from the first string to the second string.
  // The space is already reserved.
  size_t byte_offset = string1->byte_length;
  size_t grapheme_offset = string1->grapheme_length;
  for (size_t i = 0; i < string2->grapheme_length; ++i) {
    newString->grapheme_offsets->data[i + grapheme_offset] = GCU_TYPE32_UI32(string2->grapheme_offsets->data[i].ui32 + byte_offset);
  }
  // Add the final offset.
  newString->grapheme_offsets->data[string1->grapheme_length + string2->grapheme_length] = GCU_TYPE32_UI32(string1->byte_length + string2->byte_length);
  // Since we reached into the vector and manipulated data directly, we need to
  // manually fix the count.
  newString->grapheme_offsets->count = string1->grapheme_length + string2->grapheme_length + 1;
  // Set the grapheme length.
  newString->grapheme_length = string1->grapheme_length + string2->grapheme_length;

  // Create the string type vector.
  // Because the last type of the first string and the first type of the second
  // string may be the same, we may need to merge them.  We will allocate
  // enough space for the worst case, which is that the last type of the first
  // string and the first type of the second string are different.
  newString->string_type = gcu_vector64_create(string1->string_type->count + string2->string_type->count);
  if (newString->string_type == NULL) {
    gcu_vector32_destroy(newString->grapheme_offsets);
    gcu_free((void *)newString->buffer);
    gcu_free(newString);
    return NULL;
  }
  // If the first string is empty, then skip it.
  size_t types_copied_count = 0;
  bool strings_are_same_type = false;
  if (string1->byte_length != 0) {
    // Copy the string types from the first string.
    // The space is already reserved.
    memcpy(newString->string_type->data, string1->string_type->data, string1->string_type->count * sizeof(GCU_Type64_Union));
    strings_are_same_type = GTA_UC_GET_TYPE_FROM_TYPE_OFFSET_PAIR(string1->string_type->data[string1->string_type->count - 1]) == GTA_UC_GET_TYPE_FROM_TYPE_OFFSET_PAIR(string2->string_type->data[0]);
    // Since we reached into the vector and manipulated data directly, we need to
    // manually fix the count.
    newString->string_type->count = string1->string_type->count;
    types_copied_count = string1->string_type->count;
  }
  // Copy the string types from the second string, adding the grapheme count
  // from the first string to the second string's grapheme offsets.
  // The space is already reserved.
  if (string2->byte_length || !string1->byte_length) {
    size_t offset = strings_are_same_type ? 1 : 0;
    for (size_t i = offset; i < string2->string_type->count; ++i) {
      // Was originally:
      //   uint64_t currentUnion = string2->string_type->data[i].ui64;
      //   GTA_UC_MAKE_TYPE_OFFSET_PAIR(GTA_UC_GET_TYPE_FROM_TYPE_OFFSET_PAIR(currentUnion), GTA_UC_GET_OFFSET_FROM_TYPE_OFFSET_PAIR(currentUnion) + string1->grapheme_length)
      // but, since the union is a integer and the addition is happening to the
      // lower bytes, that simplifies to:
      //   string2->string_type->data[i].ui64 + string1->grapheme_length
      newString->string_type->data[types_copied_count + i - offset].ui64 = string2->string_type->data[i].ui64 + string1->grapheme_length;
    }
    // Since we reached into the vector and manipulated data directly, we need to
    // manually fix the count.
    newString->string_type->count = types_copied_count + string2->string_type->count - (strings_are_same_type ? 1 : 0);
  }

  return newString;
}


GTA_Unicode_String * gta_unicode_string_substring(const GTA_Unicode_String * string, size_t grapheme_start, size_t grapheme_count) {
  assert(string);

  // If grapheme start is beyond the end of the string, then return an empty
  // string.
  if ((grapheme_start >= string->grapheme_length) || (grapheme_count == 0)) {
    return gta_unicode_string_create("", 0, GTA_UNICODE_STRING_TYPE_TRUSTED);
  }

  // Determine the byte offset of the start of the substring.
  size_t byte_start = string->grapheme_offsets->data[grapheme_start].ui32;

  // Determine the byte offset of the end of the substring.
  size_t byte_end = (grapheme_start + grapheme_count >= string->grapheme_length)
    ? string->grapheme_offsets->data[string->grapheme_length].ui32
    : string->grapheme_offsets->data[grapheme_start + grapheme_count].ui32;

  // Determine the actual last grapheme to be copied from the original string.
  size_t end_grapheme = (grapheme_start + grapheme_count >= string->grapheme_length)
    ? string->grapheme_length
    : grapheme_start + grapheme_count;

  // Determine the type of the substring by finding the last segment that
  // begins at or before the start grapheme - that is the segment the start
  // grapheme falls inside.
  //
  // The comparison used to be `>=`, which is the test for a segment that
  // begins at or after the start, and every segment satisfies it when the
  // start is 0.  The loop therefore ran to the end of the list and left the
  // *last* segment's type as the type of the whole substring.  A whole-string
  // substring is what print() takes, so `!"<i>" + "<b>"` rendered as
  // `<i><b>`: the untrusted half inherited the trusted tag of the half after
  // it and reached the page unescaped.  It was only unreachable from the
  // language because nothing but the (uncalled) simplifier built a
  // multi-segment string.
  GTA_String_Type newStringType = GTA_UNICODE_STRING_TYPE_TRUSTED;
  size_t first_string_type_index_to_include = 0;
  size_t iterator = 0;
  while ((iterator < string->string_type->count) && (GTA_UC_GET_OFFSET_FROM_TYPE_OFFSET_PAIR(string->string_type->data[iterator]) <= grapheme_start)) {
    newStringType = GTA_UC_GET_TYPE_FROM_TYPE_OFFSET_PAIR(string->string_type->data[iterator]);
    first_string_type_index_to_include = iterator;
    ++iterator;
  }
  // Create a new string.
  GTA_Unicode_String * newString = gta_unicode_string_create(string->buffer + byte_start, byte_end - byte_start, newStringType);
  if (newString == NULL) {
    return NULL;
  }

  // Determine the last string type offset that is less than the last grapheme
  // to be copied from the original string.  `end_grapheme` is one past the
  // last grapheme kept, so a segment beginning exactly there belongs to the
  // text after the substring and is not included.
  size_t last_string_type_index_to_include = first_string_type_index_to_include;
  iterator = first_string_type_index_to_include;
  while ((iterator < string->string_type->count) && (GTA_UC_GET_OFFSET_FROM_TYPE_OFFSET_PAIR(string->string_type->data[iterator]) < end_grapheme)) {
    last_string_type_index_to_include = iterator;
    ++iterator;
  }

  // Add the remaining string types.
  // The first entry is already present.
  // We will allocate enough space from the start, because we know how many
  // entries are required.
  if (!gcu_vector64_reserve(newString->string_type, last_string_type_index_to_include - first_string_type_index_to_include + 1)) {
    gta_unicode_string_destroy(newString);
    return NULL;
  }
  for (size_t i = first_string_type_index_to_include + 1; i <= last_string_type_index_to_include; ++i) {
    // The original string_type already contains the correct type, but the
    // offset is relative to the start of the original string, so we need to
    // subtract the start of the substring.
    gcu_vector64_append(newString->string_type, GCU_TYPE64_I64(string->string_type->data[i].ui64 - grapheme_start));
  }
  return newString;
}


/*
 * Claim room for one rendered segment and return where to write it.
 *
 * `bytes_needed` is what this segment encodes to; `bytes_after` is what is
 * left of the source once it ends.  Both are reserved, so that a string
 * whose remainder is TRUSTED - which is the common case - is sized once and
 * never grown, exactly as the hand written resize this replaces did.
 *
 * The caller writes `bytes_needed` bytes into the returned pointer and must
 * not hold it across another call: the next claim may move the storage.
 */
static char * gta_unicode_render_claim(GCU_Array * out, size_t bytes_needed, size_t bytes_after) {
  assert(out);

  size_t optimistic;
  if (!gcu_safe_add3_size(gcu_array_count(out), bytes_needed, bytes_after, &optimistic)
    || !gcu_array_reserve(out, optimistic)) {
    return NULL;
  }
  return gcu_array_extend_n(out, bytes_needed);
}


GTA_Unicode_Rendered_String gta_unicode_string_render(const GTA_Unicode_String * string) {
  assert(string);

  // Safety check for an empty string.
  if (!string->buffer) {
    return (GTA_Unicode_Rendered_String){
      .buffer = NULL,
      .length = 0
    };
  }

  assert(string->string_type);
  assert(string->string_type->count);

  // The output is a growable array of bytes.  Each of the four encodings
  // below used to carry its own copy of the same resize - the identical
  // eight lines, written out four times - and each tracked the length and
  // the capacity in locals of its own.  That bookkeeping is the array's now,
  // and gcu_array_reserve() reports an overflowing total rather than
  // wrapping it.
  //
  // The optimistic sizing is kept: every segment reserves room for its own
  // encoded bytes plus the whole of the rest of the source, on the
  // assumption that the remainder is TRUSTED and will be copied straight
  // through.  A string of one type - which is almost all of them - is
  // therefore allocated once and never grown.
  GCU_Array * out = gcu_array_create(1, string->byte_length + 1, gta_allocator());
  if (!out) {
    goto RENDER_ERROR;
  }

  // Loop through the string types and render the string.
  for (size_t i = 0; i < string->string_type->count; ++i) {
    // Compute the offsets for the current and next graphemes.
    GTA_String_Type type = GTA_UC_GET_TYPE_FROM_TYPE_OFFSET_PAIR(string->string_type->data[i]);
    size_t source_grapheme_offset = GTA_UC_GET_OFFSET_FROM_TYPE_OFFSET_PAIR(string->string_type->data[i]);
    size_t next_source_grapheme_offset = (i + 1 < string->string_type->count)
      ? GTA_UC_GET_OFFSET_FROM_TYPE_OFFSET_PAIR(string->string_type->data[i + 1])
      : string->grapheme_length;
    size_t source_byte_offset = string->grapheme_offsets->data[source_grapheme_offset].ui32;
    size_t next_source_byte_offset = string->grapheme_offsets->data[next_source_grapheme_offset].ui32;

    // Render the string based on the type.
    switch (type) {
      case GTA_UNICODE_STRING_TYPE_TRUSTED: {
        // This is a direct copy.
        size_t bytes_to_copy = next_source_byte_offset - source_byte_offset;
        size_t bytes_remaining_in_source = string->byte_length - source_byte_offset + 1;
        if (!gcu_array_reserve(out, gcu_array_count(out) + bytes_remaining_in_source)
          || !gcu_array_append_n(out, string->buffer + source_byte_offset, bytes_to_copy)) {
          goto RENDER_ERROR;
        }
        break;
      }
      case GTA_UNICODE_STRING_TYPE_HTML:
      case GTA_UNICODE_STRING_TYPE_HTML_ATTRIBUTE: {
        // Encode the following characters: < > &
        // Attribute encoding also encodes: " '

        // First pass, determine the length of the buffer required.
        size_t bytes_needed = 0;
        for (size_t i = source_byte_offset; i < next_source_byte_offset; ++i) {
          switch (string->buffer[i]) {
            case '<':
              bytes_needed += 4; // &lt;
              break;
            case '>':
              bytes_needed += 4; // &gt;
              break;
            case '&':
              bytes_needed += 5; // &amp;
              break;
            case '"':
              bytes_needed += type == GTA_UNICODE_STRING_TYPE_HTML
                ? 1
                : 6; // &quot;
              break;
            case '\'':
              bytes_needed += type == GTA_UNICODE_STRING_TYPE_HTML
                ? 1
                : 5; // &#39;
              break;
            default:
              ++bytes_needed;
              break;
          }
        }

        // Claim the space for this segment in one call, then write into it
        // directly.  Appending a byte at a time would be a function call per
        // character on the hot path.
        char * dest = gta_unicode_render_claim(out, bytes_needed,
          string->byte_length - next_source_byte_offset + 1);
        if (!dest) {
          goto RENDER_ERROR;
        }

        // Second pass, encode the characters.
        size_t offset = 0;
        for (size_t i = source_byte_offset; i < next_source_byte_offset; ++i) {
          switch (string->buffer[i]) {
            case '<':
              memcpy(dest + offset, "&lt;", 4);
              offset += 4;
              break;
            case '>':
              memcpy(dest + offset, "&gt;", 4);
              offset += 4;
              break;
            case '&':
              memcpy(dest + offset, "&amp;", 5);
              offset += 5;
              break;
            case '"':
              if (type == GTA_UNICODE_STRING_TYPE_HTML) {
                dest[offset++] = '"';
              }
              else {
                memcpy(dest + offset, "&quot;", 6);
                offset += 6;
              }
              break;
            case '\'':
              if (type == GTA_UNICODE_STRING_TYPE_HTML) {
                dest[offset++] = '\'';
              }
              else {
                memcpy(dest + offset, "&#39;", 5);
                offset += 5;
              }
              break;
            default:
              dest[offset++] = string->buffer[i];
              break;
          }
        }
        assert(offset == bytes_needed);
        break;
      }
      case GTA_UNICODE_STRING_TYPE_PERCENT: {
        // Encodes everything except for the following characters: A-Z a-z 0-9 - _ . ~

        // First pass, determine the length of the buffer required.
        size_t bytes_needed = 0;
        for (size_t i = source_byte_offset; i < next_source_byte_offset; ++i) {
          // As an unsigned char throughout. char is signed here, and every
          // byte of a non-ASCII character is negative as one: isalnum() is
          // undefined for a negative argument other than EOF, and the hex
          // lookup below indexes off the front of its table.
          unsigned char byte = (unsigned char)string->buffer[i];
          if (isalnum(byte)
            || byte == '-'
            || byte == '_'
            || byte == '.'
            || byte == '~'
            || byte == ' ') {
            ++bytes_needed;
          }
          else {
            bytes_needed += 3; // %XX
          }
        }

        char * dest = gta_unicode_render_claim(out, bytes_needed,
          string->byte_length - next_source_byte_offset + 1);
        if (!dest) {
          goto RENDER_ERROR;
        }

        // Second pass, encode the characters.
        size_t offset = 0;
        for (size_t i = source_byte_offset; i < next_source_byte_offset; ++i) {
          unsigned char byte = (unsigned char)string->buffer[i];
          if (isalnum(byte)
            || byte == '-'
            || byte == '_'
            || byte == '.'
            || byte == '~') {
            dest[offset++] = (char)byte;
          }
          else if (byte == ' ') {
            dest[offset++] = '+';
          }
          else {
            dest[offset++] = '%';
            dest[offset++] = "0123456789ABCDEF"[byte >> 4];
            dest[offset++] = "0123456789ABCDEF"[byte & 0x0F];
          }
        }
        assert(offset == bytes_needed);
        break;
      }
      case GTA_UNICODE_STRING_TYPE_JAVASCRIPT: {
        // Encodes the following characters: ' " \ \n \r \t < > &

        // First pass, determine the length of the buffer required.
        size_t bytes_needed = 0;
        for (size_t i = source_byte_offset; i < next_source_byte_offset; ++i) {
          switch (string->buffer[i]) {
            case '\'':
            case '"':
            case '\\':
            case '\n':
            case '\r':
            case '\t':
              bytes_needed += 2; // Escape sequence is two characters.
              break;
            case '<':
            case '>':
            case '&':
              bytes_needed += 6; // Unicode escape sequence is six characters.
              break;
            default:
              ++bytes_needed;
              break;
          }
        }

        char * dest = gta_unicode_render_claim(out, bytes_needed,
          string->byte_length - next_source_byte_offset + 1);
        if (!dest) {
          goto RENDER_ERROR;
        }

        // Second pass, encode the characters.
        size_t offset = 0;
        for (size_t i = source_byte_offset; i < next_source_byte_offset; ++i) {
          switch (string->buffer[i]) {
            case '\'':
            case '"':
            case '\\':
              dest[offset++] = '\\';
              dest[offset++] = string->buffer[i];
              break;
            case '\n':
              dest[offset++] = '\\';
              dest[offset++] = 'n';
              break;
            case '\r':
              dest[offset++] = '\\';
              dest[offset++] = 'r';
              break;
            case '\t':
              dest[offset++] = '\\';
              dest[offset++] = 't';
              break;
            case '<':
              memcpy(dest + offset, "\\u003C", 6);
              offset += 6;
              break;
            case '>':
              memcpy(dest + offset, "\\u003E", 6);
              offset += 6;
              break;
            case '&':
              memcpy(dest + offset, "\\u0026", 6);
              offset += 6;
              break;
            default:
              dest[offset++] = string->buffer[i];
              break;
          }
        }
        assert(offset == bytes_needed);
        break;
      }
      default:
        assert(false);
        goto RENDER_ERROR;
    }
  }

  // The terminator is in the buffer but not in the length, which is the
  // contract the callers of this function already relied on.
  if (!gcu_array_append(out, "")) {
    goto RENDER_ERROR;
  }
  size_t count = 0;
  char * buffer = gcu_array_steal(out, &count);
  gcu_array_destroy(out);
  return (GTA_Unicode_Rendered_String){
    .buffer = buffer,
    .length = count - 1,
  };

RENDER_ERROR:
  gcu_array_destroy(out);
  return (GTA_Unicode_Rendered_String){
    .buffer = NULL,
    .length = 0,
  };
}


GTA_NO_DISCARD GTA_Unicode_Rendered_String gta_unicode_string_html_encode(const char * source, size_t length) {
  assert(length ? (bool)source : true);

  if (!length) {
    return (GTA_Unicode_Rendered_String){
      .buffer = NULL,
      .length = 0
    };
  }

  return (GTA_Unicode_Rendered_String){NULL, 0};
}
