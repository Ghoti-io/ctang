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
#include <string.h>
#include <ghoti.io/cutil/memory.h>
#include <ghoti.io/cutil/semaphore.h>
#include <ghoti.io/cutil/random.h>
#include <ghoti.io/tang/macros.h>
#include <ghoti.io/tang/library/libraryRandom.h>
#include <ghoti.io/tang/computedValue/computedValue.h>
#include <ghoti.io/tang/computedValue/computedValueBoolean.h>
#include <ghoti.io/tang/computedValue/computedValueError.h>
#include <ghoti.io/tang/computedValue/computedValueInteger.h>
#include <ghoti.io/tang/computedValue/computedValueFloat.h>
#include <ghoti.io/tang/computedValue/computedValueFunctionNative.h>
#include <ghoti.io/tang/computedValue/computedValueRNG.h>
#include <ghoti.io/tang/program/executionContext.h>

#ifdef _WIN32
#include <windows.h>

#else
#include <sys/time.h>

#endif // _WIN32

/**
 * The semaphore for the global random number generator.
 *
 * The handle itself is not safe to draw from on two threads. This lock is
 * the one that makes the global generator shareable. A seeded generator is
 * not shared, so it takes no lock.
 */
static GCU_Semaphore global_semaphore;


/**
 * The global generator lives in static storage. A handle from the heap would
 * still be allocated when a program ends, and the tests require every heap
 * block to be freed by then.
 */
static GCU_Random_MT64_State global_mt;
static unsigned char global_pending[8];
static unsigned global_pending_off;
static unsigned global_pending_len;
static _Alignas(void *) unsigned char global_handle_storage[64];
static int global_ready;


static int global_fill(void * ctx, void * out, size_t n) {
  unsigned char * dst = out;
  while (n != 0) {
    size_t have;
    size_t take;
    if (global_pending_off == global_pending_len) {
      uint64_t word = gcu_random_mt64_next(ctx);
      global_pending[0] = (unsigned char)word;
      global_pending[1] = (unsigned char)(word >> 8);
      global_pending[2] = (unsigned char)(word >> 16);
      global_pending[3] = (unsigned char)(word >> 24);
      global_pending[4] = (unsigned char)(word >> 32);
      global_pending[5] = (unsigned char)(word >> 40);
      global_pending[6] = (unsigned char)(word >> 48);
      global_pending[7] = (unsigned char)(word >> 56);
      global_pending_off = 0;
      global_pending_len = 8;
    }
    have = (size_t)(global_pending_len - global_pending_off);
    take = n < have ? n : have;
    memcpy(dst, global_pending + global_pending_off, take);
    global_pending_off += (unsigned)take;
    dst += take;
    n -= take;
  }
  return 0;
}


/**
 * Get the next random boolean from the random number generator.
 */
static GTA_Computed_Value * GTA_CALL rng_next_bool(GTA_Computed_Value * self, GTA_Execution_Context * context);


/**
 * Get the next random integer from the random number generator.
 *
 * @param self The random number generator object.
 * @param context The execution context.
 * @return The next random integer.
 */
static GTA_Computed_Value * GTA_CALL rng_next_int(GTA_Computed_Value * self, GTA_Execution_Context * context);


/**
 * Get the next random float from the random number generator.
 *
 * The float is in the range [0, 1].
 *
 * @param self The random number generator object.
 * @param context The execution context.
 * @return The next random float.
 */
static GTA_Computed_Value * GTA_CALL rng_next_float(GTA_Computed_Value * self, GTA_Execution_Context * context);


/**
 * Set the seed of the random number generator.
 *
 * The seed of the global random number generator can not be changed.
 *
 * @param self The random number generator object.
 * @param context The execution context.
 * @return The native function callback for setting the seed.
 */
static GTA_Computed_Value * GTA_CALL rng_set_seed(GTA_Computed_Value * self, GTA_Execution_Context * context);


/**
 * The callback function for setting the seed of the random number generator.
 *
 * @param bound_object The random number generator object.
 * @param argc The number of arguments.  1 is expected.
 * @param argv The arguments.  The first argument is expected to be the seed.
 * @param context The execution context.
 * @return True if the seed was set successfully, an error otherwise.
 */
