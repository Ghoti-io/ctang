
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <gtest/gtest.h>
#include <ghoti.io/cutil/memory.h>
#include <algorithm>
#include <iostream>
#include <string>

#include <ghoti.io/tang/tang.h>
#include <ghoti.io/tang/macros.h>
#include <ghoti.io/tang/computedValue/computedValueAll.h>
#include <ghoti.io/tang/program/program.h>
#include <ghoti.io/tang/program/bytecode.h>
#include <ghoti.io/tang/program/executionContext.h>
#include <ghoti.io/tang/program/variable.h>
#include <ghoti.io/tang/unicodeString.h>

using namespace std;


GTA_Language * language;


#define TEST_REUSABLE_PROGRAM(code) \
  gcu_memory_reset_counts(); \
  GTA_Program * program = gta_program_create(language, code); \
  ASSERT_TRUE(program); \
  size_t alloc_count = gcu_get_alloc_count(); \
  size_t free_count = gcu_get_free_count();

#define TEST_REUSABLE_PROGRAM_TEARDOWN() \
  gcu_memory_reset_counts(); \
  gta_program_destroy(program); \
  ASSERT_EQ(alloc_count + gcu_get_alloc_count(), free_count + gcu_get_free_count());

#define TEST_CONTEXT_SETUP() \
  gcu_memory_reset_counts(); \
  GTA_Execution_Context * context = gta_execution_context_create(program); \
  ASSERT_TRUE(context);

#define TEST_CONTEXT_TEARDOWN() \
  gta_execution_context_destroy(context); \
  ASSERT_EQ(gcu_get_alloc_count(), gcu_get_free_count());

#define TEST_PROGRAM_SETUP(code) \
  TEST_REUSABLE_PROGRAM(code); \
  TEST_CONTEXT_SETUP(); \
  ASSERT_TRUE(gta_program_execute(context)); \
  ASSERT_TRUE(context->result);

#define TEST_PROGRAM_SETUP_NO_RUN(code) \
  TEST_REUSABLE_PROGRAM(code); \
  TEST_CONTEXT_SETUP();

#define TEST_PROGRAM_TEARDOWN() \
  TEST_CONTEXT_TEARDOWN(); \
  TEST_REUSABLE_PROGRAM_TEARDOWN();


TEST(Syntax, InvalidSyntax) {
  gcu_memory_reset_counts();
  GTA_Program * program = gta_program_create(language, "invalid syntax :(");
  ASSERT_FALSE(program);
  ASSERT_EQ(gcu_get_alloc_count(), gcu_get_free_count());
}

GTA_Computed_Value * GTA_CALL make_bool_true(GTA_MAYBE_UNUSED(GTA_Execution_Context * context)) {
  return (GTA_Computed_Value *)gta_computed_value_boolean_true;
}

GTA_Computed_Value * GTA_CALL make_bool_false(GTA_MAYBE_UNUSED(GTA_Execution_Context * context)) {
  return (GTA_Computed_Value *)gta_computed_value_boolean_false;
}

GTA_Computed_Value * GTA_CALL make_int_0(GTA_Execution_Context * context) {
  return (GTA_Computed_Value *)gta_computed_value_integer_create(0, context);
}

GTA_Computed_Value * GTA_CALL make_int_3(GTA_Execution_Context * context) {
  return (GTA_Computed_Value *)gta_computed_value_integer_create(3, context);
}

GTA_Computed_Value * GTA_CALL make_int_negative_10(GTA_Execution_Context * context) {
  return (GTA_Computed_Value *)gta_computed_value_integer_create(-10, context);
}

GTA_Computed_Value * GTA_CALL make_int_42(GTA_Execution_Context * context) {
  return (GTA_Computed_Value *)gta_computed_value_integer_create(42, context);
}

GTA_Computed_Value * GTA_CALL make_float_3(GTA_Execution_Context * context) {
  return (GTA_Computed_Value *)gta_computed_value_float_create(3., context);
}

GTA_Computed_Value * GTA_CALL make_float_3_5(GTA_Execution_Context * context) {
  return (GTA_Computed_Value *)gta_computed_value_float_create(3.5, context);
}

GTA_Computed_Value * GTA_CALL make_float_0(GTA_Execution_Context * context) {
  return (GTA_Computed_Value *)gta_computed_value_float_create(0.0, context);
}

GTA_Computed_Value * GTA_CALL make_string_hello(GTA_Execution_Context * context) {
  GTA_Unicode_String * hello = gta_unicode_string_create("hello", 5, GTA_UNICODE_STRING_TYPE_TRUSTED);
  return (GTA_Computed_Value *)gta_computed_value_string_create(hello, true, context);
}

GTA_Computed_Value * GTA_CALL make_string_empty(GTA_Execution_Context * context) {
  GTA_Unicode_String * empty = gta_unicode_string_create("", 0, GTA_UNICODE_STRING_TYPE_TRUSTED);
  return (GTA_Computed_Value *)gta_computed_value_string_create(empty, true, context);
}

GTA_Computed_Value * GTA_CALL make_string_3(GTA_Execution_Context * context) {
  GTA_Unicode_String * three = gta_unicode_string_create("3", 1, GTA_UNICODE_STRING_TYPE_TRUSTED);
  return (GTA_Computed_Value *)gta_computed_value_string_create(three, true, context);
}

GTA_Computed_Value * GTA_CALL make_string_42f5(GTA_Execution_Context * context) {
  GTA_Unicode_String * forty_two = gta_unicode_string_create("42.5", 4, GTA_UNICODE_STRING_TYPE_TRUSTED);
  return (GTA_Computed_Value *)gta_computed_value_string_create(forty_two, true, context);
}

TEST(Syntax, Empty) {
  // An empty program should return a null result.
  TEST_PROGRAM_SETUP("");
  ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
  TEST_PROGRAM_TEARDOWN();
}

TEST(Declare, Null) {
  // The null keyword should return a null result.
  TEST_PROGRAM_SETUP("null");
  ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
  TEST_PROGRAM_TEARDOWN();
}

