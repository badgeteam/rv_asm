#pragma once

#include"common_types.h"

// Adds a Constant to the List of Constants
void addConstant(CompContext*ctx, Token*nameToken, Token*valueToken);

// Returns the valueToken of a constant or NULL
Token*getConstant(CompContext*ctx, Token*nameToken);

/* If the current Token is a Number,
 *   return the number
 * If the current Token is an Identifier,
 *   return the associated constant.
 */
Token*getNumberOrConstant(CompContext*ctx);