static GTA_Computed_Value * GTA_CALL rng_set_seed_callback(GTA_Computed_Value * bound_object, GTA_UInteger argc, GTA_Computed_Value * argv[], GTA_Execution_Context * context);


/**
 * The attributes for the GTA_Computed_Value_String class.
 */
static GTA_Computed_Value_Attribute_Pair attributes[] = {
  // TODO: Additional attributes planned:
  // {"next_int_range", rng_next_int_range},
  // {"next_float_range", rng_next_float_range},
  // {"next_gaussian", rng_next_gaussian},
  // {"shuffle", rng_shuffle},
  // {"sample", rng_sample},
  // {"choose", rng_choose},
  {"next_bool", rng_next_bool},
  {"next_int", rng_next_int},
  {"next_float", rng_next_float},
  {"set_seed", rng_set_seed},
};


/**
 * Get a default seed for the random number generator.
 *
 * The seed is generated from the current time.
 *
 * @return A default seed for the random number generator.
 */
static GTA_UInteger GTA_CALL rng_get_default_seed(void);


/**
 * Helper function to get the next random number from the random number generator.
 * 
 * @param self The random number generator.
 * @return The next random number.
 */
static GTA_UInteger GTA_CALL rng_get_next(GTA_Computed_Value_RNG * self);


GTA_Computed_Value_VTable gta_computed_value_rng_vtable = {
  .name = "RNG",
  .destroy = gta_computed_value_rng_destroy,
  .destroy_in_place = gta_computed_value_rng_destroy_in_place,
  .deep_copy = gta_computed_value_rng_deep_copy,
  .to_string = gta_computed_value_rng_to_string,
  .print = gta_computed_value_print_not_supported,
  .assign_index = gta_computed_value_assign_index_not_supported,
  .add = gta_computed_value_add_not_supported,
  .subtract = gta_computed_value_subtract_not_supported,
  .multiply = gta_computed_value_multiply_not_supported,
  .divide = gta_computed_value_divide_not_supported,
  .modulo = gta_computed_value_modulo_not_supported,
  .negative = gta_computed_value_negative_not_supported,
  .less_than = gta_computed_value_less_than_not_supported,
  .less_than_equal = gta_computed_value_less_than_equal_not_supported,
  .greater_than = gta_computed_value_greater_than_not_supported,
  .greater_than_equal = gta_computed_value_greater_than_equal_not_supported,
  .equal = gta_computed_value_equal_not_supported,
  .not_equal = gta_computed_value_not_equal_not_supported,
  .period = gta_computed_value_generic_period,
  .index = gta_computed_value_index_not_supported,
  .slice = gta_computed_value_slice_not_supported,
  .iterator_get = gta_computed_value_iterator_get_not_supported,
  .iterator_next = gta_computed_value_iterator_next_not_supported,
  .cast = gta_computed_value_cast_not_supported,
  .call = gta_computed_value_call_not_supported,
  .attributes = attributes,
  .attributes_count = 0,
};

// The global random number generator computed value singleton.
static GTA_Computed_Value_RNG gta_computed_value_random_global_singleton = {
  .base = {
    .vtable = &gta_computed_value_rng_vtable,
    .context = 0,
    .is_true = true,
    .is_error = false,
    .is_temporary = false,
    .requires_deep_copy = false,
    .is_singleton = true,
    .is_a_reference = false,
  },
  .seed = 0,
  .state = NULL,
};
GTA_Computed_Value * gta_computed_value_random_global = (GTA_Computed_Value *)&gta_computed_value_random_global_singleton;


GTA_Computed_Value_RNG * GTA_CALL gta_computed_value_rng_create(GTA_Execution_Context * context) {
  return gta_computed_value_rng_create_seeded(rng_get_default_seed(), context);
}


