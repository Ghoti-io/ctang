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

/**
 * @file
 *
 * Header file for the macros used throughout the project.
 *
 * Architecture macros are taken from:
 * https://sourceforge.net/p/predef/wiki/Architectures/
 * Endian macros are taken from:
 * https://sourceforge.net/p/predef/wiki/Endianness/
 */

#ifndef GHOTI_IO_GTA_MACROS_H
#define GHOTI_IO_GTA_MACROS_H

#include <ghoti.io/tang/namespace.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <ghoti.io/cutil/float.h>
#include <ghoti.io/cutil/vector.h>
#include <ghoti.io/cutil/hash.h>
#include <ghoti.io/cutil/string.h>
#include <ghoti.io/tang/libver.h>

//-----------------------------------------------------------------------------
// Visibility
//-----------------------------------------------------------------------------

/**
 * Marks a declaration as part of the public API.
 *
 * The library is built with -fvisibility=hidden, so a symbol without this is
 * not exported at all: it cannot collide with another version of this library
 * and it does not appear in the dynamic symbol table.  See CONVENTIONS.md
 * section 4.
 *
 * On Windows a consumer linking the static archive rather than the DLL must
 * define GHOTIIO_TANG_STATIC.  Without it every declaration is dllimport, the
 * compiler emits references to `__imp_` thunks, and the archive - which
 * defines the functions themselves, not import thunks - satisfies none of
 * them.  The command-line tool and the test suite link the archive, so the
 * Makefile defines it for them.
 */
#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef GHOTIIO_TANG_BUILD
#define GTA_API __declspec(dllexport)
#elif defined(GHOTIIO_TANG_STATIC)
#define GTA_API
#else
#define GTA_API __declspec(dllimport)
#endif
#else
#define GTA_API __attribute__((visibility("default")))
#endif

/**
 * Marks an exported *variable* as part of the public API.
 *
 * Same visibility as GTA_API.  Kept separate because a variable declaration
 * cannot carry a redundant linkage specification, so a library whose API macro
 * folds in `extern "C"` needs a second form for data.  See CONVENTIONS.md
 * section 4.
 */
#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef GHOTIIO_TANG_BUILD
#define GTA_API_DATA __declspec(dllexport)
#elif defined(GHOTIIO_TANG_STATIC)
#define GTA_API_DATA
#else
#define GTA_API_DATA __declspec(dllimport)
#endif
#else
#define GTA_API_DATA __attribute__((visibility("default")))
#endif

/**
 * Marks an internal declaration that the test suite needs to reach.
 *
 * Exported only when GHOTIIO_TANG_TEST_BUILD is defined, which the Makefile
 * does not do for the shipped library.  A production consumer therefore cannot
 * see or link against these.
 */
#ifdef GHOTIIO_TANG_TEST_BUILD
#define GTA_INTERNAL_API GTA_API
#else
#define GTA_INTERNAL_API
#endif

/**
 * Declaring the calling convention (for consistency).
 *
 * ARM and ARM64 use the AAPCS calling convention.  The ARM EABI is a
 * standard for ARM architecture processors that defines a common
 * application binary interface for simple, high-performance, and
 * efficient embedded systems with a focus on reducing memory usage.
 *
 * The x86 and x86-64 use the cdecl calling convention.  The cdecl
 * calling convention is the default calling convention for x86 and
 * x86-64 C and C++ programs.  It is used by many C and C++ compilers
 * for the x86 and x86-64 architectures.
 *
 * The Windows API uses the stdcall calling convention.  We want to use
 * the cdecl calling convention for the library, so we need to declare
 * the calling convention for the functions.
 */
#if defined(ARM) || defined(ARM64)
#define GTA_CALL __attribute__((pcs("aapcs")))
#define GTA_SHADOW_SIZE__X86_64 0

#elif defined(_WIN32) || defined(_WIN64)
#define GTA_CALL
#define GTA_SHADOW_SIZE__X86_64 32

