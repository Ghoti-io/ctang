#include <gtest/gtest.h>
#include <ghoti.io/cutil/memory.h>
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


#define TEST_REUSABLE_PROGRAM(code, flags) \
  gcu_memory_reset_counts(); \
  GTA_Program * program = gta_program_create_with_flags(language, code, flags); \
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
  TEST_REUSABLE_PROGRAM(code, GTA_PROGRAM_FLAG_DEFAULT); \
  TEST_CONTEXT_SETUP(); \
  ASSERT_TRUE(gta_program_execute(context)); \
  ASSERT_TRUE(context->result);

#define TEST_PROGRAM_SETUP_NO_RUN(code) \
  TEST_REUSABLE_PROGRAM(code, GTA_PROGRAM_FLAG_DEFAULT); \
  TEST_CONTEXT_SETUP();

#define TEST_PROGRAM_TEARDOWN() \
  TEST_CONTEXT_TEARDOWN(); \
  TEST_REUSABLE_PROGRAM_TEARDOWN();

#define TEST_TEMPLATE_SETUP(code) \
  TEST_REUSABLE_PROGRAM(code, GTA_PROGRAM_FLAG_IS_TEMPLATE); \
  TEST_CONTEXT_SETUP(); \
  ASSERT_TRUE(gta_program_execute(context)); \
  ASSERT_TRUE(context->result);

#define TEST_TEMPLATE_SETUP_NO_RUN(code) \
  TEST_REUSABLE_PROGRAM(code, GTA_PROGRAM_FLAG_IS_TEMPLATE); \
  TEST_CONTEXT_SETUP();


