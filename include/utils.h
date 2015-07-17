/* Jakopter
 * Copyright © 2015 ALF@INRIA
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef JAKOPTER_UTILS_H
#define JAKOPTER_UTILS_H

/* C-style public-like keyword */
#if defined(__GNUC__) && __GNUC__ >= 4
#define JAKO_EXPORT __attribute__ ((visibility("default")))
#else
#define JAKO_EXPORT
#endif

#define JAKO_LUA_API_VERSION 1.1
#define JAKO_C_API_VERSION 1.1

/* Max number of digit into an integer plus the sign and \0. */
#define INT_LEN     13
/* 2 integers plus the dot and \0*/
#define TSTAMP_LEN  2*INT_LEN+2
/* Max number of digit we use in a float (nb digits of an int + 6 decimal digits) and \0*/
#define FLOAT_LEN INT_LEN+6

#endif