#elif defined(__i386__) || defined(__x86_64__)
// x86 and x86-64 use the cdecl calling convention already, and some compilers
// will give an attribute error if we try to add it.
#define GTA_CALL
#define GTA_SHADOW_SIZE__X86_64 0

#else
#error "Unsupported architecture or compiler"

#endif


/**
 * A cross-compiler macro for declaring a function as no discard.
 *
 * GCC and clang get the attribute form in C++ as well as in C.  Every
 * declaration here spells it `GTA_API GTA_NO_DISCARD`, which puts it after a
 * GNU attribute and ahead of the return type; in C++ an attribute in that
 * position appertains to the *type*, and a type is not something
 * [[nodiscard]] can be applied to.  g++ lets it pass, clang++ refuses the
 * translation unit.  __attribute__((warn_unused_result)) carries no such
 * positional rule, and is what the library's own C compilation already uses,
 * so both languages now mean the same thing by the macro.
 */
#if defined(__GNUC__) || defined(__clang__)
#define GTA_NO_DISCARD __attribute__((warn_unused_result))
#elif defined(__cplusplus) && __cplusplus >= 201703L
#define GTA_NO_DISCARD [[nodiscard]]
#else
#error "Unsupported compiler"
#endif


/**
 * A cross-compiler macro for marking a function parameter as unused.
 */
#if defined(__GNUC__) || defined(__clang__)
#define GTA_MAYBE_UNUSED(X) __attribute__((unused)) X

#elif defined(_MSC_VER)
#define GTA_MAYBE_UNUSED(X) (void)(X)

#else
#define GTA_MAYBE_UNUSED(X) X

#endif


/**
 * A cross-compiler macro for identifying the system is big endian.
 */
#define GTA_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

/**
 * A cross-compiler macro for identifying the system is little endian.
 */
#define GTA_LITTLE_ENDIAN (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)

/**
 * A cross-compiler macro for identifying the system is 32-bit.
 */
#define GTA_32_BIT (__SIZEOF_POINTER__ == 4)

/**
 * A cross-compiler macro for identifying the system is 64-bit.
 */
#define GTA_64_BIT (__SIZEOF_POINTER__ == 8)

/**
 * A cross-compiler macro for identifying the system is x86.
 */
#if defined(__i386__) || defined(__i386) || defined(__i486__) || defined(__i486) || defined(__i586__) || defined(__i586) || defined(__i686__) || defined(__i686) || defined(_M_IX86) || defined(__X86__) || defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__) || defined(__INTEL__) || defined(__386)
#define GTA_X86 1
#else
#define GTA_X86 0
#endif

/**
 * A cross-compiler macro for identifying the system is x86_64.
 */
#if defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64) || defined(_M_X64) || defined(_M_AMD64)
#define GTA_X86_64 1
#else
#define GTA_X86_64 0
#endif

/**
 * A cross-compiler macro for identifying the system is ARM.
 */
#if defined(__arm__) || defined(__arm) || defined(__thumb__) || defined(__thumb) || defined(_M_ARM) || defined(_M_ARMT) || defined(__TARGET_ARCH_ARM) || defined(__TARGET_ARCH_THUMB) || defined(__TARGET_ARCH_ARM64) || defined(__TARGET_ARCH_THUMB)
#define GTA_ARM 1
#else
#define GTA_ARM 0
#endif

/**
 * A cross-compiler macro for identifying the system is ARM64.
 */
#if defined(__aarch64__) || defined(__aarch64) || defined(__arm64__) || defined(__arm64) || defined(__arm64)
#define GTA_ARM64 1
#else
#define GTA_ARM64 0
#endif

/**
 * A cross-compiler macro for identifying the system is IA64.
 */
#if defined(__ia64__) || defined(_IA64) || defined(__IA64__) || defined(_M_IA64) || defined(__itanium__)
#define GTA_IA64 1
#else
#define GTA_IA64 0
#endif