GTA_Computed_Value_RNG * GTA_CALL gta_computed_value_rng_create_seeded(GTA_UInteger seed, GTA_Execution_Context * context) {
  GTA_Computed_Value_RNG * self = (GTA_Computed_Value_RNG *)gcu_malloc(sizeof(GTA_Computed_Value_RNG));
  if (!self) {
    return 0;
  }
  if (context) {
    // Attempt to add the pointer to the context's garbage collection list.
    if (!GTA_VECTORX_APPEND(context->garbage_collection, GTA_TYPEX_MAKE_P(self))) {
      gcu_free(self);
      return NULL;
    }
  }
  if (!gta_computed_value_rng_create_seeded_in_place(self, seed, context)) {
    gcu_free(self);
    return NULL;
  }
  return self;
}


bool GTA_CALL gta_computed_value_rng_create_seeded_in_place(GTA_Computed_Value_RNG * self, GTA_UInteger seed, GTA_Execution_Context * context) {
  assert(self);
  // MT19937-64 on every architecture, so a seed names one sequence.
  GCU_Random * state = gcu_random_mt64((uint64_t)seed);
  if (!state) {
    return false;
  }

  *self = (GTA_Computed_Value_RNG) {
    .base = {
      .vtable = &gta_computed_value_rng_vtable,
      .context = context,
      .is_true = true,
      .is_error = false,
      .is_temporary = false,
      .requires_deep_copy = false,
      .is_singleton = false,
      .is_a_reference = false,
    },
    .seed = seed,
    .state = state,
  };
  return true;
}


void GTA_CALL gta_computed_value_rng_destroy(GTA_Computed_Value * self) {
  assert(self);
  gta_computed_value_rng_destroy_in_place(self);
  gcu_free(self);
}


void GTA_CALL gta_computed_value_rng_destroy_in_place(GTA_Computed_Value * self) {
  assert(self);
  GTA_Computed_Value_RNG * rng = (GTA_Computed_Value_RNG *)self;
  if ((GTA_Computed_Value *)rng == gta_computed_value_random_global) {
    return;
  }
  if (rng->state) {
    gcu_random_free((GCU_Random *)rng->state);
    rng->state = NULL;
  }
}


GTA_Computed_Value * GTA_CALL gta_computed_value_rng_deep_copy(GTA_Computed_Value * self, GTA_MAYBE_UNUSED(GTA_Execution_Context * context)) {
  return self;
}


char * GTA_CALL gta_computed_value_rng_to_string(GTA_MAYBE_UNUSED(GTA_Computed_Value * self)) {
  char * str = (char *)gcu_malloc(4);
  strcpy(str, "RNG");
  return str;
}


static GTA_UInteger GTA_CALL rng_get_default_seed(void) {
#ifdef _WIN32
  // Windows implementation.
  SYSTEMTIME time;
  GetSystemTime(&time);
  GTA_UInteger seed = ((GTA_UInteger)time.wSecond * 1000) + (GTA_UInteger)time.wMilliseconds;
#else
  // Unix implementation.
  struct timeval time;
  gettimeofday(&time, NULL);
  GTA_UInteger seed = ((GTA_UInteger)time.tv_sec * 1000) + ((GTA_UInteger)time.tv_usec / 1000);
#endif // _WIN32
  return seed;
}


static GCU_Random * GTA_CALL rng_engine(GTA_Computed_Value_RNG * self, bool * locked) {
  assert(self);
  *locked = (GTA_Computed_Value *)self == gta_computed_value_random_global;
  if (*locked) {
    gcu_semaphore_wait(&global_semaphore);
    if (!global_ready) {
      GCU_Random_Engine engine;
      assert(sizeof global_handle_storage >= gcu_random_handle_size());
      gcu_random_mt64_init(&global_mt, (uint64_t)self->seed);
      global_pending_off = 0;
      global_pending_len = 0;
      engine.ctx = &global_mt;
      engine.fill = global_fill;
      engine.destroy = NULL;
      self->state = gcu_random_place(global_handle_storage, sizeof global_handle_storage, &engine);
      assert(self->state);
      global_ready = 1;
    }
  }
  return (GCU_Random *)self->state;
}


static void GTA_CALL rng_unlock(bool locked) {
  if (locked) {
    gcu_semaphore_signal(&global_semaphore);
  }
}


static uint64_t GTA_CALL rng_get_next(GTA_Computed_Value_RNG * self) {
  bool locked = false;
  GCU_Random * engine = rng_engine(self, &locked);
  uint64_t result = 0;
  int rc = gcu_random_u64(engine, &result);
  rng_unlock(locked);
  if (rc != 0) {
    assert(rc == 0);
    return 0;
  }
  return result;
}


