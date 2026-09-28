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
#include <ghoti.io/tang/macros.h>
#include <ghoti.io/tang/library/libraryRandom.h>
#include <ghoti.io/tang/computedValue/computedValueError.h>
#include <ghoti.io/tang/computedValue/computedValueInteger.h>
#include <ghoti.io/tang/computedValue/computedValueLibrary.h>
#include <ghoti.io/tang/computedValue/computedValueRNG.h>
#include <ghoti.io/tang/computedValue/computedValueFunctionNative.h>

/**
 * Random library attribute to get the global random number generator.
 *
 * @param context The context of the program being executed.
 * @return The computed value for the global random number generator (as a
 *   singleton).
 */
static GTA_Computed_Value * GTA_CALL gta_library_random_make_global(GTA_Execution_Context * context);


/**
 * Random library attribute to get a random number generator seeded with the
 * current time.
 *
 * @param context The context of the program being executed.
 * @return The computed value for a random number generator seeded with the
 *  current time.
 */
static GTA_Computed_Value * GTA_CALL gta_library_random_make_default(GTA_Execution_Context * context);


/**
 * Random library attribute to get a seeded random number generator.
 *
 * @param context The context of the program being executed.
 * @return A native function that creates a new seeded random number generator.
 */
static GTA_Computed_Value * GTA_CALL gta_library_random_make_seeded(GTA_Execution_Context * context);


/**
 * Callback for the random.seeded attribute.
 *
 * This function creates a new seeded random number generator.
 *
 * @param bound_object The object the function is bound to.  It should be NULL.
 * @param argc The number of arguments passed to the function.  It should be 1.
 * @param argv The arguments passed to the function.  The first argument should
 *   be the seed.
 */
static GTA_Computed_Value * gta_library_random_make_seeded_callback(GTA_Computed_Value * bound_object, GTA_UInteger argc, GTA_Computed_Value * argv[], GTA_Execution_Context *context);


/**
 * The value returned by random.seeded.
 *
 * The callback for this function creates a new seeded random number generator.
 */
static GTA_Computed_Value_Function_Native lib_rand_make_seeded = {
  .base = {
    .vtable = &gta_computed_value_function_native_vtable,
    .context = 0,
    .is_true = true,
    .is_error = false,
    .is_temporary = false,
    .requires_deep_copy = false,
    .is_singleton = true,
    .is_a_reference = false,
  },
  .callback = gta_library_random_make_seeded_callback,
  .bound_object = 0,
};


/**
 * The attributes of the Random library.
 */
static GTA_Computed_Value_Library_Attribute_Pair attributes[] = {
  {"default", gta_library_random_make_default},
  {"global", gta_library_random_make_global},
  {"seeded", gta_library_random_make_seeded},
};


/**
 * The Random library singleton.
 */
static GTA_Computed_Value_Library gta_computed_value_library_random_singleton = {
  .base = {
    .vtable = &gta_computed_value_library_vtable,
    .context = 0,
    .is_true = true,
    .is_error = false,
    .is_temporary = false,
    .requires_deep_copy = false,
    .is_singleton = true,
    .is_a_reference = false,
  },
  .name = "random",
  .attributes = attributes,
  .attribute_count = 0,
  .library = 0,
};
GTA_Computed_Value * gta_computed_value_library_random = (GTA_Computed_Value *)&gta_computed_value_library_random_singleton;


GTA_Computed_Value * GTA_CALL gta_library_random_load(GTA_MAYBE_UNUSED(GTA_Execution_Context * context)) {
  return gta_computed_value_library_random;
}


static GTA_Computed_Value * GTA_CALL gta_library_random_make_default(GTA_Execution_Context * context) {
  return (GTA_Computed_Value *)gta_computed_value_rng_create(context);
}


static GTA_Computed_Value * GTA_CALL gta_library_random_make_global(GTA_MAYBE_UNUSED(GTA_Execution_Context * context)) {
  return gta_computed_value_random_global;
}


static GTA_Computed_Value * GTA_CALL gta_library_random_make_seeded(GTA_MAYBE_UNUSED(GTA_Execution_Context * context)) {
  return (GTA_Computed_Value *)&lib_rand_make_seeded;
}


static GTA_Computed_Value * gta_library_random_make_seeded_callback(GTA_MAYBE_UNUSED(GTA_Computed_Value * bound_object), GTA_UInteger argc, GTA_Computed_Value * argv[], GTA_Execution_Context *context) {
  assert(!bound_object);
  assert(argv);
  if (argc != 1) {
    return gta_computed_value_error_argument_count_mismatch;
  }

  if (!GTA_COMPUTED_VALUE_IS_INTEGER(argv[0])) {
    return gta_computed_value_error_invalid_function_call;
  }

  GTA_Computed_Value_Integer * seed = (GTA_Computed_Value_Integer *)argv[0];
  return (GTA_Computed_Value *)gta_computed_value_rng_create_seeded(seed->value, context);
}


/**
 * Setup the proper attribute count and build the library attribute hash, which
 * cannot be done at compile time.
 */
GTA_INIT_FUNCTION(setup) {
  gta_computed_value_library_random_singleton.attribute_count = sizeof(attributes) / sizeof(GTA_Computed_Value_Library_Attribute_Pair);
  if (!gta_computed_value_library_build_library_attributes_hash(&gta_computed_value_library_random_singleton)) {
    if (gta_computed_value_library_random_singleton.library) {
      gta_library_destroy(gta_computed_value_library_random_singleton.library);
      gta_computed_value_library_random_singleton.library = 0;
    }
  }
}