/**
 * Define the type for integers based on the system architecture.
 */
#if GTA_32_BIT
typedef int32_t GTA_Integer;
typedef uint32_t GTA_UInteger;
#define GTA_INTEGER_MAX INT32_MAX
#define GTA_INTEGER_MIN INT32_MIN
#define GTA_UINTEGER_MAX UINT32_MAX
#define GTA_UINTEGER_MIN 0
#elif GTA_64_BIT
typedef int64_t GTA_Integer;
typedef uint64_t GTA_UInteger;
#define GTA_INTEGER_MAX INT64_MAX
#define GTA_INTEGER_MIN INT64_MIN
#define GTA_UINTEGER_MAX UINT64_MAX
#define GTA_UINTEGER_MIN 0
#else
#error "Unsupported architecture"
#endif

/**
 * Define the type for floats based on the system architecture.
 */
#if GTA_32_BIT
typedef GCU_float32_t GTA_Float;
#elif GTA_64_BIT
typedef GCU_float64_t GTA_Float;
#else
#error "Unsupported architecture"
#endif

/**
 * Define the bytecode vector type based on the system architecture.
 */
#if GTA_32_BIT
typedef GCU_Vector32 GTA_Bytecode_Vector;
typedef GCU_Type32_Union GTA_Bytecode_Type_Union;
typedef GCU_Hash32 GTA_Bytecode_Hash;
typedef GCU_Hash32_Iterator GTA_Bytecode_Hash_Iterator;
#define GTA_STRING_HASH gcu_string_hash_32
#define GTA_VECTORX_CREATE gcu_vector32_create
#define GTA_VECTORX_DESTROY gcu_vector32_destroy
#define GTA_VECTORX_APPEND gcu_vector32_append
#define GTA_VECTORX_COUNT gcu_vector32_count
#define GTA_VECTORX_RESERVE gcu_vector32_reserve
#define GTA_HASHX_CREATE gcu_hash32_create
#define GTA_HASHX_DESTROY gcu_hash32_destroy
#define GTA_HASHX_SET gcu_hash32_set
#define GTA_HASHX_GET gcu_hash32_get
#define GTA_HASHX_CONTAINS gcu_hash32_contains
#define GTA_HASHX_REMOVE gcu_hash32_remove
#define GTA_HASHX_COUNT gcu_hash32_count
#define GTA_HASHX_ITERATOR_GET gcu_hash32_iterator_get
#define GTA_HASHX_ITERATOR_NEXT gcu_hash32_iterator_next
#define GTA_TYPEX_UI(X) (X).ui32
#define GTA_TYPEX_I(X) (X).i32
#define GTA_TYPEX_F(X) (X).f32
#define GTA_TYPEX_P(X) (X).p
#define GTA_TYPEX_B(X) (X).b
#define GTA_TYPEX_MAKE_UI(X) GCU_TYPE32_UI32(X)
#define GTA_TYPEX_MAKE_I(X) GCU_TYPE32_I32(X)
#define GTA_TYPEX_MAKE_F(X) GCU_TYPE32_F32(X)
#define GTA_TYPEX_MAKE_P(X) GCU_TYPE32_P(X)
#define GTA_TYPEX_MAKE_B(X) GCU_TYPE32_B(X)
#elif GTA_64_BIT

