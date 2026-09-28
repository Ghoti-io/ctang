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
 * Tang's random values. The generator is MT19937-64, the C++ std::mt19937_64
 * sequence, on every architecture. The handle is CUtil's GCU_Random. The
 * global generator is shared under Tang's own semaphore.
 */

#ifndef GHOTI_IO_GTA_COMPUTEDVALUE_COMPUTEDVALUERNG_H
#define GHOTI_IO_GTA_COMPUTEDVALUE_COMPUTEDVALUERNG_H

#include <ghoti.io/tang/macros.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * A singleton representing the global random number generator.
 */
GTA_API extern GTA_Computed_Value * gta_computed_value_random_global;

/**
 * The VTable for the ComputedValueRNG class.
 */
GTA_API extern GTA_Computed_Value_VTable gta_computed_value_rng_vtable;

/**
 * The computed value for a random number generator.
 */
struct GTA_Computed_Value_RNG {
  /**
   * The base class for the computed value.
   */
  GTA_Computed_Value base;
  /**
   * The seed for the random number generator.
   */
  GTA_UInteger seed;
  /**
   * A GCU_Random handle, MT19937-64. NULL until the global generator is first used.
   */
  void * state;
};

/**
 * Create a new computed value for a random number generator with the provided
 * seed.
 *
 * @param seed The seed for the random number generator.
 * @param context The execution context to create the value in.
 * @return The new computed value for the random number generator.
 */
GTA_API GTA_NO_DISCARD GTA_Computed_Value_RNG * GTA_CALL gta_computed_value_rng_create_seeded(GTA_UInteger seed, GTA_Execution_Context * context);

/**
 * Create a new computed value for a random number generator with a default
 * seed.
 *
 * The seed is generated from the current time.
 *
 * @param context The execution context to create the value in.
 * @return The new computed value for the random number generator.
 */
GTA_API GTA_NO_DISCARD GTA_Computed_Value_RNG * GTA_CALL gta_computed_value_rng_create(GTA_Execution_Context * context);

/**
 * Create a new computed value for a random number generator in place with the
 * provided seed.
 *
 * @param self The memory address of the computed value.
 * @param seed The seed for the random number generator.
 * @param context The execution context to create the value in.
 * @return True if the operation was successful, false otherwise.
 */
GTA_API bool GTA_CALL gta_computed_value_rng_create_seeded_in_place(GTA_Computed_Value_RNG * self, GTA_UInteger seed, GTA_Execution_Context * context);

/**
 * Destroy a computed value for a random number generator.
 *
 * @see gta_computed_value_destroy
 *
 * @param self The computed value to be destroyed.
 */
GTA_API void GTA_CALL gta_computed_value_rng_destroy(GTA_Computed_Value * self);

/**
 * Destroy a computed value for a random number generator in place.
 *
 * @see gta_computed_value_destroy_in_place
 *
 * @param self The computed value to be destroyed.
 */
GTA_API void GTA_CALL gta_computed_value_rng_destroy_in_place(GTA_Computed_Value * self);

/**
 * Deep copy a computed value for a random number generator.
 *
 * @see gta_computed_value_deep_copy
 *
 * @param self The computed value for the random number generator.
 * @param context The execution context to create the value in.
 * @return The deep copy of the computed value.
 */
GTA_API GTA_NO_DISCARD GTA_Computed_Value * GTA_CALL gta_computed_value_rng_deep_copy(GTA_Computed_Value * self, GTA_Execution_Context * context);

/**
 * Get a string representation of the computed value for a random number
 * generator.
 *
 * The caller is responsible for freeing the returned string.
 *
 * @see gta_computed_value_to_string
 *
 * @param self The computed value for the random number generator.
 * @return The string representation of the computed value for the random number
 *   generator.
 */
GTA_API GTA_NO_DISCARD char * GTA_CALL gta_computed_value_rng_to_string(GTA_Computed_Value * self);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // GHOTI_IO_GTA_COMPUTEDVALUE_COMPUTEDVALUERNG_H