TEST(ControlFlow, IF) {
  {
    // True condition with else branch.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      if (true) {
        print("true");
      } else {
        print("false");
      }
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start true end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // False condition with else branch.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      if (false) {
        print("true");
      } else {
        print("false");
      }
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start false end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // True condition without else branch.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      if (true) {
        print("true");
      }
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start true end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // False condition without else branch.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      if (false) {
        print("true");
      }
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start  end");
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(ControlFlow, While) {
  {
    // True condition.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      i = 0;
      while (i < 3) {
        print(i);
        i = i + 1;
      }
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 012 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // False condition.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      i = 3;
      while (i < 3) {
        print(i);
        i = i + 1;
      }
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start  end");
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(ControlFlow, DoWhile) {
  {
    // True condition.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      i = 0;
      do {
        print(i);
        i = i + 1;
      } while (i < 3);
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 012 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // False condition.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      i = 3;
      do {
        print(i);
        i = i + 1;
      } while (i < 3);
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 3 end");
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(ControlFlow, For) {
  {
    // True condition.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      for (i = 0; i < 3; i = i + 1) {
        print(i);
      }
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 012 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // False condition.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      for (i = 3; i < 3; i = i + 1) {
        print(i);
      }
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start  end");
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(ControlFlow, RangedFor) {
  {
    // Non-empty container.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      for (i : [0, 1, 2]) {
        print(i);
      }
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 012 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Empty container.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      for (i : []) {
        print(i);
      }
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start  end");
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(ControlFlow, RangedForBindsTheElement) {
  {
    // The loop variable is bound to the element, not to a copy of it, so
    // writing through it reaches the container. Arrays are reference types
    // everywhere else in the language - `b = a` and passing to a function
    // both share - and this binding is not an exception.
    //
    // The two engines disagreed here for the life of the library: the
    // bytecode interpreter emitted ADOPT, which deep copies, while the
    // x86_64 compiler asked for the same thing and never got it, because it
    // read the one-byte is_temporary and is_singleton flags as 64-bit words.
    TEST_PROGRAM_SETUP(R"(
      a = [[1], [2]];
      for (x : a) {
        x[0] = 9;
      }
      print(a[0][0]);
      print(a[1][0]);
    )");
    ASSERT_STREQ(context->output->buffer, "99");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Rebinding the loop variable is not mutation, and must not reach the
    // array. This is the line between the two, and it is why clearing
    // is_temporary is not the same as copying.
    TEST_PROGRAM_SETUP(R"(
      a = [1, 2];
      for (x : a) {
        x = 99;
      }
      print(a[0]);
      print(a[1]);
    )");
    ASSERT_STREQ(context->output->buffer, "12");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A map element is shared the same way.
    TEST_PROGRAM_SETUP(R"(
      a = [{k: 1}];
      for (x : a) {
        x["k"] = 9;
      }
      print(a[0]["k"]);
    )");
    ASSERT_STREQ(context->output->buffer, "9");
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Assignment, ContainersAreShared) {
  {
    // Assignment does not copy: `b = a` gives both names the same array, in
    // both engines. This is the language as it stands rather than a
    // conclusion anyone reached - see the comments in
    // gta_ast_node_assign_compile_to_bytecode() and its x86_64 counterpart -
    // and it is pinned here so that changing it has to be deliberate.
    TEST_PROGRAM_SETUP(R"(
      a = [1];
      b = a;
      b[0] = 9;
      print(a[0]);
    )");
    ASSERT_STREQ(context->output->buffer, "9");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // A map is shared the same way.
    TEST_PROGRAM_SETUP(R"(
      a = {k: 1};
      b = a;
      b["k"] = 9;
      print(a["k"]);
    )");
    ASSERT_STREQ(context->output->buffer, "9");
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(ControlFlow, Break) {
  {
    // Break in a while loop.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      i = 0;
      while (i < 4) {
        i = i + 1;
        if (i == 3) {
          break;
        }
        print(i);
      }
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 12 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Break in a do..while loop.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      i = 0;
      do {
        i = i + 1;
        if (i == 3) {
          break;
        }
        print(i);
      } while (i < 4);
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 12 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Break in a for loop.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      for (i = 0; i < 4; i = i + 1) {
        if (i == 2) {
          break;
        }
        print(i);
      }
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 01 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Break outside of a control flow structure.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      break;
      print(" end");
    )");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    ASSERT_STREQ(context->output->buffer, "start ");
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(ControlFlow, Continue) {
  {
    // Continue in a while loop.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      i = 0;
      while (i < 3) {
        i = i + 1;
        if (i == 2) {
          continue;
        }
        print(i);
      }
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 13 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Continue in a do..while loop.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      i = 0;
      do {
        i = i + 1;
        if (i == 2) {
          continue;
        }
        print(i);
      } while (i < 3);
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 13 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Continue in a for loop.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      for (i = 0; i < 3; i = i + 1) {
        if (i == 1) {
          continue;
        }
        print(i);
      }
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 02 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Continue outside of a control flow structure.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      continue;
      print(" end");
    )");
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(GTA_COMPUTED_VALUE_IS_NULL(context->result));
    ASSERT_STREQ(context->output->buffer, "start ");
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Function, Simple) {
  {
    // Function with no arguments.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      function foo() {
        print("foo");
      }
      foo();
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start foo end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Function with arguments.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      function foo(a, b) {
        print(a + b);
      }
      foo(1, 2);
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 3 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Function with return value.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      function foo(a, b) {
        return a + b;
      }
      print(foo(1, 2));
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 3 end");
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(ControlFlowEdgeCases, Break) {
  {
    // Break in global context.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      break;
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start ");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Break in function.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      function foo() {
        print("foo");
        break;
        print("bar");
      }
      foo();
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start foo end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Multiple breaks.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      i = 0;
      while (i < 3) {
        if (i == 2) {
          break;
        }
        print(i);
        i = i + 1;
      }
      i = 0;
      while (i < 3) {
        if (i == 2) {
          break;
        }
        print("a");
        i = i + 1;
        break;
      }
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 01a end");
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(ControlFlowEdgeCases, Continue) {
  {
    // Continue in global context.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      continue;
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start ");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Continue in function.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      function foo() {
        print("foo");
        continue;
        print("bar");
      }
      foo();
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start foo end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Multiple continues.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      i = 0;
      while (i < 3) {
        i = i + 1;
        if (i == 2) {
          continue;
        }
        print(i);
      }
      i = 0;
      while (i < 3) {
        i = i + 1;
        if (i == 2) {
          continue;
        }
        print("a");
        continue;
      }
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 13aa end");
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(VariableScope, Global) {
  {
    // Local variable in a function.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      a = 1;
      function foo() {
        a = 2;
        print(a);
      }
      print(a);
      foo();
      print(a);
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 121 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Global variable in a function, assignment separate.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      a = 1;
      function foo() {
        global a;
        print(a);
        a = 2;
        print(a);
      }
      print(a);
      foo();
      print(a);
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 1122 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Global variable in a function, assignment combined.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      a = 1;
      function foo() {
        global a = 2;
        print(a);
      }
      print(a);
      foo();
      print(a);
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 122 end");
    TEST_PROGRAM_TEARDOWN();
  }
}


static GTA_Computed_Value * GTA_CALL int_3_callback(GTA_MAYBE_UNUSED(GTA_Computed_Value * bound_object), GTA_MAYBE_UNUSED(GTA_UInteger argc), GTA_MAYBE_UNUSED(GTA_Computed_Value * argv[]), GTA_Execution_Context * context) {
  assert(!bound_object);
  assert(argv);
  return (GTA_Computed_Value *)gta_computed_value_integer_create(3, context);
}

static GTA_Computed_Value * GTA_CALL make_int_3(GTA_Execution_Context * context) {
  return (GTA_Computed_Value *)gta_computed_value_function_native_create(int_3_callback, NULL, context);
}

static GTA_Computed_Value * GTA_CALL add_callback(GTA_MAYBE_UNUSED(GTA_Computed_Value * bound_object), GTA_UInteger argc, GTA_Computed_Value * argv[], GTA_Execution_Context * context) {
  assert(!bound_object);
  assert(argc == 2);
  assert(argv);
  assert(GTA_COMPUTED_VALUE_IS_INTEGER(argv[0]));
  assert(GTA_COMPUTED_VALUE_IS_INTEGER(argv[1]));
  GTA_Computed_Value_Integer * a = (GTA_Computed_Value_Integer *)argv[0];
  GTA_Computed_Value_Integer * b = (GTA_Computed_Value_Integer *)argv[1];
  // Verify that they are given in the expected order.
  // This was a bug in the past, so don't remove this check.
  assert(a->value == 1);
  assert(b->value == 2);
  return (GTA_Computed_Value *)gta_computed_value_integer_create(a->value + b->value, context);
}

static GTA_Computed_Value * GTA_CALL make_add(GTA_Execution_Context * context) {
  return (GTA_Computed_Value *)gta_computed_value_function_native_create(add_callback, NULL, context);
}

static GTA_Computed_Value * GTA_CALL str_len(GTA_Computed_Value * bound_object, GTA_MAYBE_UNUSED(GTA_UInteger argc), GTA_MAYBE_UNUSED(GTA_Computed_Value * argv[]), GTA_Execution_Context * context) {
  assert(bound_object);
  assert(argc == 0);
  assert(GTA_COMPUTED_VALUE_IS_STRING(bound_object));
  GTA_Computed_Value_String * str = (GTA_Computed_Value_String *)bound_object;
  return (GTA_Computed_Value *)gta_computed_value_integer_create(str->value->byte_length, context);
}

static GTA_Computed_Value * GTA_CALL make_str_len(GTA_Execution_Context * context) {
  GTA_Unicode_String * abc = gta_unicode_string_create("abc", 3, GTA_UNICODE_STRING_TYPE_TRUSTED);
  GTA_Computed_Value * str = (GTA_Computed_Value *)gta_computed_value_string_create(abc, true, context);
  return (GTA_Computed_Value *)gta_computed_value_function_native_create(str_len, str, context);
}


TEST(NativeFunction, Library) {
  {
    // Simple function, no arguments
    TEST_PROGRAM_SETUP_NO_RUN(R"(
      use a;
      print("start ");
      print(a());
      print(" end");
    )");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_int_3));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_STREQ(context->output->buffer, "start 3 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Simple function, with arguments
    TEST_PROGRAM_SETUP_NO_RUN(R"(
      use a;
      print("start ");
      print(a(1, 2));
      print(" end");
    )");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_add));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_STREQ(context->output->buffer, "start 3 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Function with bound object
    TEST_PROGRAM_SETUP_NO_RUN(R"(
      use a;
      print("start ");
      print(a());
      print(" end");
    )");
    ASSERT_TRUE(gta_library_add_library_from_string(context->library, "a", make_str_len));
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_STREQ(context->output->buffer, "start 3 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Function not found. Does not crash.
    TEST_PROGRAM_SETUP_NO_RUN(R"(
      use a;
      print("start ");
      print(a());
      print(" end");
    )");
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_STREQ(context->output->buffer, "start  end");
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(Attributes, String) {
  {
    // Length (in graphemes).
    // The long string of hex values is a UTF-8 encoding of the Scottish Flag.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      print("$\xF0\x9F\x8F\xB4\xF3\xA0\x81\xA7\xF3\xA0\x81\xA2\xF3\xA0\x81\xB3\xF3\xA0\x81\xA3\xF3\xA0\x81\xB4\xF3\xA0\x81\xBF.".length);
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 3 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Length (in bytes).
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      print("$\xF0\x9F\x8F\xB4\xF3\xA0\x81\xA7\xF3\xA0\x81\xA2\xF3\xA0\x81\xB3\xF3\xA0\x81\xA3\xF3\xA0\x81\xB4\xF3\xA0\x81\xBF.".byte_length);
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 30 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // HTML encoding.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      print("a&b".html);
      print(" end");
    )");
    GTA_Unicode_Rendered_String rendered = gta_unicode_string_render(context->output);
    ASSERT_TRUE(rendered.buffer);
    ASSERT_STREQ(rendered.buffer, "start a&amp;b end");
    gcu_free(rendered.buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Undo the HTML encoding.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      print("a&b\"'".html.raw);
      print(" end");
    )");
    GTA_Unicode_Rendered_String rendered = gta_unicode_string_render(context->output);
    ASSERT_TRUE(rendered.buffer);
    ASSERT_STREQ(rendered.buffer, "start a&b\"' end");
    gcu_free(rendered.buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Double-encoding.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      print("a&b".html.render.html);
      print(" end");
    )");
    GTA_Unicode_Rendered_String rendered = gta_unicode_string_render(context->output);
    ASSERT_TRUE(rendered.buffer);
    ASSERT_STREQ(rendered.buffer, "start a&amp;amp;b end");
    gcu_free(rendered.buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // HTML attribute encoding.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      print("a&b\"'".html_attribute);
      print(" end");
    )");
    GTA_Unicode_Rendered_String rendered = gta_unicode_string_render(context->output);
    ASSERT_TRUE(rendered.buffer);
    ASSERT_STREQ(rendered.buffer, "start a&amp;b&quot;&#39; end");
    gcu_free(rendered.buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Percent encoding.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      print("a & b".percent);
      print(" end");
    )");
    GTA_Unicode_Rendered_String rendered = gta_unicode_string_render(context->output);
    ASSERT_TRUE(rendered.buffer);
    ASSERT_STREQ(rendered.buffer, "start a+%26+b end");
    gcu_free(rendered.buffer);
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // JavaScript encoding.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      print("a&b\"'\n".javascript);
      print(" end");
    )");
    GTA_Unicode_Rendered_String rendered = gta_unicode_string_render(context->output);
    ASSERT_TRUE(rendered.buffer);
    ASSERT_STREQ(rendered.buffer, "start a\\u0026b\\\"\\'\\n end");
    gcu_free(rendered.buffer);
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(Attributes, Array) {
  {
    // size (empty array).
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      print([].size);
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 0 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // size (non-empty array).
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      print([1, 2, 3].size);
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 3 end");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // size (sliced array).
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      a = [1, 2, 3];
      print(a[-2:].size);
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 2 end");
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Recursion, Fibonacci) {
  {
    // Fibonacci sequence.
    TEST_PROGRAM_SETUP(R"(
      print("start ");
      function fib(n) {
        if (n <= 0) {
          return 0;
        }
        else if (n <= 2) {
          return 1;
        }
        return fib(n - 1) + fib(n - 2);
      }
      print(fib(10));
      print(" end");
    )");
    ASSERT_STREQ(context->output->buffer, "start 55 end");
    TEST_PROGRAM_TEARDOWN();
  }
}

TEST(Assignment, StoredValueIsNotTemporary) {
  // The arithmetic operators have an in-place fast path: when an operand is
  // still marked temporary they mutate it rather than allocating a result.
  // A value that has been stored in a variable must therefore not be marked
  // temporary, or the next expression that reads the variable rewrites it.
  //
  // The bytecode compiler emitted no SET_NOT_TEMP before its POKE, so `a` below
  // came back as 9 instead of 6. The x86_64 path was always correct, which is
  // why this file must keep asserting under both engines - it is run twice,
  // once with TANG_DISABLE_BINARY and once with TANG_DISABLE_BYTECODE, and a
  // single-engine assertion would have passed throughout.
  {
    // Reading a self-assigned variable must not modify it.
    TEST_PROGRAM_SETUP(R"(
      a = 5;
      a = a + 1;
      b = a + 3;
      print(a); print(","); print(b);
    )");
    ASSERT_STREQ(context->output->buffer, "6,9");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // The step is the constant, so a wrong answer here is not off-by-one.
    TEST_PROGRAM_SETUP(R"(
      a = 5;
      a = a * 2;
      b = a + 3;
      print(a); print(","); print(b);
    )");
    ASSERT_STREQ(context->output->buffer, "10,13");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // The case that makes it matter: `i = i + 1` advances every loop, so a
    // body that reads the counter arithmetically used to corrupt it. This
    // printed "0 2 " before the fix.
    TEST_PROGRAM_SETUP(R"(
      for (i = 0; i < 3; i = i + 1) {
        x = i + 1;
        print(i); print(" ");
      }
    )");
    ASSERT_STREQ(context->output->buffer, "0 1 2 ");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Same defect, opposite sign: this one did not terminate at all, because
    // the body decremented the counter by exactly what the loop added.
    TEST_PROGRAM_SETUP(R"(
      for (i = 0; i < 4; i = i + 1) {
        x = i - 1;
        print(i); print(" ");
      }
    )");
    ASSERT_STREQ(context->output->buffer, "0 1 2 3 ");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // while and do-while advance by the same self-assignment.
    TEST_PROGRAM_SETUP(R"(
      i = 0;
      while (i < 3) {
        x = i + 1;
        print(i); print(" ");
        i = i + 1;
      }
    )");
    ASSERT_STREQ(context->output->buffer, "0 1 2 ");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    TEST_PROGRAM_SETUP(R"(
      i = 0;
      do {
        x = i + 1;
        print(i); print(" ");
        i = i + 1;
      } while (i < 3);
    )");
    ASSERT_STREQ(context->output->buffer, "0 1 2 ");
    TEST_PROGRAM_TEARDOWN();
  }
  {
    // Assigning a variable to another must still not alias a scalar.
    TEST_PROGRAM_SETUP(R"(
      a = 5;
      b = a;
      b = b + 1;
      print(a); print(","); print(b);
    )");
    ASSERT_STREQ(context->output->buffer, "5,6");
    TEST_PROGRAM_TEARDOWN();
  }
}


TEST(Execute, Template) {
  {
    // Fibonacci sequence.
    TEST_TEMPLATE_SETUP(R"(<%
      function fib(n) {
        if (n <= 0) {
          return 0;
        }
        else if (n <= 2) {
          return 1;
        }
        return fib(n - 1) + fib(n - 2);
      }
      num = fib(10);
    %>start <%= num %> end)");
    ASSERT_STREQ(context->output->buffer, "start 55 end");
    TEST_PROGRAM_TEARDOWN();
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


// A local is addressed as stack[fp + position], and the bytecode engine never
// made the slots past the arguments exist.  They were above the top of the
// stack, which is where the next push goes, so a local and an expression
// temporary shared a slot; and reading one that had never been assigned read
// the vector's spare capacity, found a zero, and dereferenced it as a value.
// The x86-64 engine reserves the frame and fills it with null, and disagreed.
TEST(Function, ALocalHasASlotOfItsOwn) {
  // A temporary deeper than one slot used to land on the first local.
  expect_integer("function f() { z = 5; w = ((1 + 2) + 3); z; } f();", 5);
  expect_integer("function f() { z = 5; w = ((1 + 2) + 3); w; } f();", 6);
  expect_integer("function f(a, b) { c = a + b; d = ((c * 2) + 1); c; } f(3, 4);", 7);
  expect_integer("function f(a, b) { c = a + b; d = ((c * 2) + 1); d; } f(3, 4);", 15);
  // Section 6: reading a name that was never assigned yields null, and that
  // has to be the null value rather than a null pointer - so something has to
  // use it.
  expect_null("function f() { z; } f();");
  expect_boolean("function f() { z; } (!f());", true);
  expect_null("function f() { return z; } f();");
  expect_null("function f(a) { z; } f(1);");
  // The frame is given back, so a second call starts from null again.
  expect_integer("function f() { if (z) { return 1; } z = 2; return 0; } f(); f();", 0);
}


// The x86-64 caller walked the arguments backwards while walking the slots it
// wrote them into forwards, so every parameter was bound to the wrong
// argument: `f(1, 2, 3)` bound a to 3 and c to 1.  Silent wrong answers, not
// a crash, under the engine that runs by default, and the bytecode engine
// disagreed with it.
//
// Each parameter is read on its own so that a failure says which slot moved.
TEST(Function, EachParameterGetsItsOwnArgument) {
  expect_integer("function f(a) { return a; } f(7);", 7);
  expect_integer("function f(a, b) { return a; } f(1, 2);", 1);
  expect_integer("function f(a, b) { return b; } f(1, 2);", 2);
  expect_integer("function f(a, b, c) { return a; } f(1, 2, 3);", 1);
  expect_integer("function f(a, b, c) { return b; } f(1, 2, 3);", 2);
  expect_integer("function f(a, b, c) { return c; } f(1, 2, 3);", 3);
  expect_integer("function f(a, b, c, d) { return a; } f(1, 2, 3, 4);", 1);
  expect_integer("function f(a, b, c, d) { return d; } f(1, 2, 3, 4);", 4);
  // A weighted sum catches any permutation at once.
  expect_integer("function f(a, b, c, d, e) { return a + b * 2 + c * 4 + d * 8 + e * 16; } f(1, 2, 3, 4, 5);", 129);
  // Through a nested call, so that one frame's layout does not cover another's.
  expect_integer("function g(x, y) { return x - y; } function f(a, b) { return g(a, b); } f(10, 3);", 7);
}


// A repeated parameter name gives two parameters one slot, so the scope holds
// fewer variables than the function has parameters - and the x86-64 prologue
// works that difference out in size_t.  It wrapped, and the loop that fills
// the locals with null then ran about 2^64 times, which is what "hangs the
// compiler" was.
TEST(Function, ARepeatedParameterNameIsRejected) {
  const char * cases[] = {
    "function f(a, a) {}",
    "function f(a, b, a) {} 1;",
    "function f(a, a, a) {} 1;",
  };
  for (const char * code : cases) {
    gcu_memory_reset_counts();
    GTA_Program * program = gta_program_create(language, code);
    ASSERT_FALSE(program) << code;
    ASSERT_EQ(gcu_get_alloc_count(), gcu_get_free_count()) << code;
  }
  // Distinct names in the same positions still compile.
  expect_integer("function f(a, b) { return b; } f(1, 2);", 2);
}


// Reporting one of these used to abort the process.  gta_ast_node_destroy()
// folded its is_singleton test into the condition that picks the destructor,
// which sent singletons to the fallback - and the fallback is a free(), not a
// no-op.  So the check that exists to protect a singleton was what freed it,
// and every error reported by returning a parse-error singleton ended in
// "free(): invalid pointer".
TEST(Function, RedeclarationAndForwardCallsFailCompilation) {
  const char * cases[] = {
    "function f() {} function f() {}",
    "x = 1; function x() {}",
    "foo(); function foo() {}",
  };
  for (const char * code : cases) {
    gcu_memory_reset_counts();
    GTA_Program * program = gta_program_create(language, code);
    ASSERT_FALSE(program) << code;
    ASSERT_EQ(gcu_get_alloc_count(), gcu_get_free_count()) << code;
  }
  // Two different functions, and a call after the declaration, still compile.
  expect_integer("function f() { return 1; } function g() { return 2; } f() + g();", 3);
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


// A call is an expression and has to leave exactly one value behind, like
// every other one.  The bytecode engine's refusals set context->result and
// left nothing, so the stack was short by one: the POP after the statement
// took the value underneath, context->result was overwritten at the end by
// whatever was then on top, and `f = 3; f();` came out as 3 - no error, while
// the x86-64 engine said `Invalid function call` for the same source.
TEST(Function, CallingSomethingThatIsNotAFunctionIsAnError) {
  expect_error("f = 3; f();", "Error: Invalid function call");
  expect_error("(1)(2);", "Error: Invalid function call");
  expect_error("f = \"x\"; f();", "Error: Invalid function call");
  expect_error("f = [1]; f();", "Error: Invalid function call");
  expect_error("null();", "Error: Invalid function call");
  // The statement after it still runs, and gets the stack it expects.
  expect_integer("f = 3; f(); 7;", 7);
  expect_integer("f = 3; f(); f(); f(); 7;", 7);
}


TEST(Function, TheWrongNumberOfArgumentsIsAnError) {
  expect_error("function f(a) { return a; } f();", "Error: Argument Count Mismatch");
  expect_error("function f(a) { return a; } f(1, 2);", "Error: Argument Count Mismatch");
  expect_error("function f() { return 1; } f(1);", "Error: Argument Count Mismatch");
  expect_integer("function f(a) { return a; } f(1, 2); 7;", 7);
}



// Unbounded recursion used to run the process's own stack out under the
// x86-64 engine, which calls compiled functions with real `call`
// instructions - a template could take the host down.  Both engines now share
// one limit, carried on the execution context.
TEST(Function, RecursionIsBounded) {
  expect_error("function f(n) { return f(n + 1); } f(0);",
    "Error: Recursion Limit Exceeded");
  expect_error("function f() { return f(); } f();",
    "Error: Recursion Limit Exceeded");
  // Indirect recursion counts the same way.
  expect_error("function g(n) { return g(n); } function f(n) { return g(n); } f(1);",
    "Error: Recursion Limit Exceeded");
}


// The limit must not leak between calls: a call that hits it has to give back
// everything it counted, or a later call inherits the depth and fails too.
TEST(Function, TheCallDepthIsGivenBack) {
  const char * deep = "function d(n) { if (n <= 0) { return 0; } return d(n - 1); }";
  // Comfortably inside the default limit.
  expect_integer((std::string(deep) + " d(400);").c_str(), 0);
  // Past it, and then inside it again.
  expect_integer((std::string(deep) + " d(100000); d(400);").c_str(), 0);
  expect_integer((std::string(deep) + " d(100000); d(100000); d(400);").c_str(), 0);
  // And in a loop, where the count is spent and recovered repeatedly.
  expect_integer((std::string(deep) + " x = 1; for (i : [1, 2, 3]) { x = d(400); } x;").c_str(), 0);
}


// A host that knows its own stack can move the limit, and zero removes it.
TEST(Function, TheCallDepthLimitIsTheHostsToSet) {
  const char * code = "function d(n) { if (n <= 0) { return 0; } return d(n - 1); } d(40);";
  {
    GTA_Program * program = gta_program_create(language, code);
    ASSERT_TRUE(program);
    GTA_Execution_Context * context = gta_execution_context_create(program);
    ASSERT_TRUE(context);
    ASSERT_EQ(context->max_call_depth, (GTA_UInteger)GTA_EXECUTION_CONTEXT_DEFAULT_MAX_CALL_DEPTH);
    context->max_call_depth = 10;
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_TRUE(context->result->is_error);
    gta_execution_context_destroy(context);
    gta_program_destroy(program);
  }
  {
    GTA_Program * program = gta_program_create(language, code);
    ASSERT_TRUE(program);
    GTA_Execution_Context * context = gta_execution_context_create(program);
    ASSERT_TRUE(context);
    context->max_call_depth = 100;
    ASSERT_TRUE(gta_program_execute(context));
    ASSERT_TRUE(context->result);
    ASSERT_FALSE(context->result->is_error);
    gta_execution_context_destroy(context);
    gta_program_destroy(program);
  }
}


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