typedef GCU_Vector64 GTA_VectorX;
typedef GCU_Type64_Union GTA_TypeX_Union;
typedef GCU_Hash64 GTA_HashX;
typedef GCU_Hash64_Iterator GTA_HashX_Iterator;
typedef GCU_Hash64_Value GTA_HashX_Value;
#define GTA_STRING_HASH gcu_string_hash_64
#define GTA_VECTORX_CREATE gcu_vector64_create
#define GTA_VECTORX_DESTROY gcu_vector64_destroy
#define GTA_VECTORX_APPEND gcu_vector64_append
#define GTA_VECTORX_COUNT gcu_vector64_count
#define GTA_VECTORX_RESERVE gcu_vector64_reserve
#define GTA_HASHX_CREATE gcu_hash64_create
#define GTA_HASHX_DESTROY gcu_hash64_destroy
#define GTA_HASHX_SET gcu_hash64_set
#define GTA_HASHX_GET gcu_hash64_get
#define GTA_HASHX_CONTAINS gcu_hash64_contains
#define GTA_HASHX_REMOVE gcu_hash64_remove
#define GTA_HASHX_COUNT gcu_hash64_count
#define GTA_HASHX_ITERATOR_GET gcu_hash64_iterator_get
#define GTA_HASHX_ITERATOR_NEXT gcu_hash64_iterator_next
#define GTA_TYPEX_UI(X) (X).ui64
#define GTA_TYPEX_I(X) (X).i64
#define GTA_TYPEX_F(X) (X).f64
#define GTA_TYPEX_P(X) (X).p
#define GTA_TYPEX_B(X) (X).b
#define GTA_TYPEX_MAKE_UI(X) GCU_TYPE64_UI64(X)
#define GTA_TYPEX_MAKE_I(X) GCU_TYPE64_I64(X)
#define GTA_TYPEX_MAKE_F(X) GCU_TYPE64_F64(X)
#define GTA_TYPEX_MAKE_P(X) GCU_TYPE64_P(X)
#define GTA_TYPEX_MAKE_B(X) GCU_TYPE64_B(X)
#else
#error "Unsupported architecture"
#endif

/**
 * Macros for identifying the AST type.
 *
 * @{
 */
#define GTA_AST_IS_ARRAY(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_array_vtable)
#define GTA_AST_IS_ASSIGN(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_assign_vtable)
#define GTA_AST_IS_BINARY(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_binary_vtable)
#define GTA_AST_IS_BLOCK(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_block_vtable)
#define GTA_AST_IS_BOOLEAN(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_boolean_vtable)
#define GTA_AST_IS_BREAK(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_break_vtable)
#define GTA_AST_IS_CAST(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_cast_vtable)
#define GTA_AST_IS_CONTINUE(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_continue_vtable)
#define GTA_AST_IS_DO_WHILE(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_do_while_vtable)
#define GTA_AST_IS_FLOAT(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_float_vtable)
#define GTA_AST_IS_FOR(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_for_vtable)
#define GTA_AST_IS_FUNCTION(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_function_vtable)
#define GTA_AST_IS_FUNCTION_CALL(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_function_call_vtable)
#define GTA_AST_IS_GLOBAL(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_global_vtable)
#define GTA_AST_IS_IDENTIFIER(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_identifier_vtable)
#define GTA_AST_IS_IF_ELSE(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_if_else_vtable)
#define GTA_AST_IS_INDEX(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_index_vtable)
#define GTA_AST_IS_INTEGER(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_integer_vtable)
#define GTA_AST_IS_LIBRARY(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_library_vtable)
#define GTA_AST_IS_MAP(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_map_vtable)
#define GTA_AST_IS_PARSE_ERROR(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_parse_error_vtable)
#define GTA_AST_IS_PERIOD(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_period_vtable)
#define GTA_AST_IS_PRINT(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_print_vtable)
#define GTA_AST_IS_RANGED_FOR(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_ranged_for_vtable)
#define GTA_AST_IS_RETURN(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_return_vtable)
#define GTA_AST_IS_SLICE(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_slice_vtable)
#define GTA_AST_IS_STRING(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_string_vtable)
#define GTA_AST_IS_TERNARY(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_ternary_vtable)
#define GTA_AST_IS_UNARY(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_unary_vtable)
#define GTA_AST_IS_USE(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_use_vtable)
#define GTA_AST_IS_WHILE(X) (((GTA_Ast_Node *) X)->vtable == &gta_ast_node_while_vtable)