// .next_bool
static GTA_Computed_Value * GTA_CALL rng_next_bool(GTA_Computed_Value * self, GTA_MAYBE_UNUSED(GTA_Execution_Context * context)) {
  assert(self);
  assert(GTA_COMPUTED_VALUE_IS_RNG(self));
  return rng_get_next((GTA_Computed_Value_RNG *)self) & 1
    ? gta_computed_value_boolean_true
    : gta_computed_value_boolean_false;
}


// .next_int
static GTA_Computed_Value * GTA_CALL rng_next_int(GTA_Computed_Value * self, GTA_Execution_Context * context) {
  assert(self);
  assert(GTA_COMPUTED_VALUE_IS_RNG(self));
  return (GTA_Computed_Value *)gta_computed_value_integer_create((GTA_UInteger)rng_get_next((GTA_Computed_Value_RNG *)self), context);
}


// .next_float
static GTA_Computed_Value * GTA_CALL rng_next_float(GTA_Computed_Value * self, GTA_Execution_Context * context) {
  assert(self);
  assert(GTA_COMPUTED_VALUE_IS_RNG(self));
  bool locked = false;
  GTA_Computed_Value_RNG * rng = (GTA_Computed_Value_RNG *)self;
  GCU_Random * engine = rng_engine(rng, &locked);
  double value = 0.0;
  int rc = gcu_random_f64(engine, &value);
  rng_unlock(locked);
  if (rc != 0) {
    assert(rc == 0);
    return gta_computed_value_error_out_of_memory;
  }
  return (GTA_Computed_Value *)gta_computed_value_float_create((GTA_Float)value, context);
}


// .set_seed(seed)
static GTA_Computed_Value * GTA_CALL rng_set_seed(GTA_Computed_Value * self, GTA_Execution_Context * context) {
  assert(self);
  assert(GTA_COMPUTED_VALUE_IS_RNG(self));

  return (GTA_Computed_Value *)gta_computed_value_function_native_create(rng_set_seed_callback, self, context);
}


// .set_seed(seed) callback
static GTA_Computed_Value * GTA_CALL rng_set_seed_callback(GTA_Computed_Value * bound_object, GTA_UInteger argc, GTA_Computed_Value * argv[], GTA_MAYBE_UNUSED(GTA_Execution_Context * context)) {
  assert(bound_object);

  // Check the number of arguments.
  if (argc != 1) {
    return gta_computed_value_error_argument_count_mismatch;
  }

  // Check the type of the argument.
  assert(argv);
  if (!GTA_COMPUTED_VALUE_IS_INTEGER(argv[0])) {
    return gta_computed_value_error_invalid_function_call;
  }

  // Check if the object is the global RNG.
  if (bound_object == gta_computed_value_random_global) {
    return gta_computed_value_error_global_rng_seed_not_changeable;
  }

  GTA_Computed_Value_RNG * rng = (GTA_Computed_Value_RNG *)bound_object;
  GTA_Computed_Value_Integer * seed = (GTA_Computed_Value_Integer *)argv[0];

  // Set the seed.
  GCU_Random * next = gcu_random_mt64((uint64_t)seed->value);
  if (!next) {
    return gta_computed_value_error_out_of_memory;
  }
  rng->seed = seed->value;
  gcu_random_free((GCU_Random *)rng->state);
  rng->state = next;

  return bound_object;
}


/**
 * Setup things that cannot be done at compile time.
 */
GTA_INIT_FUNCTION(setup) {
  gta_computed_value_random_global_singleton.seed = rng_get_default_seed();
  gta_computed_value_rng_vtable.attributes_count = sizeof(attributes) / sizeof(GTA_Computed_Value_Attribute_Pair);

  if (gcu_semaphore_create(&global_semaphore, 1)) {
    // Error creating the semaphore.
    exit(1);
  }
}


/**
 * Cleanup things that cannot be done at compile time.
 */
GTA_CLEANUP_FUNCTION(teardown) {
  gcu_semaphore_destroy(&global_semaphore);
}
