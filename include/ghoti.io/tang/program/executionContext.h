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
 * Header file for the Context class.
 *
 * The Context class is used to manage the state of the execution environment
 * for a Tang program as it is being executed.
 */

#ifndef GHOTI_IO_GTA_PROGRAM_EXECUTIONCONTEXT_H
#define GHOTI_IO_GTA_PROGRAM_EXECUTIONCONTEXT_H

#include <ghoti.io/tang/macros.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdbool.h>
#include <ghoti.io/cutil/vector.h>
#include <ghoti.io/tang/unicodeString.h>

/**
 * Gobal variable creation function.
 *
 * Libraries and other contextual variables are not always needed.  In order to
 * prevent unnecessary memory allocations, gobals will not be instantiated
 * unless they are referenced in the script.  When registering a global using
 * gta_execution_context_add_library(), a callback with this signature must be
 * provided.  This callback will be invoked during the script setup.
 *
 * @param context The context of the currently executing script.
 * @return The resulting computed value.
 */
typedef GTA_Computed_Value * GTA_CALL (*GTA_Execution_Context_Global_Create) (GTA_Execution_Context * context);

/**
 * The call depth a new execution context allows by default.
 *
 * The x86-64 engine calls compiled functions with real `call` instructions,
 * so this is a bound on the process's own stack, not on a heap structure:
 * `function f(n) { return f(n + 1); } f(0);` used to run the stack out and
 * take the host down with it.  The figure is deliberately well inside a
 * default 8 MiB thread stack, because a frame's size depends on how many
 * locals the function has and a template author cannot be expected to
 * reason about that.
 */
#define GTA_EXECUTION_CONTEXT_DEFAULT_MAX_CALL_DEPTH 512

/**
 * The Context class.
 *
 * The Context class is used to manage the state of the execution environment
 * for a Tang program as it is being executed.
 */
struct GTA_Execution_Context {
  /**
   * The program being executed.
  */
  GTA_Program * program;
  /**
   * The output string.
   */
  GTA_Unicode_String * output;
  /**
   * The result of the last operation.
   */
  GTA_Computed_Value * result;
  /**
   * The stack used for execution.
   */
  GTA_VectorX * stack;
  /**
   * The stack used to track the base pointers.
   */
  GTA_VectorX * pc_stack;
  /**
   * The garbage collection list.
   */
  GTA_VectorX * garbage_collection;
  /**
   * A hash table used to store libraries and user-defined global variables.
   */
  GTA_Library * library;
  /**
   * A user-defined pointer that can be used to store additional data.
   */
  void * user_data;
  /**
   * The current frame pointer.
   */
  GTA_UInteger fp;
  /**
   * How many Tang function calls are currently on the stack.
   *
   * Both engines keep this; the x86-64 engine reads and writes it from
   * compiled code, which is why it lives here rather than being derived.
   */
  GTA_UInteger call_depth;
  /**
   * The deepest `call_depth` a call is allowed to reach.
   *
   * A call that would go past it yields
   * `gta_computed_value_error_recursion_limit` instead of being made.  A host
   * that knows its own stack may raise or lower this after creating the
   * context; zero disables the limit, which is only safe for the bytecode
   * engine, whose calls do not use the C stack.
   */
  GTA_UInteger max_call_depth;
};

/**
 * Creates a new Context object.
 *
 * Use with gta_execution_context_destroy().
 *
 * @see gta_execution_context_destroy()
 *
 * @param program The program associated with the execution.
 * @return The new Context object or NULL on failure.
 */
GTA_API GTA_NO_DISCARD GTA_Execution_Context * gta_execution_context_create(GTA_Program * program);

/**
 * Creates a new Context object using the supplied memory location.
 *
 * Use with gta_bytecode_execution_context_destroy_in_place().
 *
 * @see gta_bytecode_execution_context_destroy_in_place()
 *
 * @param context The memory location to use for the new Context object.
 * @param program The program associated with the execution.
 * @return true on success, false on failure.
 */
GTA_API bool gta_execution_context_create_in_place(GTA_Execution_Context * context, GTA_Program * program);

/**
 * Destroys a Context object.
 *
 * Use with gta_execution_context_create().
 *
 * @see gta_execution_context_create()
 *
 * @param context The Context object to destroy.
 */
GTA_API void gta_execution_context_destroy(GTA_Execution_Context * context);

/**
 * Destroys a Context object using the supplied memory location.
 *
 * The memory used by the Context object is not freed, but the object is
 * destroyed.
 *
 * Use with gta_execution_context_create_in_place().
 *
 * @see gta_execution_context_create_in_place()
 *
 * @param context The Context object to destroy.
 */
GTA_API void gta_execution_context_destroy_in_place(GTA_Execution_Context * context);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // GHOTI_IO_GTA_PROGRAM_EXECUTIONCONTEXT_H