#define GTA_AST_IS_NUMERIC(X) (GTA_AST_IS_INTEGER(X) || GTA_AST_IS_FLOAT(X))
#define GTA_AST_IS_PRIMITIVE(X) (GTA_AST_IS_NUMERIC(X) || GTA_AST_IS_STRING(X) || GTA_AST_IS_BOOLEAN(X))

/**
 * @}
 */

/**
 * Macros for identifying the computed value type.
 * @{
 */
#define GTA_COMPUTED_VALUE_IS_ARRAY(X) (((GTA_Computed_Value *) X)->vtable == &gta_computed_value_array_vtable)
#define GTA_COMPUTED_VALUE_IS_BOOLEAN(X) (((GTA_Computed_Value *) X)->vtable == &gta_computed_value_boolean_vtable)
// Two vtables, because an error that renders as its own marker
// ([INTEGER TOO LARGE] and friends) is an error in every respect except that
// printing it produces the marker rather than nothing. Anything asking
// "is this an error" wants both.
#define GTA_COMPUTED_VALUE_IS_ERROR(X) ((((GTA_Computed_Value *) X)->vtable == &gta_computed_value_error_vtable) || (((GTA_Computed_Value *) X)->vtable == &gta_computed_value_error_marker_vtable))
#define GTA_COMPUTED_VALUE_IS_FLOAT(X) (((GTA_Computed_Value *) X)->vtable == &gta_computed_value_float_vtable)
#define GTA_COMPUTED_VALUE_IS_FUNCTION(X) (((GTA_Computed_Value *) X)->vtable == &gta_computed_value_function_vtable)
#define GTA_COMPUTED_VALUE_IS_FUNCTION_NATIVE(X) (((GTA_Computed_Value *) X)->vtable == &gta_computed_value_function_native_vtable)
#define GTA_COMPUTED_VALUE_IS_INTEGER(X) (((GTA_Computed_Value *) X)->vtable == &gta_computed_value_integer_vtable)
#define GTA_COMPUTED_VALUE_IS_ITERATOR(X) (((GTA_Computed_Value *) X)->vtable == &gta_computed_value_iterator_vtable)
#define GTA_COMPUTED_VALUE_IS_LIBRARY(X) (((GTA_Computed_Value *) X)->vtable == &gta_computed_value_library_vtable)
#define GTA_COMPUTED_VALUE_IS_MAP(X) (((GTA_Computed_Value *) X)->vtable == &gta_computed_value_map_vtable)
#define GTA_COMPUTED_VALUE_IS_NULL(X) (((GTA_Computed_Value *) X)->vtable == &gta_computed_value_null_vtable)
#define GTA_COMPUTED_VALUE_IS_RNG(X) (((GTA_Computed_Value *) X)->vtable == &gta_computed_value_rng_vtable)
#define GTA_COMPUTED_VALUE_IS_STRING(X) (((GTA_Computed_Value *) X)->vtable == &gta_computed_value_string_vtable)
/**
 * @}
 */

/**
 * Type prototypes.
 */
