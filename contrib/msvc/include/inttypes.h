/*
 * Clamav Native Windows Port: inttypes.h for msvc
 *
 * Copyright (c) 2008 Gianluigi Tiesi <sherpya@netfarm.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; if not, write to the
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _INTTYPES_H_
#define _INTTYPES_H_
#include <sys/stat.h>

#ifdef __GNUC__
#include <stdint.h>
#else

typedef unsigned __int64 uint64_t;
typedef signed   __int64 int64_t;

typedef unsigned __int32 uint32_t;
typedef signed   __int32 int32_t;

typedef unsigned __int16 uint16_t;
typedef signed   __int16 int16_t;

typedef unsigned __int8  uint8_t;
typedef signed   __int8  int8_t;

typedef uint64_t u_int64_t;
typedef uint32_t u_int32_t;
typedef uint16_t u_int16_t;
typedef uint8_t  u_int8_t;

typedef unsigned long int uintmax_t;
typedef long int intmax_t;

#ifndef __cplusplus
typedef uint32_t mode_t;
#endif

#ifdef  _WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif

#endif /* __GNUC__ */

#endif /* _INTTYPES_H */