TEST(Declare, Boolean) {
  {
    // The true keyword should return a boolean result with a value of true.
    TEST_PROGRAM_SETUP("true");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
    ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // The false keyword should return a boolean result with a value of false.
    TEST_PROGRAM_SETUP("false");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
    ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Declare, ManyDistinctLiterals) {
  // Each distinct literal value is interned as a singleton, in a table
  // created with room for 32. Inserting the 33rd grows it, and growth used to
  // run the table's cleanup hook over the entries it was moving rather than
  // discarding - destroying the very singletons the compiled program was
  // about to use. A program with more than 32 distinct literals died there,
  // as a segfault or a heap-corruption abort depending on what reused the
  // freed block.
  //
  // Nothing in this suite had crossed that threshold, which is why it went
  // unnoticed; these cases sit either side of it.
  for (int count : {32, 33, 100}) {
    std::string code = "a = 0;";
    for (int i = 0; i < count; ++i) {
      code += " a = " + std::to_string(i) + ";";
    }
    code += " a;";

    TEST_PROGRAM_SETUP(code.c_str());
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result))
        << count << " distinct literals";
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value,
        count - 1)
        << count << " distinct literals";
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Declare, Integer) {
  {
    // A positive integer.
    TEST_PROGRAM_SETUP("3");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 3);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A positive integer.
    TEST_PROGRAM_SETUP("42");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 42);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A negative integer.
    TEST_PROGRAM_SETUP("-42");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, -42);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // An integer with a value of 0.
    TEST_PROGRAM_SETUP("0");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 0);
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Declare, Float) {
  {
    // A positive float.
    TEST_PROGRAM_SETUP("3.14");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 3.14);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A positive float.
    TEST_PROGRAM_SETUP("42.0");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 42.0);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A negative float.
    TEST_PROGRAM_SETUP("-42.0");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, -42.0);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A float with a value of 0.
    TEST_PROGRAM_SETUP("0.");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 0.);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A float with a leading 0.
    TEST_PROGRAM_SETUP("0.42");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 0.42);
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Declare, String) {
  {
    // Empty string.
    TEST_PROGRAM_SETUP(R"("")");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
    ASSERT_STREQ(((GTA_Computed_Value_String *)context->result)->value->buffer, "");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Non-empty string.
    TEST_PROGRAM_SETUP(R"("hello")");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
    ASSERT_STREQ(((GTA_Computed_Value_String *)context->result)->value->buffer, "hello");
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Declare, Block) {
  {
    // Empty block.
    TEST_PROGRAM_SETUP("{}");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Block with a single statement.
    TEST_PROGRAM_SETUP("{3;}");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 3);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Block with multiple statements.
    TEST_PROGRAM_SETUP("{3; 4.5;}");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 4.5);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Block with multiple statements of different types.
    TEST_PROGRAM_SETUP(R"(3; 4.5; true; "hello"; "foo";)");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
    ASSERT_STREQ(((GTA_Computed_Value_String *)context->result)->value->buffer, "foo");
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Declare, Array) {
  {
    // Empty array.
    TEST_PROGRAM_SETUP("[]");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ARRAY(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Array *)context->result)->elements->count, 0);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Array with a single element.
    TEST_PROGRAM_SETUP("[3]");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ARRAY(context->result));
    GTA_Computed_Value_Array * array = (GTA_Computed_Value_Array *)context->result;
    ASSERT_EQ(array->elements->count, 1);
    GTA_Computed_Value * value = (GTA_Computed_Value *)GTA_TYPEX_P(array->elements->data[0]);
    ASSERT_FALSE(value->is_temporary);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(value));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)(value))->value, 3);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Array with multiple elements.
    TEST_PROGRAM_SETUP("[3, 4.5, true, \"hello\"]");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ARRAY(context->result));
    GTA_Computed_Value_Array * array = (GTA_Computed_Value_Array *)context->result;
    ASSERT_EQ(array->elements->count, 4);
    GTA_Computed_Value * value = (GTA_Computed_Value *)GTA_TYPEX_P(array->elements->data[0]);
    ASSERT_FALSE(value->is_temporary);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(value));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)(value))->value, 3);
    value = (GTA_Computed_Value *)GTA_TYPEX_P(array->elements->data[1]);
    ASSERT_FALSE(value->is_temporary);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(value));
    ASSERT_EQ(((GTA_Computed_Value_Float *)(value))->value, 4.5);
    value = (GTA_Computed_Value *)GTA_TYPEX_P(array->elements->data[2]);
    ASSERT_FALSE(value->is_temporary);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(value));
    ASSERT_TRUE(((GTA_Computed_Value_Boolean *)(value))->value);
    value = (GTA_Computed_Value *)GTA_TYPEX_P(array->elements->data[3]);
    ASSERT_FALSE(value->is_temporary);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(value));
    ASSERT_STREQ(((GTA_Computed_Value_String *)(value))->value->buffer, "hello");
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Declare, Map) {
  {
    // Empty map.
    TEST_PROGRAM_SETUP("{:}");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_MAP(context->result));
    GTA_Computed_Value_Map * map = (GTA_Computed_Value_Map *)context->result;
    ASSERT_EQ(map->key_hash->entries, 0);
    ASSERT_EQ(map->value_hash->entries, 0);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Map with multiple key-value pairs.
    TEST_PROGRAM_SETUP(R"({number: 4.5, greeting: "hello"})");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_MAP(context->result));
    GTA_Computed_Value_Map * map = (GTA_Computed_Value_Map *)context->result;
    ASSERT_EQ(map->key_hash->entries, 2);
    ASSERT_EQ(map->value_hash->entries, 2);
    GTA_Computed_Value * val1 = gta_computed_value_map_get_from_cstring(map, "number");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(val1));
    ASSERT_EQ(((GTA_Computed_Value_Float *)val1)->value, 4.5);
    GTA_Computed_Value * val2 = gta_computed_value_map_get_from_cstring(map, "greeting");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(val2));
    ASSERT_STREQ(((GTA_Computed_Value_String *)val2)->value->buffer, "hello");
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Identifier, Local) {
  {
    // `a` is not declared global, so should not have a value.
    TEST_PROGRAM_SETUP_NO_RUN("a");
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // `a` and `b` are not declared global.
    TEST_PROGRAM_SETUP_NO_RUN("a; b;");
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Identifier, Library) {
  {
    // `a` is declared global, so should have the default value.
    TEST_PROGRAM_SETUP_NO_RUN("use a; a;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 3);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // `a` and `b` are libraries.
    // Confirm that `a` can be loaded.
    TEST_PROGRAM_SETUP_NO_RUN("use a; use b; a;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 3);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // `a` and `b` are libraries.
    // Confirm that `b` can be loaded.
    TEST_PROGRAM_SETUP_NO_RUN("use a; use b; b;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 42);
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Assignment, ToIdentifier) {
  {
    // Assign a value to a local variable.
    TEST_PROGRAM_SETUP("a = 3");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 3);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Assign two local variables, return first.
    TEST_PROGRAM_SETUP("a = 3; b = 4; a;");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 3);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Assign two local variables, return second.
    TEST_PROGRAM_SETUP("a = 3; b = 4; b;");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 4);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Assign a value to a library.
    TEST_PROGRAM_SETUP_NO_RUN("use a; a = 42; a;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 42);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Load two libraries.  Overwrite the first, return the first.
    TEST_PROGRAM_SETUP_NO_RUN("use a; use b; a = 10; a;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 10);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Load two libraries.  Overwrite the second, return the first.
    TEST_PROGRAM_SETUP_NO_RUN("use a; use b; b = 10; a;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 3);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Load two libraries.  Overwrite the second, return the second.
    TEST_PROGRAM_SETUP_NO_RUN("use a; use b; b = 10; b;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 10);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Load a library, then assign a value to a local variable, then load
    // another library.  Return the local variable.
    // Verifying that the local variable is not overwritten by the library.
    TEST_PROGRAM_SETUP_NO_RUN("use a; b = 10; use c; b;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "c", make_int_42));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 10);
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Unary, Negative) {
  // Optimizations mean that something like "-3", which is two AST nodes, will
  // be simplified to a single AST node.  Therefore, we will use a variable to
  // prevent the optimization.
  {
    // Integer negation.
    TEST_PROGRAM_SETUP_NO_RUN("use a; -a;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, -3);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Double negation.
    TEST_PROGRAM_SETUP_NO_RUN("use a; -(-a);");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 3);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Float negation.
    TEST_PROGRAM_SETUP_NO_RUN("use a; -a;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
    ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, -3.5);
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Unary, Not) {
  {
    // !null
    TEST_PROGRAM_SETUP_NO_RUN("!a;");
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
    ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // !!null (double negation)
    TEST_PROGRAM_SETUP_NO_RUN("!!a;");
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
    ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Boolean !true
    TEST_PROGRAM_SETUP_NO_RUN("use a; !a;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_bool_true));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
    ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Boolean !false
    TEST_PROGRAM_SETUP_NO_RUN("use a; !a;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_bool_false));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
    ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Integer !3
    TEST_PROGRAM_SETUP_NO_RUN("use a; !a;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
    ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Integer !0
    TEST_PROGRAM_SETUP_NO_RUN("use a; !a;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_0));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
    ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Float !3.5
    TEST_PROGRAM_SETUP_NO_RUN("use a; !a;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
    ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Float !0.0
    TEST_PROGRAM_SETUP_NO_RUN("use a; !a;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_0));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
    ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // String !"hello"
    TEST_PROGRAM_SETUP_NO_RUN("use a; !a;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_string_hello));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
    ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // String !""
    TEST_PROGRAM_SETUP_NO_RUN("use a; !a;");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_string_empty));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
    ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Binary, Add) {
  {
    TEST_REUSABLE_PROGRAM("use a; use b; a + b;");
    {
      // Integer + Integer.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 45);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer + Float.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 6.5);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float + Float.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 7.0);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float + Integer.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 6.5);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Binary, Subtract) {
  {
    TEST_REUSABLE_PROGRAM("use a; use b; a - b;");
    {
      // Integer - Integer.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 39);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer - Float.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 38.5);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float - Float.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 0.0);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float - Integer.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 0.5);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Binary, Multiply) {
  {
    TEST_REUSABLE_PROGRAM("use a; use b; a * b;");
    {
      // Integer * Integer.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 126);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer * Float.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 10.5);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float * Float.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 12.25);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float * Integer.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 10.5);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Binary, Divide) {
  {
    TEST_REUSABLE_PROGRAM("use a; use b; a / b;");
    {
      // Integer / Integer.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 14.0);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer / -Integer. (not evenly divisible)
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_negative_10));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, -3);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer / Float.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 12.0);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float / Float.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 1.0);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float / Integer.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Float *)context->result)->value, 1.1666666666666667);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Division by zero, Integer / Integer.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_0));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ERROR(context->result));
      ASSERT_EQ(context->result, gta_computed_value_error_divide_by_zero);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Division by zero, Integer / Float.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_0));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ERROR(context->result));
      ASSERT_EQ(context->result, gta_computed_value_error_divide_by_zero);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Division by zero, Float / Float.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_0));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ERROR(context->result));
      ASSERT_EQ(context->result, gta_computed_value_error_divide_by_zero);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Division by zero, Float / Integer.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_0));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ERROR(context->result));
      ASSERT_EQ(context->result, gta_computed_value_error_divide_by_zero);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Binary, Modulo) {
  {
    TEST_REUSABLE_PROGRAM("use a; use b; a % b;");
    {
      // Integer % Integer.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 0);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // -Integer % Integer. (not evenly divisible)
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_negative_10));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, -1);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer % -Integer. (not evenly divisible)
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_negative_10));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
      ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 2);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer % Float. Should error.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ERROR(context->result));
      ASSERT_EQ(context->result, gta_computed_value_error_not_supported);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Binary, LessThan) {
  {
    TEST_REUSABLE_PROGRAM("use a; use b; a < b;");
    {
      // Integer < Integer => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer < Integer => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer < Integer => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer < Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer < Float => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float < Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_0));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float < Float => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float < Float => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_0));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float < Integer => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float < Integer => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Binary, LessThanEqual) {
  {
    TEST_REUSABLE_PROGRAM("use a; use b; a <= b;");
    {
      // Integer <= Integer => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer <= Integer => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer <= Integer => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer <= Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer <= Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer <= Float => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float <= Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_0));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float <= Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float <= Float => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_0));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float <= Integer => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float <= Integer => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float <= Integer => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Binary, GreaterThan) {
  {
    TEST_REUSABLE_PROGRAM("use a; use b; a > b;");
    {
      // Integer > Integer => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer > Integer => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer > Integer => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer > Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer > Float => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float > Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_0));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float > Float => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float > Float => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_0));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float > Integer => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float > Integer => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Binary, GreaterThanEqual) {
  {
    TEST_REUSABLE_PROGRAM("use a; use b; a >= b;");
    {
      // Integer >= Integer => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer >= Integer => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer >= Integer => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer >= Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer >= Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer >= Float => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float >= Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_0));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float >= Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float >= Float => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_0));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float >= Integer => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float >= Integer => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Binary, Equal) {
  {
    TEST_REUSABLE_PROGRAM("use a; use b; a == b;");
    {
      // Integer == Integer => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer == Integer => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer == Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer == Float => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float == Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float == Float => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_0));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float == Integer => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float == Integer => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Binary, NotEqual) {
  {
    TEST_REUSABLE_PROGRAM("use a; use b; a != b;");
    {
      // Integer != Integer => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer != Integer => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer != Float => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer != Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float != Float => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float != Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_float_0));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float != Integer => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float != Integer => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Binary, And) {
  {
    TEST_REUSABLE_PROGRAM("use a; use b; a && b;");
    {
      // true && true => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_bool_true));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_bool_true));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // true && false => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_bool_true));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_bool_false));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // false && true => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_bool_false));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_bool_true));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // false && false => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_bool_false));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_bool_false));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Binary, Or) {
  {
    TEST_REUSABLE_PROGRAM("use a; use b; a || b;");
    {
      // true || true => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_bool_true));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_bool_true));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // true || false => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_bool_true));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_bool_false));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // false || true => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_bool_false));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_bool_true));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // false || false => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_bool_false));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_bool_false));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Ternary, Conditional) {
  {
    TEST_REUSABLE_PROGRAM("use a; use b; use c; a ? b : c;");
    {
      // true ? 42 : 3 => 42.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_bool_true));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "c", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
      ASSERT_EQ(42, ((GTA_Computed_Value_Integer *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // false ? 42 : 3 => 3.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_bool_false));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "b", make_int_42));
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "c", make_int_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
      ASSERT_EQ(3, ((GTA_Computed_Value_Integer *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Cast, ToBoolean) {
  {
    TEST_REUSABLE_PROGRAM("use a; a as bool;");
    {
      // Integer => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Integer => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_0));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_0));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Boolean => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_bool_true));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Null => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // String => true.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_string_hello));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Empty String => false.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_string_empty));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Case, ToInteger) {
  {
    TEST_REUSABLE_PROGRAM("use a; a as int;");
    {
      // Integer => Integer.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
      ASSERT_EQ(42, ((GTA_Computed_Value_Integer *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float => Integer.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
      ASSERT_EQ(3, ((GTA_Computed_Value_Integer *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Boolean => Integer.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_bool_true));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
      ASSERT_EQ(1, ((GTA_Computed_Value_Integer *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Null => Integer.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
      ASSERT_EQ(0, ((GTA_Computed_Value_Integer *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // String => Integer.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_string_3));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
      ASSERT_EQ(3, ((GTA_Computed_Value_Integer *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Syntax, IntegerDivisionOverflowDoesNotKillTheProcess) {
  // INT64_MIN / -1 overflows, and on x86-64 the hardware raises SIGFPE rather
  // than wrapping - so this was not a wrong answer but a dead process, from a
  // one-line script, in both execution engines.
  //
  // This test only asserts that the line is reached at all. What the operators
  // now answer is asserted in Syntax.IntegerOverflowIsReportedNotWrapped.
  {
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      print((0 - 9223372036854775807 - 1) / -1);
      print(" end");
    )");
    // What matters is reaching this line at all.
    ASSERT_NE(context->output->buffer, nullptr);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      print((0 - 9223372036854775807 - 1) % -1);
      print(" end");
    )");
    ASSERT_NE(context->output->buffer, nullptr);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Ordinary division must be untouched by the guard.
    TEST_PROGRAM_SETUP(R"(
      print(10 / 2); print(","); print(10 % 3); print(","); print(0 - 7 / 2);
    )");
    ASSERT_STREQ(context->output->buffer, "5,1,-3");
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(Cast, OutOfRangeStringToIntegerSaysSo) {
  // `"99999999999999999999999" as int` was 9223372036854775807 - the clamp to
  // INT_MAX that the markers exist to remove - and it got there through atoll,
  // whose behaviour when the value does not fit is undefined. glibc happens to
  // saturate; nothing promises it.
  //
  // The marker text is the same as the float cast's deliberately. Both mean
  // "this value does not fit in an integer", and spelling them differently by
  // source type would invite a reader to think they were different conditions.
  {
    TEST_PROGRAM_SETUP(R"(print("99999999999999999999999" as int);)");
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO LARGE]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    TEST_PROGRAM_SETUP(R"(print("-99999999999999999999999" as int);)");
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO SMALL]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // The boundaries themselves convert; one past does not.
    TEST_PROGRAM_SETUP(R"(print("9223372036854775807" as int); print(","); print("-9223372036854775808" as int);)");
    ASSERT_STREQ(context->output->buffer, "9223372036854775807,-9223372036854775808");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    TEST_PROGRAM_SETUP(R"(print("9223372036854775808" as int);)");
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO LARGE]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // The leading-number rule is untouched: a string that continues past its
    // number still converts, and leading whitespace is still skipped.
    TEST_PROGRAM_SETUP(R"(print("42" as int); print(","); print("12abc" as int); print(","); print("  7" as int);)");
    ASSERT_STREQ(context->output->buffer, "42,12,7");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A string with no leading number is not 0. It used to be, which made it
    // indistinguishable from the string "0" - two different situations giving
    // one plausible answer, which is the collapse these markers undo.
    TEST_PROGRAM_SETUP(R"(print("abc" as int); print(","); print("" as int); print(","); print("0" as int);)");
    ASSERT_STREQ(context->output->buffer, "[NOT A NUMBER],[NOT A NUMBER],0");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // The float cast answers the same way, and for the same reason.
    TEST_PROGRAM_SETUP(R"(print("abc" as float); print(","); print("" as float); print(","); print("3.3" as float); print(","); print("12abc" as float);)");
    ASSERT_STREQ(context->output->buffer, "[NOT A NUMBER],[NOT A NUMBER],3.3,12.");
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(Cast, OutOfRangeFloatToIntegerSaysSo) {
  // Casting a float that does not fit in an integer used to be undefined
  // behaviour, and on x86-64 the conversion instruction answers with the
  // "integer indefinite" value. That is GTA_INTEGER_MIN - for a huge POSITIVE
  // float as readily as a huge negative one - so `hugeFloat as int` printed
  // -9223372036854775808 and nothing told the reader that the number was not
  // the answer. It now says which way it went out of range.
  //
  // Both engines run this file, and both must agree: the constant folder
  // declines to fold an out-of-range cast rather than carrying a second copy
  // of the rule, so there is one answer by construction.

  // Larger than any double, so it is an infinity. Exponent notation does not
  // lex, hence the digits.
  const std::string huge = std::string(320, '9') + ".0";

  {
    // In range, and still folded and converted as before.
    TEST_PROGRAM_SETUP(R"(print(3.9 as int); print(","); print((0.0 - 3.9) as int);)");
    ASSERT_STREQ(context->output->buffer, "3,-3");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Finite, but far outside the range.
    TEST_PROGRAM_SETUP(R"(print(77777777777777777777777.9 as int);)");
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO LARGE]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    TEST_PROGRAM_SETUP(R"(print((0.0 - 77777777777777777777777.9) as int);)");
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO SMALL]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Infinity.
    std::string src = "print(" + huge + " as int);";
    TEST_PROGRAM_SETUP(src.c_str());
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO LARGE]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    std::string src = "print((0.0 - " + huge + ") as int);";
    TEST_PROGRAM_SETUP(src.c_str());
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO SMALL]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A NaN is neither, and is reachable: infinity minus infinity. It compares
    // false against every bound, so without its own test it would fall through
    // into the conversion and be undefined behaviour again.
    std::string src = "print((" + huge + " - " + huge + ") as int);";
    TEST_PROGRAM_SETUP(src.c_str());
    ASSERT_STREQ(context->output->buffer, "[NOT A NUMBER]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // The boundary, which is where a naive test gets it wrong. GTA_INTEGER_MAX
    // is not representable as a double and rounds UP to 2^63, so the literal
    // spelling of the maximum does not fit and must be refused...
    TEST_PROGRAM_SETUP(R"(print(9223372036854775807.0 as int);)");
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO LARGE]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // ...while the largest double below 2^63 converts exactly, and so does
    // GTA_INTEGER_MIN, which is a power of two and exact.
    TEST_PROGRAM_SETUP(R"(print(9223372036854774784.0 as int); print(","); print((0.0 - 9223372036854775808.0) as int);)");
    ASSERT_STREQ(context->output->buffer, "9223372036854774784,-9223372036854775808");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // The marker prints where the number would have gone, and execution
    // carries on - it is a value, not a halt.
    TEST_PROGRAM_SETUP(R"(print("A"); print(77777777777777777777777.9 as int); print("B");)");
    ASSERT_STREQ(context->output->buffer, "A[INTEGER TOO LARGE]B");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // It survives assignment, so it cannot be laundered into a number by
    // storing it first.
    TEST_PROGRAM_SETUP(R"(x = 77777777777777777777777.9 as int; print(x);)");
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO LARGE]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // And it is an error value: arithmetic on it does not produce a number,
    // and it is false in a condition.
    TEST_PROGRAM_SETUP(R"(if (77777777777777777777777.9 as int) { print("T"); } else { print("F"); })");
    ASSERT_STREQ(context->output->buffer, "F");
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(Print, Boolean) {
  // Printing a boolean rendered as nothing: the boolean vtable's print was
  // print_not_supported, even though its to_string already produced "true" and
  // "false" and `true as string` already printed them. In a language whose job
  // is generating text, that is a value that cannot be seen - and it quietly
  // turned `print(a == b)` in a test into an assertion of nothing.
  {
    TEST_PROGRAM_SETUP(R"(print(true); print(","); print(false);)");
    ASSERT_STREQ(context->output->buffer, "true,false");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // However the boolean arrives: from a comparison, from a negation, from a
    // cast, and through a variable.
    TEST_PROGRAM_SETUP(R"(
      print(1 < 2); print(","); print(1 > 2); print(",");
      print(!true); print(","); print(1 as bool); print(",");
      x = 2 == 2; print(x);
    )");
    ASSERT_STREQ(context->output->buffer, "true,false,false,true,true");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // And it still agrees with the string cast, which was already right.
    TEST_PROGRAM_SETUP(R"(print(true as string); print(","); print(true);)");
    ASSERT_STREQ(context->output->buffer, "true,true");
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(Syntax, IntegerLiteralOutOfRangeIsRejected) {
  // A literal that does not fit used to saturate, so `9223372036854775808`
  // silently became `9223372036854775807`. A literal has no operands and
  // cannot vary, so there is nothing to wait for: it is refused at parse time
  // rather than answered with a run-time marker.
  for (const char * src : {
      "9223372036854775808;",
      "99999999999999999999999;",
      "9223372036854775808 - 1;",
      "x = 9223372036854775808;",
      "-99999999999999999999999;",
      }) {
    gcu_memory_reset_counts();
    GTA_Program * program = gta_program_create(language, src);
    ASSERT_FALSE(program) << "accepted: " << src;
    ASSERT_EQ(gcu_get_alloc_count(), gcu_get_free_count()) << "leaked on: " << src;
  }
  {
    // The bounds themselves must still be accepted.
    TEST_PROGRAM_SETUP(R"(print(9223372036854775807);)");
    ASSERT_STREQ(context->output->buffer, "9223372036854775807");
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(Syntax, TheMostNegativeIntegerCanBeWritten) {
  // Tang has no negative literals - unary minus is an operator - and the
  // signed range is asymmetric, so the most negative integer can only be
  // written as minus applied to a magnitude that is itself one past the
  // maximum. That made it unwritable twice over: the literal saturated, so
  // `-9223372036854775808` evaluated to `-9223372036854775807`, off by one and
  // silently; and refusing out-of-range literals outright would have left no
  // way to write it at all.
  //
  // So that magnitude is accepted in exactly one position, directly after
  // unary minus, and is a syntax error anywhere else. Java has the same
  // carve-out for the same reason.
  {
    TEST_PROGRAM_SETUP(R"(print(-9223372036854775808);)");
    ASSERT_STREQ(context->output->buffer, "-9223372036854775808");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Whitespace between the operator and the literal is still fine, and the
    // value is exact rather than a negation that would itself overflow - so it
    // matches the arithmetic spelling of the same number. Compared by printing
    // both rather than by printing the comparison, because printing a boolean
    // produces nothing (13.8).
    TEST_PROGRAM_SETUP(R"(print(- 9223372036854775808); print(","); print(0 - 9223372036854775807 - 1);)");
    ASSERT_STREQ(context->output->buffer, "-9223372036854775808,-9223372036854775808");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // But only there. Without the minus it is the out-of-range literal again.
    gcu_memory_reset_counts();
    GTA_Program * program = gta_program_create(language, "9223372036854775808;");
    ASSERT_FALSE(program);
    ASSERT_EQ(gcu_get_alloc_count(), gcu_get_free_count());
  }
}


TEST(Syntax, IntegerOverflowIsReportedNotWrapped) {
  // Arithmetic that cannot produce a representable answer says so, rather than
  // wrapping. Wrapping was the worst of the three options: a clamp is wrong by
  // a bounded amount, but 9223372036854775807 + 1 came back as the most
  // negative integer there is, which looks exactly like a real result.
  //
  // GTA_INTEGER_MIN cannot be written as a literal - 9223372036854775808
  // saturates to the maximum - so it is built by arithmetic here.
  //
  // Both engines run this file and both must agree: the constant folder
  // declines to fold anything that overflows, so run time answers all of it.
  {
    TEST_PROGRAM_SETUP(R"(print(9223372036854775807 + 1);)");
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO LARGE]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    TEST_PROGRAM_SETUP(R"(print((0 - 9223372036854775807 - 1) - 1);)");
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO SMALL]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    TEST_PROGRAM_SETUP(R"(print(9223372036854775807 * 2);)");
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO LARGE]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A negative product reports the other direction.
    TEST_PROGRAM_SETUP(R"(print((0 - 9223372036854775807 - 1) * 2);)");
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO SMALL]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Negation, where the result is one past the maximum. Both spellings.
    TEST_PROGRAM_SETUP(R"(print(-(0 - 9223372036854775807 - 1));)");
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO LARGE]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    TEST_PROGRAM_SETUP(R"(print(0 - (0 - 9223372036854775807 - 1));)");
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO LARGE]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // GTA_INTEGER_MIN / -1 is one past the maximum, and raised SIGFPE before
    // it was guarded at all - a killed process from one line of script.
    TEST_PROGRAM_SETUP(R"(print((0 - 9223372036854775807 - 1) / -1);)");
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO LARGE]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // But the REMAINDER is representable: x % -1 is 0 for every x, under both
    // the truncating and the flooring convention. Only the hardware objected,
    // because it computes the quotient too. It used to be refused; it has an
    // ordinary answer.
    TEST_PROGRAM_SETUP(R"(print((0 - 9223372036854775807 - 1) % -1); print(","); print(7 % -1);)");
    ASSERT_STREQ(context->output->buffer, "0,0");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Ordinary arithmetic must be untouched, or the guard has simply broken
    // the operators.
    TEST_PROGRAM_SETUP(R"(
      print(2 + 3); print(","); print(10 - 4); print(","); print(6 * 7);
      print(","); print(-5); print(","); print(-(0 - 5));
      print(","); print(10 / 2); print(","); print(10 % 3);
      print(","); print(9223372036854775807); print(","); print(0 - 9223372036854775807 - 1);
    )");
    ASSERT_STREQ(context->output->buffer,
      "5,6,42,-5,5,5,1,9223372036854775807,-9223372036854775808");
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(Declare, FloatLiteralsDoNotShareASingleton) {
  // Literals are interned in the program's singleton pool, which is keyed by a
  // GTA_UInteger. The float node passed its GTA_Float straight into that
  // parameter, which converts rather than reinterprets, so the key was the
  // truncated integer part and every literal sharing one collapsed onto a
  // single value. `print(1.5); print(1.0);` printed 1.5 twice.
  //
  // Both engines were affected, because the defect is in the shared front end.
  {
    // Same integer part, different values: the case that collided.
    TEST_PROGRAM_SETUP(R"(
      print(1.5); print(","); print(1.0); print(","); print(1.25);
    )");
    ASSERT_STREQ(context->output->buffer, "1.5,1.,1.25");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Integer part 0 collided too, which the 1.x cases alone would not catch.
    TEST_PROGRAM_SETUP(R"(
      print(0.5); print(","); print(0.25);
    )");
    ASSERT_STREQ(context->output->buffer, "0.5,0.25");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Arithmetic on two such literals produced the wrong sum, because the
    // second operand was the first literal's value.
    TEST_PROGRAM_SETUP(R"(
      print(1.5 + 1.0); print(","); print(0.5 + 0.25);
    )");
    ASSERT_STREQ(context->output->buffer, "2.5,0.75");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Interning must still work: one value used twice is still one singleton,
    // and a fix that simply stopped sharing would pass every case above.
    TEST_PROGRAM_SETUP(R"(
      a = 2.75;
      b = 2.75;
      print(a == b);
      print(a); print(","); print(b);
    )");
    // The leading "true" is the `a == b` above. It used to contribute nothing,
    // because printing a boolean rendered as nothing - so the interning check
    // this case exists for was silently asserting only that two prints of 2.75
    // came out as 2.75.
    ASSERT_STREQ(context->output->buffer, "true2.75,2.75");
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(Declare, LargeFloatRendersWithoutOverflowingItsBuffer) {
  // to_string rendered with %f into a fixed 32-byte allocation. %f writes a
  // large magnitude in full rather than in exponent form, so this overflowed
  // the heap - the first case below aborted inside glibc with a corrupted
  // heap before the buffer was sized from the value.
  {
    TEST_PROGRAM_SETUP(R"(
      a = 1.0;
      for (i = 0; i < 40; i = i + 1) {
        a = a * 10.0;
      }
      print(a);
    )");
    // The exact digits are the double nearest 1e40; what matters is that it
    // renders in full and does not overrun.
    ASSERT_GT(strlen(context->output->buffer), (size_t)32);
    ASSERT_EQ(context->output->buffer[0], '9');
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Far past the old buffer: DBL_MAX needs 316 characters.
    TEST_PROGRAM_SETUP(R"(
      a = 1.0;
      for (i = 0; i < 300; i = i + 1) {
        a = a * 10.0;
      }
      print(a);
    )");
    ASSERT_GT(strlen(context->output->buffer), (size_t)300);
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(Cast, ToFloat) {
  {
    TEST_REUSABLE_PROGRAM("use a; a as float;");
    {
      // Integer => Float.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(42.0, ((GTA_Computed_Value_Float *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float => Float.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(3.5, ((GTA_Computed_Value_Float *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Boolean => Float.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_bool_true));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(1.0, ((GTA_Computed_Value_Float *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Null => Float.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(0.0, ((GTA_Computed_Value_Float *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // String => Float.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_string_42f5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
      ASSERT_EQ(42.5, ((GTA_Computed_Value_Float *)context->result)->value);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Cast, ToString) {
  {
    TEST_REUSABLE_PROGRAM("use a; a as string;");
    {
      // Integer => String.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_42));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
      ASSERT_STREQ("42", ((GTA_Computed_Value_String *)context->result)->value->buffer);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Float => String.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_float_3_5));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
      ASSERT_STREQ("3.5", ((GTA_Computed_Value_String *)context->result)->value->buffer);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Boolean => String.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_bool_true));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
      ASSERT_STREQ("true", ((GTA_Computed_Value_String *)context->result)->value->buffer);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // Null => String.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
      ASSERT_STREQ("null", ((GTA_Computed_Value_String *)context->result)->value->buffer);
      TEST_CONTEXT_TEARDOWN();
    }
    {
      // String => String.
      TEST_CONTEXT_SETUP();
      ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_string_hello));
      ASSERT_TRUE(gta_program_execute(context));
      ASSERT_TRUE(context->result);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
      ASSERT_STREQ("hello", ((GTA_Computed_Value_String *)context->result)->value->buffer);
      TEST_CONTEXT_TEARDOWN();
    }
    TEST_REUSABLE_PROGRAM_TEARDOWN();
  }
}

TEST(Index, Array) {
  {
    // First element.
    TEST_PROGRAM_SETUP("a = [3, 4.5, true, \"hello\"]; a[0];");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(3, ((GTA_Computed_Value_Integer *)context->result)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Last element.
    TEST_PROGRAM_SETUP("a = [3, 4.5, true, \"hello\"]; a[3];");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
    ASSERT_STREQ("hello", ((GTA_Computed_Value_String *)context->result)->value->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Out of bounds.
    TEST_PROGRAM_SETUP("a = [3, 4.5, true, \"hello\"]; a[4];");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Negative index from end.
    TEST_PROGRAM_SETUP("a = [3, 4.5, true, \"hello\"]; a[-2];");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result));
    ASSERT_TRUE(((GTA_Computed_Value_Boolean *)context->result)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Negative index out of bounds.
    TEST_PROGRAM_SETUP("a = [3, 4.5, true, \"hello\"]; a[-5];");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Non-integer index.
    TEST_PROGRAM_SETUP("a = [3, 4.5, true, \"hello\"]; a[3.5];");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ERROR(context->result));
    ASSERT_EQ(context->result, gta_computed_value_error_invalid_index);
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Index, String) {
  {
    // First character.
    TEST_PROGRAM_SETUP("a = \"hello\"; a[0];");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
    ASSERT_STREQ("h", ((GTA_Computed_Value_String *)context->result)->value->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Last character.
    TEST_PROGRAM_SETUP("a = \"hello\"; a[4];");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
    ASSERT_STREQ("o", ((GTA_Computed_Value_String *)context->result)->value->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Out of bounds.
    TEST_PROGRAM_SETUP("a = \"hello\"; a[5];");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
    ASSERT_STREQ("", ((GTA_Computed_Value_String *)context->result)->value->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Negative index from end.
    TEST_PROGRAM_SETUP("a = \"hello\"; a[-4];");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
    ASSERT_STREQ("e", ((GTA_Computed_Value_String *)context->result)->value->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Negative index out of bounds.
    TEST_PROGRAM_SETUP("a = \"hello\"; a[-6];");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
    ASSERT_STREQ("", ((GTA_Computed_Value_String *)context->result)->value->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Non-integer index.
    TEST_PROGRAM_SETUP("a = \"hello\"; a[3.5];");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ERROR(context->result));
    ASSERT_EQ(context->result, gta_computed_value_error_invalid_index);
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Index, Map) {
  {
    // Existing key.
    TEST_PROGRAM_SETUP(R"(
      a = {
        foo: 3,
        bar: 4.5,
        baz: true,
        qux: "hello",
      };
      a["bar"];)");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(context->result));
    ASSERT_EQ(4.5, ((GTA_Computed_Value_Float *)context->result)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Non-existent key.
    TEST_PROGRAM_SETUP(R"(
      a = {
        foo: 3,
        bar: 4.5,
        baz: true,
        qux: "hello",
      };
      a["quux"];)");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Non-string key.
    TEST_PROGRAM_SETUP(R"(
      a = {
        foo: 3,
        bar: 4.5,
        baz: true,
        qux: "hello",
      };
      a[3];)");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ERROR(context->result));
    ASSERT_EQ(context->result, gta_computed_value_error_map_key_not_string);
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Assignment, ArrayIndex) {
  {
    // Assign to an existing index.
    TEST_PROGRAM_SETUP(R"(a = [3, 4.5, true, "hello"]; a[2] = false; a;)");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ARRAY(context->result));
    GTA_Computed_Value_Array * result = (GTA_Computed_Value_Array *)context->result;
    ASSERT_EQ(4, result->elements->count);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(result->elements->data[0].p));
    ASSERT_EQ(3, ((GTA_Computed_Value_Integer *)result->elements->data[0].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(result->elements->data[1].p));
    ASSERT_EQ(4.5, ((GTA_Computed_Value_Float *)result->elements->data[1].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(result->elements->data[2].p));
    ASSERT_FALSE(((GTA_Computed_Value_Boolean *)result->elements->data[2].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(result->elements->data[3].p));
    ASSERT_STREQ("hello", ((GTA_Computed_Value_String *)result->elements->data[3].p)->value->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Assign to a new index.
    TEST_PROGRAM_SETUP(R"(a = [3, 4.5, true, "hello"]; a[4] = 42; a;)");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ARRAY(context->result));
    GTA_Computed_Value_Array * result = (GTA_Computed_Value_Array *)context->result;
    ASSERT_EQ(5, result->elements->count);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(result->elements->data[0].p));
    ASSERT_EQ(3, ((GTA_Computed_Value_Integer *)result->elements->data[0].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(result->elements->data[1].p));
    ASSERT_EQ(4.5, ((GTA_Computed_Value_Float *)result->elements->data[1].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(result->elements->data[2].p));
    ASSERT_TRUE(((GTA_Computed_Value_Boolean *)result->elements->data[2].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(result->elements->data[3].p));
    ASSERT_STREQ("hello", ((GTA_Computed_Value_String *)result->elements->data[3].p)->value->buffer);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(result->elements->data[4].p));
    ASSERT_EQ(42, ((GTA_Computed_Value_Integer *)result->elements->data[4].p)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Assign to a negative index.
    TEST_PROGRAM_SETUP(R"(a = [3, 4.5, true, "hello"]; a[-2] = 42; a;)");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ARRAY(context->result));
    GTA_Computed_Value_Array * result = (GTA_Computed_Value_Array *)context->result;
    ASSERT_EQ(4, result->elements->count);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(result->elements->data[0].p));
    ASSERT_EQ(3, ((GTA_Computed_Value_Integer *)result->elements->data[0].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(result->elements->data[1].p));
    ASSERT_EQ(4.5, ((GTA_Computed_Value_Float *)result->elements->data[1].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(result->elements->data[2].p));
    ASSERT_EQ(42, ((GTA_Computed_Value_Integer *)result->elements->data[2].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(result->elements->data[3].p));
    ASSERT_STREQ("hello", ((GTA_Computed_Value_String *)result->elements->data[3].p)->value->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Assign to a non-integer index.
    TEST_PROGRAM_SETUP(R"(a = [3, 4.5, true, "hello"]; a[3.5] = 42;)");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ERROR(context->result));
    ASSERT_EQ(context->result, gta_computed_value_error_invalid_index);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Assign to a non-existent index, which results in nulls being added as
    // placeholders.
    TEST_PROGRAM_SETUP(R"(a = [3, 4.5, true, "hello"]; a[6] = 42; a;)");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ARRAY(context->result));
    GTA_Computed_Value_Array * result = (GTA_Computed_Value_Array *)context->result;
    ASSERT_EQ(7, result->elements->count);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(result->elements->data[0].p));
    ASSERT_EQ(3, ((GTA_Computed_Value_Integer *)result->elements->data[0].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(result->elements->data[1].p));
    ASSERT_EQ(4.5, ((GTA_Computed_Value_Float *)result->elements->data[1].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(result->elements->data[2].p));
    ASSERT_TRUE(((GTA_Computed_Value_Boolean *)result->elements->data[2].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(result->elements->data[3].p));
    ASSERT_STREQ("hello", ((GTA_Computed_Value_String *)result->elements->data[3].p)->value->buffer);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(result->elements->data[4].p));
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(result->elements->data[5].p));
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(result->elements->data[6].p));
    ASSERT_EQ(42, ((GTA_Computed_Value_Integer *)result->elements->data[6].p)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Verify that the assignment returns the assigned value.
    TEST_PROGRAM_SETUP(R"(a = [3, 4.5, true, "hello"]; a[2] = 42;)");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result));
    ASSERT_EQ(42, ((GTA_Computed_Value_Integer *)context->result)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Verify error on invalid (negative, out-of-bounds) index.
    TEST_PROGRAM_SETUP(R"(a = [3, 4.5, true, "hello"]; a[-10] = 42;)");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ERROR(context->result));
    ASSERT_EQ(context->result, gta_computed_value_error_invalid_index);
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Assignment, MapIndex) {
  {
    // Assign to an existing key.
    TEST_PROGRAM_SETUP(R"(
      a = {
        foo: 3,
        bar: 4.5,
        baz: true,
        qux: "hello",
      };
      a["baz"] = false;
      a;)");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_MAP(context->result));
    GTA_Computed_Value_Map * result = (GTA_Computed_Value_Map *)context->result;
    ASSERT_EQ(4, result->key_hash->entries);
    ASSERT_EQ(4, result->value_hash->entries);
    {
      // foo
      const char * key = "foo";
      GTA_Integer key_hash = gcu_string_hash_64(key, strlen(key));
      GTA_HashX_Value key_result = GTA_HASHX_GET(result->key_hash, key_hash);
      ASSERT_TRUE(key_result.exists);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(GTA_TYPEX_P(key_result.value)));
      ASSERT_STREQ(key, ((GTA_Computed_Value_String *)GTA_TYPEX_P(key_result.value))->value->buffer);
      GTA_HashX_Value value_result = GTA_HASHX_GET(result->value_hash, key_hash);
      ASSERT_TRUE(value_result.exists);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(GTA_TYPEX_P(value_result.value)));
      ASSERT_EQ(3, ((GTA_Computed_Value_Integer *)GTA_TYPEX_P(value_result.value))->value);
    }
    {
      // bar
      const char * key = "bar";
      GTA_Integer key_hash = gcu_string_hash_64(key, strlen(key));
      GTA_HashX_Value key_result = GTA_HASHX_GET(result->key_hash, key_hash);
      ASSERT_TRUE(key_result.exists);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(GTA_TYPEX_P(key_result.value)));
      ASSERT_STREQ(key, ((GTA_Computed_Value_String *)GTA_TYPEX_P(key_result.value))->value->buffer);
      GTA_HashX_Value value_result = GTA_HASHX_GET(result->value_hash, key_hash);
      ASSERT_TRUE(value_result.exists);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(GTA_TYPEX_P(value_result.value)));
      ASSERT_EQ(4.5, ((GTA_Computed_Value_Float *)GTA_TYPEX_P(value_result.value))->value);
    }
    {
      // baz
      const char * key = "baz";
      GTA_Integer key_hash = gcu_string_hash_64(key, strlen(key));
      GTA_HashX_Value key_result = GTA_HASHX_GET(result->key_hash, key_hash);
      ASSERT_TRUE(key_result.exists);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(GTA_TYPEX_P(key_result.value)));
      ASSERT_STREQ(key, ((GTA_Computed_Value_String *)GTA_TYPEX_P(key_result.value))->value->buffer);
      GTA_HashX_Value value_result = GTA_HASHX_GET(result->value_hash, key_hash);
      ASSERT_TRUE(value_result.exists);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(GTA_TYPEX_P(value_result.value)));
      ASSERT_FALSE(((GTA_Computed_Value_Boolean *)GTA_TYPEX_P(value_result.value))->value);
    }
    {
      // qux
      const char * key = "qux";
      GTA_Integer key_hash = gcu_string_hash_64(key, strlen(key));
      GTA_HashX_Value key_result = GTA_HASHX_GET(result->key_hash, key_hash);
      ASSERT_TRUE(key_result.exists);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(GTA_TYPEX_P(key_result.value)));
      ASSERT_STREQ(key, ((GTA_Computed_Value_String *)GTA_TYPEX_P(key_result.value))->value->buffer);
      GTA_HashX_Value value_result = GTA_HASHX_GET(result->value_hash, key_hash);
      ASSERT_TRUE(value_result.exists);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(GTA_TYPEX_P(value_result.value)));
      ASSERT_STREQ("hello", ((GTA_Computed_Value_String *)GTA_TYPEX_P(value_result.value))->value->buffer);
    }
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Assign to a new key.
    TEST_PROGRAM_SETUP(R"(
      a = {
        foo: 3,
      };
      a["bar"] = 42;
      a;)");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_MAP(context->result));
    GTA_Computed_Value_Map * result = (GTA_Computed_Value_Map *)context->result;
    ASSERT_EQ(2, result->key_hash->entries);
    ASSERT_EQ(2, result->value_hash->entries);
    {
      // foo
      const char * key = "foo";
      GTA_Integer key_hash = gcu_string_hash_64(key, strlen(key));
      GTA_HashX_Value key_result = GTA_HASHX_GET(result->key_hash, key_hash);
      ASSERT_TRUE(key_result.exists);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(GTA_TYPEX_P(key_result.value)));
      ASSERT_STREQ(key, ((GTA_Computed_Value_String *)GTA_TYPEX_P(key_result.value))->value->buffer);
      GTA_HashX_Value value_result = GTA_HASHX_GET(result->value_hash, key_hash);
      ASSERT_TRUE(value_result.exists);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(GTA_TYPEX_P(value_result.value)));
      ASSERT_EQ(3, ((GTA_Computed_Value_Integer *)GTA_TYPEX_P(value_result.value))->value);
    }
    {
      // bar
      const char * key = "bar";
      GTA_Integer key_hash = gcu_string_hash_64(key, strlen(key));
      GTA_HashX_Value key_result = GTA_HASHX_GET(result->key_hash, key_hash);
      ASSERT_TRUE(key_result.exists);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(GTA_TYPEX_P(key_result.value)));
      ASSERT_STREQ(key, ((GTA_Computed_Value_String *)GTA_TYPEX_P(key_result.value))->value->buffer);
      GTA_HashX_Value value_result = GTA_HASHX_GET(result->value_hash, key_hash);
      ASSERT_TRUE(value_result.exists);
      ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(GTA_TYPEX_P(value_result.value)));
      ASSERT_EQ(42, ((GTA_Computed_Value_Integer *)GTA_TYPEX_P(value_result.value))->value);
    }
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Slice, Array) {
  {
    // Slice from start, no skip.
    TEST_PROGRAM_SETUP("[3, 4.5, true, \"hello\"][:2]");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ARRAY(context->result));
    GTA_Computed_Value_Array * result = (GTA_Computed_Value_Array *)context->result;
    ASSERT_EQ(2, result->elements->count);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(result->elements->data[0].p));
    ASSERT_EQ(3, ((GTA_Computed_Value_Integer *)result->elements->data[0].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(result->elements->data[1].p));
    ASSERT_EQ(4.5, ((GTA_Computed_Value_Float *)result->elements->data[1].p)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Slice from end with negative index, no end, negative skip.
    TEST_PROGRAM_SETUP("[3, 4.5, true, \"hello\"][-2::-1]");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ARRAY(context->result));
    GTA_Computed_Value_Array * result = (GTA_Computed_Value_Array *)context->result;
    ASSERT_EQ(3, result->elements->count);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(result->elements->data[0].p));
    ASSERT_TRUE(((GTA_Computed_Value_Boolean *)result->elements->data[0].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_FLOAT(result->elements->data[1].p));
    ASSERT_EQ(4.5, ((GTA_Computed_Value_Float *)result->elements->data[1].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(result->elements->data[2].p));
    ASSERT_EQ(3, ((GTA_Computed_Value_Integer *)result->elements->data[2].p)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Slice from negative index (past beginning) to end, positive skip.  The
    // start clamps to the beginning, so the walk starts at element 0 - it
    // does not keep the phase it would have had counting up from -13.
    TEST_PROGRAM_SETUP("[0,1,2,3,4,5,6,7,8,9,0][-13::3]");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ARRAY(context->result));
    GTA_Computed_Value_Array * result = (GTA_Computed_Value_Array *)context->result;
    ASSERT_EQ(4, result->elements->count);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(result->elements->data[0].p));
    ASSERT_EQ(0, ((GTA_Computed_Value_Integer *)result->elements->data[0].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(result->elements->data[1].p));
    ASSERT_EQ(3, ((GTA_Computed_Value_Integer *)result->elements->data[1].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(result->elements->data[2].p));
    ASSERT_EQ(6, ((GTA_Computed_Value_Integer *)result->elements->data[2].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(result->elements->data[3].p));
    ASSERT_EQ(9, ((GTA_Computed_Value_Integer *)result->elements->data[3].p)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Slice from index past end to beginning, negative skip.  The start
    // clamps to the last element, so the walk starts there.
    TEST_PROGRAM_SETUP("[0,1,2,3,4,5,6,7,8,9,0][19::-5]");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ARRAY(context->result));
    GTA_Computed_Value_Array * result = (GTA_Computed_Value_Array *)context->result;
    ASSERT_EQ(3, result->elements->count);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(result->elements->data[0].p));
    ASSERT_EQ(0, ((GTA_Computed_Value_Integer *)result->elements->data[0].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(result->elements->data[1].p));
    ASSERT_EQ(5, ((GTA_Computed_Value_Integer *)result->elements->data[1].p)->value);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(result->elements->data[2].p));
    ASSERT_EQ(0, ((GTA_Computed_Value_Integer *)result->elements->data[2].p)->value);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Slice with both indices out of bounds, positive skip (negative).
    TEST_PROGRAM_SETUP("[0,1,2,3,4,5,6,7,8,9,0][-20:-15]");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ARRAY(context->result));
    GTA_Computed_Value_Array * result = (GTA_Computed_Value_Array *)context->result;
    ASSERT_EQ(0, result->elements->count);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Slice with both indices out of bounds, negative skip (positive).
    TEST_PROGRAM_SETUP("[0,1,2,3,4,5,6,7,8,9,0][30:20:-1]");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ARRAY(context->result));
    GTA_Computed_Value_Array * result = (GTA_Computed_Value_Array *)context->result;
    ASSERT_EQ(0, result->elements->count);
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Slice, String) {
  {
    // Slice from start, no skip.
    TEST_PROGRAM_SETUP(R"("abcdefghijklmnopqrstuvwxyz"[:2])");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
    ASSERT_STREQ("ab", ((GTA_Computed_Value_String *)context->result)->value->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Slice from end with negative index, no end, negative skip.
    TEST_PROGRAM_SETUP(R"("abcdefghijklmnopqrstuvwxyz"[-2::-1])");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
    ASSERT_STREQ("yxwvutsrqponmlkjihgfedcba", ((GTA_Computed_Value_String *)context->result)->value->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Slice from negative index (past beginning) to end, positive skip.
    // This is 4.9's own worked example: the start clamps to the beginning,
    // so the walk starts at "a".
    TEST_PROGRAM_SETUP(R"("abcdefghijklmnopqrstuvwxyz"[-34::3])");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
    ASSERT_STREQ("adgjmpsvy", ((GTA_Computed_Value_String *)context->result)->value->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Slice from index past end to beginning, negative skip.
    TEST_PROGRAM_SETUP(R"("abcdefghijklmnopqrstuvwxyz"[33::-5])");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
    ASSERT_STREQ("zupkfa", ((GTA_Computed_Value_String *)context->result)->value->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Slice with both indices out of bounds, positive skip (negative).
    TEST_PROGRAM_SETUP(R"("abcdefghijklmnopqrstuvwxyz"[-40:-35])");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
    ASSERT_STREQ("", ((GTA_Computed_Value_String *)context->result)->value->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Slice with both indices out of bounds, negative skip (positive).
    TEST_PROGRAM_SETUP(R"("abcdefghijklmnopqrstuvwxyz"[40:30:-1])");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result));
    ASSERT_STREQ("", ((GTA_Computed_Value_String *)context->result)->value->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Print, Simple) {
  {
    // Print an integer.
    TEST_PROGRAM_SETUP("print(42);");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    ASSERT_STREQ("42", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Print a float.
    TEST_PROGRAM_SETUP("print(3.5);");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    ASSERT_STREQ("3.5", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Print a string.
    TEST_PROGRAM_SETUP("print(\"hello\");");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    ASSERT_STREQ("hello", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Print a null (should print nothing).
    TEST_PROGRAM_SETUP("print(null);");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    ASSERT_STREQ("", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Multiple prints.
    TEST_PROGRAM_SETUP("print(42); print(3.5); print(\"hello\"); print(null); print(-42);");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    ASSERT_STREQ("423.5hello-42", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Map, NestedLiteral) {
  // A map literal whose value is another map literal used to segfault under
  // the JIT while it was being built, printed or not.
  //
  // The map compiler saved the key across the value expression with a single
  // push, which left rsp eight bytes out for everything the value compiled
  // to. A literal value survived that; a nested map did not, because
  // building one reaches gcu_string_hash_64(), whose murmur3 state is a
  // stack local the compiler loads with movdqa - an aligned move, which
  // faults on a stack that is not.
  //
  // This file runs twice, once with the JIT disabled and once with the
  // bytecode disabled, so these cover the path that crashed as well as the
  // one that never did.
  {
    TEST_PROGRAM_SETUP("print({a: {b: 1}});");
    ASSERT_TRUE(context->result);
    ASSERT_STREQ("{\"a\": {\"b\": 1}}", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Nested two deep.
    TEST_PROGRAM_SETUP("print({a: {b: {c: 2}}});");
    ASSERT_TRUE(context->result);
    ASSERT_STREQ("{\"a\": {\"b\": {\"c\": 2}}}", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Reached through an array, which also crashed.
    TEST_PROGRAM_SETUP("print({a: [1, {b: 2}]});");
    ASSERT_TRUE(context->result);
    ASSERT_STREQ("{\"a\": [1, {\"b\": 2}]}", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // An empty nested map never crashed - it adds no key and so never
    // hashes one - which is what made the first report look like it was
    // about maps rather than about the stack.
    TEST_PROGRAM_SETUP("print({a: {:}});");
    ASSERT_TRUE(context->result);
    ASSERT_STREQ("{\"a\": {}}", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A nested map that is not the first entry, so that the pairs before it
    // have already pushed and popped.
    TEST_PROGRAM_SETUP("m = {a: 1, b: {c: 2}, d: 3}; print(m[\"b\"]);");
    ASSERT_TRUE(context->result);
    ASSERT_STREQ("{\"c\": 2}", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Two nested maps in one literal.
    TEST_PROGRAM_SETUP("print({a: {b: 1}, c: {d: 2}});");
    ASSERT_TRUE(context->result);
    std::string out{context->output->buffer};
    ASSERT_TRUE(out == "{\"a\": {\"b\": 1}, \"c\": {\"d\": 2}}"
      || out == "{\"c\": {\"d\": 2}, \"a\": {\"b\": 1}}") << out;
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // The value is still reachable afterwards, so the pair really was
    // stored rather than merely surviving construction.
    TEST_PROGRAM_SETUP("m = {a: {b: 42}}; print(m[\"a\"][\"b\"]);");
    ASSERT_TRUE(context->result);
    ASSERT_STREQ("42", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Map, ValueThatCallsOut) {
  // While a map literal's value is being computed, the map and the key are
  // saved on the stack with push. Under the Windows x64 ABI every callee owns
  // the 32 bytes just above its return address and may overwrite them, so a
  // value expression that calls a C function - which most do - used to have
  // that callee write over the saved map and key: the map came out empty or
  // the program crashed. Worse, the binary-operator, print and function-call
  // compilers stored their own temporaries 32 bytes above the space they had
  // reserved, on the assumption that nothing else was there, and a pushed
  // map or key is exactly what was there.
  //
  // None of these can fold to a constant: each operand is a variable.
  // Under System V (no shadow space) all of these already worked; they pin
  // down that the stack discipline is the same on both.
  {
    // A binary operator: a five-argument call, the fifth on the stack under
    // the Windows ABI.
    TEST_PROGRAM_SETUP("x = 1; print({a: x + 2});");
    ASSERT_STREQ("{\"a\": 3}", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A unary operator.
    TEST_PROGRAM_SETUP("x = 3; print({a: -x});");
    ASSERT_STREQ("{\"a\": -3}", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A slice, whose callee takes a fifth argument on the stack under the
    // Windows ABI.
    TEST_PROGRAM_SETUP("s = [1, 2, 3]; print({a: s[0:2]});");
    ASSERT_STREQ("{\"a\": [1, 2]}", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A comparison, whose result is a singleton rather than a new value.
    TEST_PROGRAM_SETUP("x = 1; print({a: x < 2});");
    ASSERT_STREQ("{\"a\": true}", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A call to a Tang function, with an argument.
    TEST_PROGRAM_SETUP("function f(n) { return n + 1; } print({a: f(1)});");
    ASSERT_STREQ("{\"a\": 2}", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // An array literal of binary operators, nested in a map, nested in an
    // array.
    TEST_PROGRAM_SETUP("x = 1; print([x, {a: [x + 1, x + 2]}]);");
    ASSERT_STREQ("[1, {\"a\": [2, 3]}]", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Print, Map) {
  {
    // The empty map.
    TEST_PROGRAM_SETUP("print({:});");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    ASSERT_STREQ("{}", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A single entry.  The key is quoted; the value is rendered the way
    // print would show it, as the array does with its elements.
    TEST_PROGRAM_SETUP("print({a: 1});");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    ASSERT_STREQ("{\"a\": 1}", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A nested array renders recursively.
    TEST_PROGRAM_SETUP("print({k: [1, 2]});");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    ASSERT_STREQ("{\"k\": [1, 2]}", context->output->buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Two entries.  Iteration order is the hash table's, so either order is
    // correct; what matters is the separator between them and the absence of
    // one before the closing brace.
    TEST_PROGRAM_SETUP("print({a: 1, b: 2});");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    std::string out{context->output->buffer};
    ASSERT_TRUE(out == "{\"a\": 1, \"b\": 2}" || out == "{\"b\": 2, \"a\": 1}") << out;
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Enough entries to grow the buffer several times over.  The hand
    // written builder this replaced never wrote the reallocated pointer
    // back, so the first growth left the caller holding a freed block and
    // every entry was written over the one before it; a single short map
    // never grew and so never showed it.
    TEST_PROGRAM_SETUP(
      "print({k00: 0, k01: 1, k02: 2, k03: 3, k04: 4, k05: 5, k06: 6,"
      " k07: 7, k08: 8, k09: 9, k10: 10, k11: 11, k12: 12, k13: 13,"
      " k14: 14, k15: 15, k16: 16, k17: 17, k18: 18, k19: 19});");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    std::string out{context->output->buffer};

    ASSERT_EQ('{', out.front());
    ASSERT_EQ('}', out.back());
    ASSERT_EQ(std::string::npos, out.find(",}"));
    ASSERT_EQ(19u, std::count(out.begin(), out.end(), ','));
    for (int i = 0; i < 20; ++i) {
      char entry[32];
      snprintf(entry, sizeof(entry), "\"k%02d\": %d", i, i);
      ASSERT_NE(std::string::npos, out.find(entry)) << entry << " missing from " << out;
    }
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Concatenate, Strings) {
  // `+` on two strings was `Not implemented` at run time while the AST
  // simplifier already folded it with gta_unicode_string_concat, so a
  // constant-folded program and an executed one disagreed about the same
  // source.
  {
    TEST_PROGRAM_SETUP(R"(print("ab" + "cd");)");
    ASSERT_STREQ(context->output->buffer, "abcd");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Through variables, so nothing can be folded on the way.
    TEST_PROGRAM_SETUP(R"(a = "ab"; b = "cd"; print(a + b);)");
    ASSERT_STREQ(context->output->buffer, "abcd");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Left-associative chains, and the empty string as either operand.
    TEST_PROGRAM_SETUP(R"(print("a" + "b" + "c"); print(","); print("" + "x"); print(","); print("x" + "");)");
    ASSERT_STREQ(context->output->buffer, "abc,x,x");
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(Concatenate, NonStrings) {
  // The other operand is converted exactly as printing it would convert it,
  // so `"a" + b` and `print("a"); print(b);` agree by construction.
  {
    TEST_PROGRAM_SETUP(R"(print("count: " + 5);)");
    ASSERT_STREQ(context->output->buffer, "count: 5");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // The string on the right. The integer vtable declines a string operand,
    // and the dispatcher retries the other way round, so this exercises the
    // self_is_lhs == false path and proves the operands are not swapped.
    TEST_PROGRAM_SETUP(R"(print(5 + " apples");)");
    ASSERT_STREQ(context->output->buffer, "5 apples");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    TEST_PROGRAM_SETUP(R"(print("pi=" + 3.5); print(","); print("b=" + true); print(","); print("a=" + [1, 2]);)");
    ASSERT_STREQ(context->output->buffer, "pi=3.5,b=true,a=[1, 2]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Identical to printing the two pieces in sequence.
    TEST_PROGRAM_SETUP(R"(print("n=" + 7); print("|"); print("n="); print(7);)");
    ASSERT_STREQ(context->output->buffer, "n=7|n=7");
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(Concatenate, KeepsEachHalfsEncoding) {
  // The reason concatenation goes through GTA_Unicode_String rather than bytes:
  // the result is multi-segment and each segment keeps its own tag, so a
  // trusted literal joined to an untrusted one is still escaped on exactly the
  // untrusted half. Collapsing to one tag would either emit user input
  // unescaped or double-escape the markup around it.
  //
  // context->output->buffer is the raw buffer, so the check has to render.
  {
    TEST_PROGRAM_SETUP(R"(print("<b>" + !"<i>");)");
    GTA_Unicode_Rendered_String rendered = gta_unicode_string_render(context->output);
    ASSERT_TRUE(rendered.buffer);
    ASSERT_STREQ(rendered.buffer, "<b>&lt;i&gt;");
    gcu_free(rendered.buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // The untrusted half first: it must not inherit the trusted tag of the
    // half after it.
    TEST_PROGRAM_SETUP(R"(print(!"<i>" + "<b>");)");
    GTA_Unicode_Rendered_String rendered = gta_unicode_string_render(context->output);
    ASSERT_TRUE(rendered.buffer);
    ASSERT_STREQ(rendered.buffer, "&lt;i&gt;<b>");
    gcu_free(rendered.buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Concatenating is the same as printing the pieces one at a time.
    TEST_PROGRAM_SETUP(R"(print("<b>"); print(!"<i>");)");
    GTA_Unicode_Rendered_String rendered = gta_unicode_string_render(context->output);
    ASSERT_TRUE(rendered.buffer);
    ASSERT_STREQ(rendered.buffer, "<b>&lt;i&gt;");
    gcu_free(rendered.buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // And a slice across the join keeps the boundary where it was.
    TEST_PROGRAM_SETUP(R"(print(("<x>" + !"<i>")[1:5]);)");
    GTA_Unicode_Rendered_String rendered = gta_unicode_string_render(context->output);
    ASSERT_TRUE(rendered.buffer);
    ASSERT_STREQ(rendered.buffer, "x>&lt;i");
    gcu_free(rendered.buffer);
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(Concatenate, ErrorsAndUnprintableValues) {
  {
    // An error is the answer, not something to paste into the text. Rendering
    // it would make the result an ordinary string, and everything downstream
    // would then succeed on prose that nothing can tell apart from output the
    // author meant.
    TEST_PROGRAM_SETUP(R"("big: " + (9223372036854775807 + 1);)");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ERROR(context->result));
    ASSERT_EQ(context->result, gta_computed_value_error_integer_too_large);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // The error on the left reaches the same place by the reversed call.
    TEST_PROGRAM_SETUP(R"((9223372036854775807 + 1) + " big";)");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ERROR(context->result));
    ASSERT_EQ(context->result, gta_computed_value_error_integer_too_large);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Printing the whole expression therefore shows the error rather than a
    // sentence with the error's text embedded in it.
    TEST_PROGRAM_SETUP(R"(print("big: " + (9223372036854775807 + 1));)");
    ASSERT_STREQ(context->output->buffer, "[INTEGER TOO LARGE]");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A value that cannot be printed cannot be concatenated either, and says
    // so rather than contributing nothing to the text.
    TEST_PROGRAM_SETUP(R"("x" + null;)");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ERROR(context->result));
    ASSERT_EQ(context->result, gta_computed_value_error_not_supported);
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(CompoundAssign, Arithmetic) {
  // `a += 1` did not parse at all - there was no token for it, so the scanner
  // returned `+` then `=` and no rule accepted the pair. It is built into the
  // tree for `a = a + 1`, so the arithmetic, the overflow reporting and the
  // concatenation all come from operators that already exist.
  {
    TEST_PROGRAM_SETUP(R"(a = 1; a += 2; print(a);)");
    ASSERT_STREQ(context->output->buffer, "3");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    TEST_PROGRAM_SETUP(R"(
      a = 10; a -= 3; print(a); print(",");
      b = 10; b *= 3; print(b); print(",");
      c = 10; c /= 3; print(c); print(",");
      d = 10; d %= 3; print(d);
    )");
    ASSERT_STREQ(context->output->buffer, "7,30,3,1");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // The right-hand side is a whole expression, not just the next operand:
    // `a += 2 * 3` is `a = a + (2 * 3)`, not `(a + 2) * 3`.
    TEST_PROGRAM_SETUP(R"(a = 1; a += 2 * 3; print(a); print(","); b = 2; b *= 1 + 3; print(b);)");
    ASSERT_STREQ(context->output->buffer, "7,8");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Right-associative, like plain assignment.
    TEST_PROGRAM_SETUP(R"(a = 1; b = 1; a += b += 1; print(a); print(","); print(b);)");
    ASSERT_STREQ(context->output->buffer, "3,2");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // The shape the self-assignment bug used to corrupt: a compound assignment
    // is exactly that shape, so it is worth asserting a later read is clean.
    TEST_PROGRAM_SETUP(R"(a = 1; a += 2; b = a + 3; print(a); print(","); print(b);)");
    ASSERT_STREQ(context->output->buffer, "3,6");
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(CompoundAssign, StringsAndLoops) {
  {
    // The accumulator in a loop, which is what `+=` is mostly for.
    TEST_PROGRAM_SETUP(R"(s = ""; for (i = 0; i < 3; i += 1) { s += "x"; } print(s);)");
    ASSERT_STREQ(context->output->buffer, "xxx");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    TEST_PROGRAM_SETUP(R"(a = "q"; a += "r"; print(a); print(","); b = "n="; b += 5; print(b);)");
    ASSERT_STREQ(context->output->buffer, "qr,n=5");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Concatenating through `+=` keeps each half's tag, the same as `+` does -
    // it is the same node underneath.
    TEST_PROGRAM_SETUP(R"(a = "<b>"; a += !"<i>"; print(a);)");
    GTA_Unicode_Rendered_String rendered = gta_unicode_string_render(context->output);
    ASSERT_TRUE(rendered.buffer);
    ASSERT_STREQ(rendered.buffer, "<b>&lt;i&gt;");
    gcu_free(rendered.buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // `global` inside a function, which is the only place `global` is legal.
    TEST_PROGRAM_SETUP(R"(n = 1; function f() { global n; n += 4; } f(); print(n);)");
    ASSERT_STREQ(context->output->buffer, "5");
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(CompoundAssign, ErrorsPropagate) {
  {
    // Overflow is reported by the underlying `+`, so it reaches the variable
    // as a marker rather than wrapping.
    TEST_PROGRAM_SETUP(R"(a = 9223372036854775807; a += 1; a;)");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ERROR(context->result));
    ASSERT_EQ(context->result, gta_computed_value_error_integer_too_large);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    TEST_PROGRAM_SETUP(R"(a = 5; a /= 0; a;)");
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_ERROR(context->result));
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(CompoundAssign, FailingParseDoesNotLeakTheName) {
  // Found by the parse fuzzer within 150 seconds of the operators existing.
  //
  // The rule's action runs, and then VERIFY has to discard what the rule was
  // handed. The first version discarded only the expression, so the identifier
  // token's string - which the token owns until the action adopts it - leaked
  // whenever the expression was missing. An invalid UTF-8 byte in a string
  // literal is one way to make it missing: the string arm fails, yields a null
  // node, and the compound-assignment arm still reduces.
  //
  // Two bytes per occurrence, so nothing visible without a leak checker. Note
  // that `W = "\x9f";` never leaked - the plain assignment rule already used
  // the two-value form, and the bug was introduced by not following it.
  for (const char * src : {
      "W += \"\x9f\";",
      "W -= \"\x9f\";",
      "W *= \"\x9f\";",
      "W /= \"\x9f\";",
      "W %= \"\x9f\";",
      "longer_name += \"\x9f\";",
      }) {
    gcu_memory_reset_counts();
    GTA_Program * program = gta_program_create(language, src);
    ASSERT_FALSE(program) << "accepted: " << src;
    ASSERT_EQ(gcu_get_alloc_count(), gcu_get_free_count()) << "leaked on: " << src;
  }
}


TEST(CompoundAssign, OnlyAnIdentifierCanBeTheTarget) {
  // `a[i] += b` would have to evaluate the target twice to be desugared - there
  // is no AST deep copy - and `a[f()] += 1` calling f() twice is a worse
  // outcome than the form not existing. It is refused at parse time.
  for (const char * src : {
      "x = [1, 2]; x[0] += 5;",
      "m = [\"k\": 1]; m[\"k\"] += 5;",
      "x = [1, 2]; x[0] -= 5;",
      "1 += 2;",
      "a = 1; a += ;",
      }) {
    gcu_memory_reset_counts();
    GTA_Program * program = gta_program_create(language, src);
    ASSERT_FALSE(program) << "accepted: " << src;
    ASSERT_EQ(gcu_get_alloc_count(), gcu_get_free_count()) << "leaked on: " << src;
  }
  {
    // The spelled-out form is still accepted, and is the workaround.
    TEST_PROGRAM_SETUP(R"(x = [1, 2]; x[0] = x[0] + 5; print(x[0]);)");
    ASSERT_STREQ(context->output->buffer, "6");
    TEST_PROGRAM_TEARDOWN();
  }
}



// A construct whose compile refuses must fail the whole program, not publish
// the part of the bytecode that was emitted before it.  Publishing the partial
// vector left the program without its RETURN, and the virtual machine then ran
// off the end of the instructions into the vector's spare capacity, where a
// zero reads as RETURN - so the program silently stopped at the refused
// statement and reported success.
//
// Each case below puts a statement after the construct.  Whether the construct
// itself is supported is not what is being tested and will change; the
// invariant is that a program which compiles runs all the way to its last
// statement.
TEST(Syntax, ARefusedStatementDoesNotTruncateTheProgram) {
  const char * cases[] = {
    "x = [1, 2]; x.size = 5; y = 99; y;",
    "x = [1, 2, 3]; x[1:2] = [9]; y = 99; y;",
    "x = [1, 2, 3]; x[1:] = 7; y = 99; y;",
    "m = {:}; m.b = 2; y = 99; y;",
    "f = 1; f.g.h = 2; y = 99; y;",
    "s = \"abc\"; s.length = 1; y = 99; y;",
  };
  for (const char * code : cases) {
    gcu_memory_reset_counts();
    GTA_Program * program = gta_program_create(language, code);
    if (!program) {
      // Refused outright, which is the other acceptable answer.
      ASSERT_EQ(gcu_get_alloc_count(), gcu_get_free_count()) << code;
      continue;
    }
    GTA_Execution_Context * context = gta_execution_context_create(program);
    ASSERT_TRUE(context) << code;
    ASSERT_TRUE(gta_program_execute(context)) << code;
    ASSERT_TRUE(context->result) << code;
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result)) << code;
    ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, 99) << code;
    gta_execution_context_destroy(context);
    gta_program_destroy(program);
  }
}



// Run a source that must yield a boolean, and say which boolean.
static void expect_boolean(const char * code, bool expected) {
  gcu_memory_reset_counts();
  GTA_Program * program = gta_program_create(language, code);
  ASSERT_TRUE(program) << code;
  GTA_Execution_Context * context = gta_execution_context_create(program);
  ASSERT_TRUE(context) << code;
  ASSERT_TRUE(gta_program_execute(context)) << code;
  ASSERT_TRUE(context->result) << code;
  ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_BOOLEAN(context->result)) << code;
  ASSERT_EQ(((GTA_Computed_Value_Boolean *)context->result)->value, expected) << code;
  gta_execution_context_destroy(context);
  gta_program_destroy(program);
}


// Equality used to be implemented only for numbers and arrays: every other
// type's vtable carried the "not implemented" stub, so `"a" == "a"` was an
// error rather than true.
TEST(Binary, EqualityOfStringsBooleansAndNull) {
  expect_boolean("\"a\" == \"a\";", true);
  expect_boolean("\"a\" == \"b\";", false);
  expect_boolean("\"a\" != \"b\";", true);
  expect_boolean("\"a\" != \"a\";", false);
  expect_boolean("\"\" == \"\";", true);
  expect_boolean("\"abc\" == \"ab\";", false);
  expect_boolean("true == true;", true);
  expect_boolean("true == false;", false);
  expect_boolean("false != true;", true);
  expect_boolean("null == null;", true);
  expect_boolean("null != null;", false);
  // A concatenated string is stored in segments; equality is on content, so
  // how the string came to exist must not show through.
  expect_boolean("(\"a\" + \"b\") == \"ab\";", true);
  expect_boolean("\"ab\" == (\"a\" + \"b\");", true);
}


// Equality is strict (section 14): a value of one type is never equal to a
// value of another, and asking is not an error - a template asks `x == null`
// to find out whether a value is there at all.
TEST(Binary, EqualityAcrossTypesIsFalseNotAnError) {
  expect_boolean("\"a\" == 3;", false);
  expect_boolean("3 == \"a\";", false);
  expect_boolean("\"3\" == 3;", false);
  expect_boolean("true == 1;", false);
  expect_boolean("1 == true;", false);
  expect_boolean("null == 0;", false);
  expect_boolean("null == false;", false);
  expect_boolean("null == \"\";", false);
  expect_boolean("[1] == \"a\";", false);
  expect_boolean("\"a\" != 3;", true);
  expect_boolean("null != 0;", true);
}


// Ordering strings is by code point, which for UTF-8 is byte order.  Unlike
// equality, ordering across types has no answer and stays an error.
TEST(Binary, OrderingStrings) {
  expect_boolean("\"a\" < \"b\";", true);
  expect_boolean("\"b\" < \"a\";", false);
  expect_boolean("\"a\" < \"a\";", false);
  expect_boolean("\"a\" <= \"a\";", true);
  expect_boolean("\"b\" > \"a\";", true);
  expect_boolean("\"a\" >= \"b\";", false);
  // A prefix sorts before the longer string that contains it.
  expect_boolean("\"a\" < \"ab\";", true);
  expect_boolean("\"ab\" > \"a\";", true);
  expect_boolean("\"\" < \"a\";", true);
  // Beyond ASCII, where a signed char comparison would have gone wrong.
  expect_boolean("\"é\" > \"e\";", true);
  expect_boolean("\"é\" < \"ê\";", true);
}


TEST(Binary, OrderingAcrossTypesIsAnError) {
  const char * cases[] = {
    "\"a\" < 3;",
    "3 < \"a\";",
    "\"a\" >= null;",
    "true < false;",
  };
  for (const char * code : cases) {
    gcu_memory_reset_counts();
    GTA_Program * program = gta_program_create(language, code);
    ASSERT_TRUE(program) << code;
    GTA_Execution_Context * context = gta_execution_context_create(program);
    ASSERT_TRUE(context) << code;
    ASSERT_TRUE(gta_program_execute(context)) << code;
    ASSERT_TRUE(context->result) << code;
    ASSERT_TRUE(context->result->is_error) << code;
    gta_execution_context_destroy(context);
    gta_program_destroy(program);
  }
}



// Run a source that must yield an integer, and say which integer.
static void expect_integer(const char * code, GTA_Integer expected) {
  gcu_memory_reset_counts();
  GTA_Program * program = gta_program_create(language, code);
  ASSERT_TRUE(program) << code;
  GTA_Execution_Context * context = gta_execution_context_create(program);
  ASSERT_TRUE(context) << code;
  ASSERT_TRUE(gta_program_execute(context)) << code;
  ASSERT_TRUE(context->result) << code;
  ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_INTEGER(context->result)) << code;
  ASSERT_EQ(((GTA_Computed_Value_Integer *)context->result)->value, expected) << code;
  gta_execution_context_destroy(context);
  gta_program_destroy(program);
}


// Arrays and maps used to be false whatever they held, because is_true was
// set to false when the container was created and nothing ever set it again.
// Both engines read that byte straight out of the object, so `if (items)` -
// the first thing a template does with a list - always took the else branch.
TEST(Truthiness, ContainersAreTrueWhenTheyHoldSomething) {
  expect_integer("if ([1]) { 1; } else { 2; }", 1);
  expect_integer("if ([]) { 1; } else { 2; }", 2);
  expect_integer("if ([0]) { 1; } else { 2; }", 1);
  expect_integer("if ({a: 1}) { 1; } else { 2; }", 1);
  expect_integer("if ({:}) { 1; } else { 2; }", 2);
  expect_boolean("![1];", false);
  expect_boolean("![];", true);
  expect_boolean("!{a: 1};", false);
  expect_boolean("!{:};", true);
  expect_integer("[1] && 2;", 2);
  expect_integer("[] || 2;", 2);
  expect_integer("x = [1, 2]; if (x) { 1; } else { 2; }", 1);
}


// The flag has to keep up with the container, not just with how it was built:
// a container that starts empty and is filled afterwards becomes true, and
// one produced empty by an operation stays false.
TEST(Truthiness, ContainersTrackWhatTheyHold) {
  expect_integer("x = []; x[0] = 1; if (x) { 1; } else { 2; }", 1);
  expect_integer("m = {:}; m[\"k\"] = 1; if (m) { 1; } else { 2; }", 1);
  expect_integer("x = [1, 2]; y = x[0:0]; if (y) { 1; } else { 2; }", 2);
  expect_integer("x = [1, 2]; y = x[0:1]; if (y) { 1; } else { 2; }", 1);
  expect_integer("x = [1] + []; if (x) { 1; } else { 2; }", 1);
  expect_integer("x = [1] * 0; if (x) { 1; } else { 2; }", 2);
  expect_integer("x = [] + []; if (x) { 1; } else { 2; }", 2);
}



// Run a source that must yield an error, and say which message.
static void expect_error(const char * code, const char * message) {
  gcu_memory_reset_counts();
  GTA_Program * program = gta_program_create(language, code);
  ASSERT_TRUE(program) << code;
  GTA_Execution_Context * context = gta_execution_context_create(program);
  ASSERT_TRUE(context) << code;
  ASSERT_TRUE(gta_program_execute(context)) << code;
  ASSERT_TRUE(context->result) << code;
  ASSERT_TRUE(context->result->is_error) << code;
  char * rendered = gta_computed_value_to_string(context->result);
  ASSERT_TRUE(rendered) << code;
  ASSERT_STREQ(rendered, message) << code;
  gcu_free(rendered);
  gta_execution_context_destroy(context);
  gta_program_destroy(program);
}


// `m.name = v` wrote a member all along, but `m.name` read through the
// generic attribute lookup, which a map has no entries in, so every read was
// "not implemented" - a map could be written and not read back by the
// spelling the reference documents.
TEST(Period, ReadsAMapMember) {
  expect_integer("m = {a: 1}; m.a;", 1);
  expect_integer("m = {a: 1, b: 2}; m.b;", 2);
  expect_integer("m = {:}; m.b = 2; m.b;", 2);
  expect_integer("m = {a: {b: 2}}; m.a.b;", 2);
  expect_boolean("m = {a: 1}; m.a == m[\"a\"];", true);
  // What a name the map does not hold answers is in Period.AMissingMemberIsNull,
  // where the subscript form it has to agree with is asserted beside it.
}


// Attribute assignment is for map members (section 4.13).  It used to refuse
// to compile, which - before the caller checked for that - truncated the
// program.  It now compiles to the same thing `a["b"] = v` does, so each type
// answers for itself, and only a map accepts it.
TEST(Assignment, AttributeWritesAMapMember) {
  expect_integer("m = {:}; m.b = 2; m[\"b\"];", 2);
  expect_integer("m = {a: 1}; m.a = 9; m.a;", 9);
  // The value of the assignment is the value assigned.
  expect_integer("m = {:}; m.b = 2;", 2);
}


// An attribute assignment on anything but a map is an error, and the value it
// was meant to shadow is untouched.  `x.size = 5` used to end the program,
// which looked like it had rebound x (13.10).
TEST(Assignment, AttributeOnANonMapIsAnError) {
  expect_error("x = [1, 2]; x.size = 5;", "Error: Invalid index");
  expect_error("s = \"abc\"; s.length = 1;", "Error: Not supported");
  expect_error("f = 1; f.g = 2;", "Error: Not supported");
  expect_error("null.x = 1;", "Error: Not supported");
  expect_error("use math; math.pi = 3;", "Error: Not supported");
  // The built-in keeps its meaning, and the program keeps running.
  expect_integer("x = [1, 2]; x.size = 5; x.size;", 2);
  expect_integer("s = \"abc\"; s.length = 1; s.length;", 3);
  expect_integer("x = [1, 2]; x.size = 5; y = 99; y;", 99);
}


// Only a name, a member or a subscript can be assigned to.  Assigning to a
// slice used to look like it rebound the variable (13.11); it was the program
// ending there.
TEST(Assignment, OnlyANameMemberOrSubscriptCanBeAssignedTo) {
  const char * cases[] = {
    "x = [1, 2, 3]; x[1:2] = [9]; x;",
    "x = [1, 2, 3]; x[1:] = 7; x;",
    "s = \"abc\"; s[0:1] = \"z\"; s;",
  };
  for (const char * code : cases) {
    gcu_memory_reset_counts();
    GTA_Program * program = gta_program_create(language, code);
    ASSERT_FALSE(program) << code;
    ASSERT_EQ(gcu_get_alloc_count(), gcu_get_free_count()) << code;
  }
}



// Run a source that must yield null.
static void expect_null(const char * code) {
  gcu_memory_reset_counts();
  GTA_Program * program = gta_program_create(language, code);
  ASSERT_TRUE(program) << code;
  GTA_Execution_Context * context = gta_execution_context_create(program);
  ASSERT_TRUE(context) << code;
  ASSERT_TRUE(gta_program_execute(context)) << code;
  ASSERT_TRUE(context->result) << code;
  ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result)) << code;
  gta_execution_context_destroy(context);
  gta_program_destroy(program);
}


// A ranged for that runs to the end of its collection used to leave the
// iterator's end sentinel behind as its value, so a template ending with a
// loop - which is most of them - produced the error `Iterator end`.
TEST(RangedFor, RunningOutIsNullNotAnError) {
  expect_null("for (i : [1, 2]) {}");
  expect_null("for (i : []) {}");
  expect_null("for (i : [1, 2]) { i; }");
  expect_null("for (i : [1, 2]) { continue; }");
  expect_null("for (i : [1, 2]) { for (j : [3]) {} }");
  expect_integer("x = 0; for (i : [1, 2]) { x = x + i; } x;", 3);
}


// Breaking out of the loop is the loop's value, and it was already null;
// the two exits must not have been merged into one.
TEST(RangedFor, BreakKeepsItsOwnValue) {
  expect_null("for (i : [1, 2]) { break; }");
  expect_null("for (i : [1, 2]) { if (i == 2) { break; } }");
}


// The other way out of the loop is that the expression could not be iterated
// at all.  That is an error and has to stay one - it is the only way the
// author hears about it.
TEST(RangedFor, ANonIterableExpressionIsStillAnError) {
  expect_error("for (i : 5) {}", "Error: Not supported");
  expect_error("for (i : {a: 1}) {}", "Error: Not supported");
  expect_error("for (i : \"abc\") {}", "Error: Not implemented");
}



// Run a source that must yield a string, and say which string.
static void expect_string(const char * code, const char * expected) {
  gcu_memory_reset_counts();
  GTA_Program * program = gta_program_create(language, code);
  ASSERT_TRUE(program) << code;
  GTA_Execution_Context * context = gta_execution_context_create(program);
  ASSERT_TRUE(context) << code;
  ASSERT_TRUE(gta_program_execute(context)) << code;
  ASSERT_TRUE(context->result) << code;
  ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_STRING(context->result)) << code;
  ASSERT_STREQ(((GTA_Computed_Value_String *)context->result)->value->buffer, expected) << code;
  gta_execution_context_destroy(context);
  gta_program_destroy(program);
}


// Run a source for what it prints, which is the only way to count how many
// times a sub-expression ran.
static void expect_output(const char * code, const char * expected) {
  gcu_memory_reset_counts();
  GTA_Program * program = gta_program_create(language, code);
  ASSERT_TRUE(program) << code;
  GTA_Execution_Context * context = gta_execution_context_create(program);
  ASSERT_TRUE(context) << code;
  ASSERT_TRUE(gta_program_execute(context)) << code;
  ASSERT_TRUE(context->output) << code;
  ASSERT_STREQ(context->output->buffer, expected) << code;
  gta_execution_context_destroy(context);
  gta_program_destroy(program);
}


// The bytecode engine compiled an assignment's right-hand side before it knew
// which form of assignment it was, and the index and member forms then
// compiled it again.  Nothing in the result gave it away - the second value is
// the one that gets stored, and it equals the first - so it took an effect to
// see: `a[0] = print(1);` printed "11", and any call on the right-hand side
// was made twice.  The x86-64 engine has compiled it once per form since
// f369720; this is that commit's other half.
TEST(Assignment, TheRightHandSideRunsOnce) {
  expect_output("a = []; a[0] = print(1); 0;", "1");
  expect_output("m = {:}; m.k = print(1); 0;", "1");
  expect_output("m = {:}; m[\"k\"] = print(1); 0;", "1");
  // A plain name never had the defect, and must not acquire one.
  expect_output("a = print(1); 0;", "1");
  // Two of them, so that a fix which merely moved the duplication shows.
  expect_output("a = []; a[0] = print(1); a[1] = print(2); 0;", "12");
  // The value stored is still the value of the expression.
  expect_integer("a = []; a[0] = 7; a[0];", 7);
  expect_integer("a = []; (a[0] = 7);", 7);
}


// `array * integer` repeats the array.  The result was created with room for
// every element and the elements were written into that room, but the vector's
// count was never set - so `[1, 2] * 3` was `[]`, an array with six elements
// in it that said it held none.  The count of elements was also taken as an
// unchecked product, so a large repetition wrapped and asked the allocator for
// most of the address space.
TEST(Binary, ArrayRepetition) {
  expect_string("([1, 2] * 3) as string;", "[1, 2, 1, 2, 1, 2]");
  expect_string("([1] * 1) as string;", "[1]");
  expect_boolean("([1, 2] * 2) == [1, 2, 1, 2];", true);
  expect_string("([1, 2] * 0) as string;", "[]");
  expect_string("([] * 5) as string;", "[]");
  // The elements are copies, so mutating the source does not reach them.
  expect_integer("x = [[1]]; y = (x * 2); x[0][0] = 9; (y[0][0] + y[1][0]);", 2);
  // Only array-times-integer, in that order (4.2).
  expect_error("[1] * -1;", "Error: Not supported");
  expect_error("3 * [1, 2];", "Error: Not supported");
  // A count that cannot be allocated is an error, not a wrapped product.
  expect_error("[1, 2] * 9223372036854775807;", "Error: Out of memory");
  // Repeating an empty array answers without counting to the repetition.
  expect_string("([] * 9223372036854775807) as string;", "[]");
}


// `&&` and `||` are compiled to a conditional jump that does not pop what it
// tested, because when the left operand decides the answer it *is* the answer.
// The other branch has to pop it, and did not, so an expression that is only
// allowed to leave one value left two.  The value read back was still right -
// it is the top of the stack either way - so this was invisible to anything
// that only looked at the result.  It was not invisible to anything that
// counts down from the top of the stack to find its operands.
TEST(Binary, ShortCircuitLeavesOneValue) {
  // The value itself, in all four branches (4.5).
  expect_integer("(true && 2);", 2);
  expect_boolean("(false && 2);", false);
  expect_boolean("(true || 2);", true);
  expect_integer("(false || 2);", 2);
  expect_integer("3 && 4;", 4);
  expect_string("0 || \"x\";", "x");
  expect_integer("null || 5;", 5);

  // An array literal takes its elements from the top of the stack downward, so
  // a stray value pushes the earlier elements out of reach.
  expect_string("x = [9, (true && 2)]; x as string;", "[9, 2]");
  expect_string("x = [9, (false || 2)]; x as string;", "[9, 2]");
  expect_string("x = [(true && 1), (true && 2)]; x as string;", "[1, 2]");

  // A call takes its arguments the same way.
  expect_integer("function f(p, q) { return p; } f(9, (true && 2));", 9);
  expect_integer("function f(p, q) { return q; } f((true && 2), 9);", 9);

  // A map literal takes key and value pairs, so a stray value is read as a
  // key - and a key that is not a string had its `value` field dereferenced as
  // a pointer, which for a boolean is the integer 1.
  expect_string("m = {k: (true && 2)}; m as string;", "{\"k\": 2}");
  // Two entries, read back by key: a map does not iterate in insertion order,
  // and which order it does iterate in is an open question rather than
  // something this test should be pinning down.
  expect_integer("m = {j: 1, k: (false || 2)}; m[\"j\"];", 1);
  expect_integer("m = {j: 1, k: (false || 2)}; m[\"k\"];", 2);
}


// `m.b` and `m["b"]` are the same read, so they have to answer a missing key
// the same way, and the subscript form answers null (4.8).  The period form
// answered `Map Key Not Found`, on the strength of a note in 13.7 claiming
// the subscript said the same.  Both engines agreed with each other, so the
// differential could not see it.
TEST(Period, AMissingMemberIsNull) {
  expect_null("m = {:}; m.a;");
  expect_null("m = {a: 1}; m.b;");
  expect_integer("m = {a: 1}; m.a;", 1);
  // The two spellings, on the same map, side by side.
  expect_null("m = {a: 1}; m[\"b\"];");
  expect_integer("m = {a: 1}; m[\"a\"];", 1);
  // A key whose value is null is indistinguishable from an absent one, which
  // is what the subscript form already committed to.
  expect_null("m = {a: null}; m.a;");
}


// A slice whose start is past the far end selects nothing (4.9), and that had
// to be said before the bounds corrections rather than left to them.
// correct_bounds assumes it is moving *towards* the boundary it is given; when
// the start is already past it the division truncates towards zero and the
// rounding then moves the end one whole step the wrong way, so the end came
// out *after* the start and the intersection check let it through.  The build
// loop then read element 10 of a two-element array.  A step of 1 hid it,
// because the correction lands exactly on the boundary.
TEST(Slice, AStartPastTheFarEndSelectsNothing) {
  expect_string("([0, 1][10:65535:256]) as string;", "[]");
  expect_string("([0, 1][10:20]) as string;", "[]");
  expect_string("([0, 1][2:5:3]) as string;", "[]");
  expect_string("([][0:1]) as string;", "[]");
  expect_string("(\"ab\"[10:65535:256]) as string;", "");
  expect_string("(\"ab\"[5:9:2]) as string;", "");
  // Walking backwards, the far end is the other one.
  expect_string("([0, 1][-9::-256]) as string;", "[]");
  expect_string("(\"ab\"[-9::-256]) as string;", "");
  // The slices that were already right stay right.
  expect_string("([0, 1, 2][1:3]) as string;", "[1, 2]");
  expect_string("([0, 1, 2][::-1]) as string;", "[2, 1, 0]");
  expect_string("([0, 1, 2][5::-1]) as string;", "[2, 1, 0]");
  expect_string("(\"abc\"[::-1]) as string;", "cba");
  expect_string("(\"abc\"[5::-1]) as string;", "cba");
}


// A slice's bounds are corrected by stepping towards the container in whole
// steps, and that arithmetic was done in the signed type it is handed.  A step
// near the width of that type overflows it, which is undefined behaviour and
// not the wrap the code was written as though it would get: the corrected end
// came out *below* the start, so a slice that begins at a perfectly ordinary
// in-range index selected nothing at all.  Four sites, in two files that carry
// the same helper twice: the correction's product, the loop's own increment,
// the slice length, and negating the most negative step.
TEST(Slice, AStepNearTheWidthOfTheTypeStillSelectsTheStart) {
  // The start is in range and the step is positive, so whatever the step and
  // the end are, the element at the start is selected.
  expect_string("([1, 2, 3][2:9223372036854775807:9223372036854775807]) as string;", "[3]");
  expect_string("(\"abc\"[2:9223372036854775807:9223372036854775807]) as string;", "c");
  expect_string("([1, 2, 3][1:3:9223372036854775807]) as string;", "[2]");
  expect_string("(\"abc\"[1:3:9223372036854775807]) as string;", "b");
  // The most negative integer has no positive counterpart, so it is the value
  // that breaks a plain negation of the step as well as the distance.
  expect_string("([1, 2, 3, 4][3:-9223372036854775808:-9223372036854775808]) as string;", "[4]");
  expect_string("(\"abcd\"[3:-9223372036854775808:-9223372036854775808]) as string;", "d");
}


// 4.9 says the semantics are Python's, and gives the case outright: a start
// past the near end "starts at the beginning".  The implementation stepped in
// towards the container instead, in whole steps, which keeps the phase the
// start would have had - so a start one short of a multiple of the step began
// one element in.  A step of 1 hides it entirely, which is why every slice
// anyone writes by hand agreed.
TEST(Slice, AStartPastTheNearEndBeginsAtTheEdge) {
  expect_string("([0, 1, 2][-34::3]) as string;", "[0]");
  expect_string("(\"abc\"[-34::3]) as string;", "a");
  expect_string("([0, 1, 2][-4::3]) as string;", "[0]");
  // Walking backwards, the near end is the other one.
  expect_string("([0, 1, 2][34::-3]) as string;", "[2]");
  expect_string("(\"abc\"[34::-3]) as string;", "c");
  // The phase is not kept from outside, but it is kept from inside: a start
  // that is already in range still decides where the walk lands.
  expect_string("([0, 1, 2, 3, 4, 5][1::3]) as string;", "[1, 4]");
  expect_string("([0, 1, 2, 3, 4, 5][4::-3]) as string;", "[4, 1]");
  // A step wide enough to leave the container in one move selects the edge.
  expect_string("([0, 1, 2][-9223372036854775808::63]) as string;", "[0]");
  expect_string("(\"abc\"[-9223372036854775808::63]) as string;", "a");
  expect_string("([0, 1, 2][9223372036854775807::-63]) as string;", "[2]");
  expect_string("(\"abc\"[9223372036854775807::-63]) as string;", "c");
}


// A function's printed form used to carry its entry point, which is a bytecode
// offset under one engine and a machine address under the other - so the same
// program answered differently depending on which ran it, and the answer the
// default engine gave was a code address in text a template can emit.
TEST(Cast, AFunctionDoesNotPrintItsAddress) {
  // `f as string` is `Not supported`, so a container's rendering is how the
  // text reaches a template at all - and how this hid.
  expect_string("function f(a) { 1; } [f] as string;", "[Function(1)]");
  expect_string("function f() { 1; } [f] as string;", "[Function(0)]");
  expect_string("function f(a, b) { 1; } [f] as string;", "[Function(2)]");
  expect_string("function f(a) { 1; } {k: f} as string;", "{\"k\": Function(1)}");
  // A function assigned to another name is the same function.
  expect_string("function f(a) { 1; } g = f; [g] as string;", "[Function(1)]");
  expect_error("function f(a) { 1; } f as string;", "Error: Not supported");
}


// The array's cast slot held the generic dispatcher, which is the function
// that reads the cast slot - so `[] as bool` called itself until the stack
// ran out.  A container casts to a boolean, which is its truthiness, and to a
// string, which is its rendering; there is no number it could sensibly be.
TEST(Cast, Containers) {
  expect_boolean("[] as bool;", false);
  expect_boolean("[1] as bool;", true);
  expect_boolean("[0] as bool;", true);
  expect_boolean("{:} as bool;", false);
  expect_boolean("{a: 1} as bool;", true);
  expect_string("[1, 2] as string;", "[1, 2]");
  expect_string("[] as string;", "[]");
  expect_string("{a: 1} as string;", "{\"a\": 1}");
  expect_error("[] as int;", "Error: Not supported");
  expect_error("[] as float;", "Error: Not supported");
  expect_error("{:} as int;", "Error: Not supported");
}


// Putting a value into a container copies it unless it is a temporary or a
// singleton - the same rule `assign_index` follows (13.21), and what keeps a
// container held by a name from being aliased by every literal that mentions
// it.  The x86-64 engine emitted the test the wrong way round: it took the
// adopt path when the flag was clear, so a named container was aliased and the
// only thing ever copied was a temporary singleton, which is nothing.
TEST(Assignment, ALiteralStoresACopyOfWhatItIsGiven) {
  expect_integer("x = [1]; y = [x]; x[0] = 9; y[0][0];", 1);
  expect_integer("x = [1]; y = {k: x}; x[0] = 9; y[\"k\"][0];", 1);
  expect_integer("x = {a: 1}; y = [x]; x.a = 9; y[0][\"a\"];", 1);
  expect_integer("x = {a: 1}; y = {k: x}; x.a = 9; y[\"k\"][\"a\"];", 1);
  // A name still refers to the same container; only putting one *into* a
  // container copies it (section 3).
  expect_integer("x = [1]; y = x; x[0] = 9; y[0];", 9);
  // A temporary is adopted rather than copied, so a literal built in place is
  // not copied twice.
  expect_string("[[1, 2], 3] as string;", "[[1, 2], 3]");
  expect_string("{k: [1, 2]} as string;", "{\"k\": [1, 2]}");
}


// Storing a container into itself took the deep-copy path in assign_index,
// which is the only path that uses the execution context - and the x86-64
// caller was loading the context into the second argument register and then
// popping the index over it, so the callee read whatever was in the fourth.
// Assigning a temporary never noticed, because those are adopted without the
// context being touched.
TEST(Assignment, StoringAContainerIntoItself) {
  expect_string("x = [1, 2]; x[0] = x; x as string;", "[[1, 2], 2]");
  expect_integer("x = [1, 2]; x[0] = x; x[0][1];", 2);
  // This line used to end with `1;` rather than reading the map back, and so
  // could not see that the map had stored a reference to itself: rendering it
  // recursed until the stack ran out.
  expect_string("m = {:}; m[\"k\"] = m; m as string;", "{\"k\": {}}");
  expect_integer("d = {a: 1}; d.b = d; d.b.a;", 1);
  // The copy is of what the map held at the time, so it has no `b` of its own.
  expect_null("d = {a: 1}; d.b = d; d.b.b;");
  // Not rendered whole: a map iterates in hash order, so `{a, b}` comes back
  // as `b` first.  Read the copy back through its key instead.
  // Assigning a value held by another name takes the same path.
  expect_string("x = [1, 2]; y = [3]; x[0] = y; x as string;", "[[3], 2]");
  expect_string("m = {:}; n = [1]; m.v = n; m as string;", "{\"v\": [1]}");
  // ...and what it takes is a copy, so a later write through the other name
  // does not reach it.  A map adopted instead, where an array already copied.
  // Read back by key rather than rendered: a map iterates in hash order.
  expect_integer("m = {a: 1}; n = {a: 1}; m.b = n; n.a = 9; m.b.a;", 1);
  expect_integer("m = {a: 1}; n = [1]; m.b = n; n[0] = 9; m.b[0];", 1);

  // ...and it takes that copy when the element is evaluated, not when the
  // literal closes.  The bytecode engine did the whole copy pass in its ARRAY
  // and MAP instructions, which see every element at once and so ran after
  // all of them - a later element's side effect then reached back into an
  // earlier slot, and the two engines disagreed about the same literal.
  expect_string("x = [(d = [1]), (d[1] = 9)]; x as string;", "[[1], 9]");
  expect_integer("x = [(d = [1]), (d[1] = 9)]; (x[0]).size;", 1);
  expect_integer("x = {k1: (d = [1]), k2: (d[1] = 9)}; (x.k1).size;", 1);
  // The element that does the mutating still sees its own write.
  expect_integer("x = [(d = [1]), (d[1] = 9)]; d.size;", 2);

  // A freshly built container is nobody else's, so storing one into another
  // adopts it rather than copying it.  A map said otherwise where an array
  // said so, which cost a deep copy of every map literal put inside anything
  // - and a copy of a map rebuilds its hash table by re-inserting in
  // iteration order, so the copy came out in a different order from the
  // original.  Asserted as a comparison rather than a literal string because
  // the order itself is the hash's business and this suite does not pin it:
  // a map nested inside another renders exactly as it does on its own.
  expect_boolean("(({k1: 1, k0: 2, k3: 3}) as string)"
    " == ((({k1: {k1: 1, k0: 2, k3: 3}}).k1) as string);", true);
  expect_boolean("(([1, 2, 3]) as string) == (((([1, 2, 3]))[:]) as string);", true);

  // Storing it into itself at an index that grows the array.  The grow loop
  // used to leave the slot being assigned unwritten while raising the count,
  // and the deep copy of the value - which is this same array - then walked
  // the slot that did not exist yet.
  expect_string("a = [1]; a[5] = a; a as string;",
    "[1, null, null, null, null, [1, null, null, null, null, null]]");
  expect_string("a = [1]; a[5] = 7; a as string;", "[1, null, null, null, null, 7]");
  expect_string("a = [1]; a[2] = a; a as string;", "[1, null, [1, null, null]]");
}


#ifdef __linux__
/**
 * True if any mapping in /proc/self/maps covers `address`.
 */
static bool address_is_mapped(void * address) {
  uintptr_t target = (uintptr_t)address;
  FILE * maps = fopen("/proc/self/maps", "r");
  if (!maps) {
    return false;
  }
  char line[512];
  bool found = false;
  while (!found && fgets(line, sizeof(line), maps)) {
    uintptr_t start;
    uintptr_t end;
    if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR, &start, &end) == 2) {
      found = (target >= start) && (target < end);
    }
  }
  fclose(maps);
  return found;
}


// The JIT's code block is mmap()ed, and only gta_program_destroy() gives it
// back.  Nothing else here can see it leak: it is not a malloc'd block, so
// the allocation counts every other test in this file compares, and
// LeakSanitizer, both look straight past it.
TEST(Binary, CodeBlockIsUnmappedOnDestroy) {
  GTA_Program * program = gta_program_create(language, "print(1 + 1);");
  ASSERT_TRUE(program);
  void * binary = program->binary;
  if (!binary) {
    // The bytecode engine ran this one; there is no block to release.
    gta_program_destroy(program);
    GTEST_SKIP() << "no binary was generated";
  }
  // Read the mapping while it must still be there, so that a run which
  // cannot see /proc fails here rather than passing below for that reason.
  ASSERT_TRUE(address_is_mapped(binary));
  gta_program_destroy(program);
  ASSERT_FALSE(address_is_mapped(binary));
}
#endif // __linux__


int main(int argc, char **argv) {
  gcu_memory_reset_counts();
  language = gta_language_create();
  size_t alloc_count = gcu_get_alloc_count();
  size_t free_count = gcu_get_free_count();
  assert(language);

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();

  gcu_memory_reset_counts();
  gta_language_destroy(language);
  assert((alloc_count + gcu_get_alloc_count()) == (free_count + gcu_get_free_count()));
  return result;
}