typedef uint32_t GTA_Program_Flags;
typedef GTA_HashX GTA_Ast_Simplify_Variable_Map;
typedef GTA_VectorX GTA_Computed_Value_Vector;
typedef GTA_VectorX GTA_Garbage_Collector_Allocations_List;
typedef struct GTA_Ast_Node GTA_Ast_Node;
typedef struct GTA_Ast_Node_Array GTA_Ast_Node_Array;
typedef struct GTA_Ast_Node_Assign GTA_Ast_Node_Assign;
typedef struct GTA_Ast_Node_Binary GTA_Ast_Node_Binary;
typedef struct GTA_Ast_Node_Block GTA_Ast_Node_Block;
typedef struct GTA_Ast_Node_Boolean GTA_Ast_Node_Boolean;
typedef struct GTA_Ast_Node_Break GTA_Ast_Node_Break;
typedef struct GTA_Ast_Node_Cast GTA_Ast_Node_Cast;
typedef struct GTA_Ast_Node_Continue GTA_Ast_Node_Continue;
typedef struct GTA_Ast_Node_Do_While GTA_Ast_Node_Do_While;
typedef struct GTA_Ast_Node_Float GTA_Ast_Node_Float;
typedef struct GTA_Ast_Node_For GTA_Ast_Node_For;
typedef struct GTA_Ast_Node_Function GTA_Ast_Node_Function;
typedef struct GTA_Ast_Node_Function_Call GTA_Ast_Node_Function_Call;
typedef struct GTA_Ast_Node_Global GTA_Ast_Node_Global;
typedef struct GTA_Ast_Node_Identifier GTA_Ast_Node_Identifier;
typedef struct GTA_Ast_Node_If_Else GTA_Ast_Node_If_Else;
typedef struct GTA_Ast_Node_Index GTA_Ast_Node_Index;
typedef struct GTA_Ast_Node_Integer GTA_Ast_Node_Integer;
typedef struct GTA_Ast_Node_Library GTA_Ast_Node_Library;
typedef struct GTA_Ast_Node_Map GTA_Ast_Node_Map;
typedef struct GTA_Ast_Node_Map_Pair GTA_Ast_Node_Map_Pair;
typedef struct GTA_Ast_Node_Parse_Error GTA_Ast_Node_Parse_Error;
typedef struct GTA_Ast_Node_Period GTA_Ast_Node_Period;
typedef struct GTA_Ast_Node_Print GTA_Ast_Node_Print;
typedef struct GTA_Ast_Node_Ranged_For GTA_Ast_Node_Ranged_For;
typedef struct GTA_Ast_Node_Return GTA_Ast_Node_Return;
typedef struct GTA_Ast_Node_Slice GTA_Ast_Node_Slice;
typedef struct GTA_Ast_Node_String GTA_Ast_Node_String;
typedef struct GTA_Ast_Node_Ternary GTA_Ast_Node_Ternary;
typedef struct GTA_Ast_Node_Unary GTA_Ast_Node_Unary;
typedef struct GTA_Ast_Node_Use GTA_Ast_Node_Use;
typedef struct GTA_Ast_Node_VTable GTA_Ast_Node_VTable;
typedef struct GTA_Ast_Node_While GTA_Ast_Node_While;
typedef struct GTA_Bytecode_Compiler_Context GTA_Bytecode_Compiler_Context;
typedef struct GTA_Compiler_Context GTA_Compiler_Context;
typedef struct GTA_Computed_Value GTA_Computed_Value;
typedef struct GTA_Computed_Value_Attribute_Pair GTA_Computed_Value_Attribute_Pair;
typedef struct GTA_Computed_Value_Array GTA_Computed_Value_Array;
typedef struct GTA_Computed_Value_Boolean GTA_Computed_Value_Boolean;
typedef struct GTA_Computed_Value_Error GTA_Computed_Value_Error;
typedef struct GTA_Computed_Value_Float GTA_Computed_Value_Float;
typedef struct GTA_Computed_Value_Function GTA_Computed_Value_Function;
typedef struct GTA_Computed_Value_Function_Native GTA_Computed_Value_Function_Native;
typedef struct GTA_Computed_Value_Integer GTA_Computed_Value_Integer;
typedef struct GTA_Computed_Value_Iterator GTA_Computed_Value_Iterator;
typedef struct GTA_Computed_Value_Library GTA_Computed_Value_Library;
typedef struct GTA_Computed_Value_Library_Attribute_Pair GTA_Computed_Value_Library_Attribute_Pair;
typedef struct GTA_Computed_Value_Map GTA_Computed_Value_Map;
typedef struct GTA_Computed_Value_RNG GTA_Computed_Value_RNG;
typedef struct GTA_Computed_Value_String GTA_Computed_Value_String;
typedef struct GTA_Computed_Value_VTable GTA_Computed_Value_VTable;
typedef struct GTA_Execution_Context GTA_Execution_Context;
typedef struct GTA_Language GTA_Language;
typedef struct GTA_Library GTA_Library;
typedef struct GTA_Program GTA_Program;
typedef struct GTA_Variable_Scope GTA_Variable_Scope;
typedef struct GTA_Unicode_String GTA_Unicode_String;
typedef struct GTA_Unicode_Rendered_String GTA_Unicode_Rendered_String;

