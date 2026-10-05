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
 * Header file for the computedValueBoolean class.
 */

#ifndef GHOTI_IO_GTA_COMPUTEDVALUE_COMPUTEDVALUEBOOLEAN_H
#define GHOTI_IO_GTA_COMPUTEDVALUE_COMPUTEDVALUEBOOLEAN_H

#include <ghoti.io/tang/macros.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <ghoti.io/tang/computedValue/computedValue.h>

/**
 * The vtable for the GTA_Computed_Value_Boolean class.
 */
GTA_API extern GTA_Computed_Value_VTable gta_computed_value_boolean_vtable;

/**
 * A singleton representing a true value.
 */
GTA_API extern GTA_Computed_Value * gta_computed_value_boolean_true;

/**
 * A singleton representing a false value.
 */
GTA_API extern GTA_Computed_Value * gta_computed_value_boolean_false;

/**
 * The GTA_Computed_Value_Boolean class.
 */
struct GTA_Computed_Value_Boolean {
  GTA_Computed_Value base; ///< The common computed-value header.
  bool value;              ///< The boolean held.
};

/**
 * Initializes a GTA_Computed_Value_Boolean object.
 *
 * @param value The boolean value of the computed value.
 * @param context The execution context in which to create the value.
 * @return The initialized object or NULL if an error occurred.
 */
GTA_API GTA_Computed_Value_Boolean * GTA_CALL gta_computed_value_boolean_create(bool value, GTA_Execution_Context * context);

/**
 * Create a computed value for a boolean in place.
 *
 * @param self The memory address of the computed value.
 * @param value The boolean value of the computed value.
 * @param context The execution context in which to create the value.
 * @return True if the operation was successful, false otherwise.
 */
GTA_API bool GTA_CALL gta_computed_value_boolean_create_in_place(GTA_Computed_Value_Boolean * self, bool value, GTA_Execution_Context * context);

/**
 * Destroys a GTA_Computed_Value_Boolean object.
 *
 * This function should not be called directly. Instead, use the
 * gta_computed_value_destroy() function.
 *
 * @see gta_computed_value_destroy
 *
 * @param self The object to destroy.
 */
GTA_API void GTA_CALL gta_computed_value_boolean_destroy(GTA_Computed_Value * self);

/**
 * Destroy a computed value for a boolean in place.
 *
 * @see gta_computed_value_destroy_in_place
 *
 * @param self The memory address of the computed value.
 */
GTA_API void GTA_CALL gta_computed_value_boolean_destroy_in_place(GTA_Computed_Value * self);

/**
 * Deep copy a computed value for a boolean.
 *
 * @see gta_computed_value_deep_copy
 *
 * @param value The computed value to be copied.
 * @param context The execution context of the program.
 * @return The deep copy of the GTA_Computed_Value_Boolean or NULL if an error occurred.
 */
GTA_API GTA_NO_DISCARD GTA_Computed_Value * GTA_CALL gta_computed_value_boolean_deep_copy(GTA_Computed_Value * value, GTA_Execution_Context * context);

/**
 * Get a string representation of the computed value for a boolean.
 *
 * The caller is responsible for freeing the returned string.
 *
 * @see gta_computed_value_to_string
 *
 * @param self The computed value for the boolean.
 * @return The string representation of the computed value for the boolean.
 */
GTA_API GTA_NO_DISCARD char * GTA_CALL gta_computed_value_boolean_to_string(GTA_Computed_Value * self);

/**
 * Casts a computed value to a different type.
 *
 * Calls the `cast` method of the virtual table.
 *
 * @param self The object to cast.
 * @param type The type to cast to.
 * @param context The execution context of the program.
 * @return The result of the operation or NULL if the operation failed.
 */
GTA_API GTA_NO_DISCARD GTA_Computed_Value * GTA_CALL gta_computed_value_boolean_cast(GTA_Computed_Value * self, GTA_Computed_Value_VTable * type, GTA_Execution_Context * context);

/**
 * Compares a boolean against another value for equality.
 *
 * Equality is strict: `true == 1` is false.  Booleans have no ordering, so
 * there is nothing here for `<` and friends.
 *
 * @param self The boolean.
 * @param other The value to compare against.
 * @param self_is_lhs Whether `self` is the left-hand side of the operation.
 * @param context The execution context of the program.
 * @return The result of the operation or NULL if the operation failed.
 */
GTA_API GTA_NO_DISCARD GTA_Computed_Value * GTA_CALL gta_computed_value_boolean_equal(GTA_Computed_Value * self, GTA_Computed_Value * other, bool self_is_lhs, GTA_Execution_Context * context);

/**
 * Compares a boolean against another value for inequality.
 *
 * @param self The boolean.
 * @param other The value to compare against.
 * @param self_is_lhs Whether `self` is the left-hand side of the operation.
 * @param context The execution context of the program.
 * @return The result of the operation or NULL if the operation failed.
 */
GTA_API GTA_NO_DISCARD GTA_Computed_Value * GTA_CALL gta_computed_value_boolean_not_equal(GTA_Computed_Value * self, GTA_Computed_Value * other, bool self_is_lhs, GTA_Execution_Context * context);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // GHOTI_IO_GTA_COMPUTEDVALUE_COMPUTEDVALUEBOOLEAN_H