/**
 * Helper union for converting between function pointers and void pointers.
 */
typedef union GTA_Function_Converter {
  GTA_Computed_Value * GTA_CALL (*f)(GTA_Execution_Context *); ///< As a function pointer.
  void * b; ///< As a void pointer.
} GTA_Function_Converter;

/**
 * Helper union for converting between function pointers and integers.
 */
typedef union GTA_JIT_Function_Converter {
  void GTA_CALL (*f)(void); ///< As a function pointer.
  GTA_UInteger i; ///< As an integer.
} GTA_JIT_Function_Converter;

/**
 * Convert a function pointer to an integer.
 */
#define GTA_JIT_FUNCTION_CONVERTER(F) \
  ((GTA_JIT_Function_Converter){.f = (void GTA_CALL (*)(void))(F)}.i)


#ifdef GTA_X86_64

// TODO(windows): Figure out the Windows ABI for x86-64 (x64).
// Although some of this is stubbed out, it is not fully working.
// I believe the problem is with the calling convention and the extra
// work and information that Windows requires for x64.
// See https://learn.microsoft.com/en-us/cpp/build/exception-handling-x64?view=msvc-170
// for more information.
// Specifically, the function prologue and epilogue are different for Windows
// x64 than for Linux x64 and, since our JIT compiled code can call functions,
// we may also need to register the JITTED functions with the Windows API
// before calling them (for proper exception handling).
#if !(defined(_WIN32) || defined(_WIN64))

// Define the function to compile the binary for x86-64 (Linux).
#define gta_program_compile_binary gta_program_compile_binary__x86_64
#endif // !(_WIN32 || _WIN64)
#endif // GTA_X86_64

#ifndef gta_program_compile_binary

// Fallback for all other compilers.
#define gta_program_compile_binary gta_program_compile_binary__x86_64
#endif // gta_program_compile_binary


/**
 * Macros for declaring functions to be run before/after main.
 */
#ifdef _MSC_VER  // If using Visual Studio

#include <windows.h>

// Define the startup macro for Visual Studio
#define GTA_INIT_FUNCTION(function_name) \
    __pragma(section(".CRT$XCU", read)) \
    __declspec(allocate(".CRT$XCU")) void (*function_name##_init)(void) = function_name; \
    static void function_name(void)

// Define the cleanup macro for Visual Studio
#define GTA_CLEANUP_FUNCTION(function_name) \
    __pragma(section(".CRT$XTU", read)) \
    __declspec(allocate(".CRT$XTU")) void (*function_name##_cleanup)(void) = function_name; \
    static void function_name(void)

#elif defined(__GNUC__) || defined(__clang__)  // If using GCC/Clang

// Define the startup macro for GCC/Clang
#define GTA_INIT_FUNCTION(function_name) \
    __attribute__((constructor)) static void function_name(void)

// Define the cleanup macro for GCC/Clang
#define GTA_CLEANUP_FUNCTION(function_name) \
    __attribute__((destructor)) static void function_name(void)

#else  // Other compilers (add more cases as needed)

// Default no-op macros for unsupported compilers
#define GTA_INIT_FUNCTION(function_name)
#define GTA_CLEANUP_FUNCTION(function_name)

#endif


#ifdef __cplusplus
}
#endif //__cplusplus

#endif // GHOTI_IO_GTA_MACROS_H
