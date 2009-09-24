/*
 * Clamav Native Windows Port: dummy definition of system resources limits
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

#ifndef _SYS_RESOURCE_H_
#define _SYS_RESOURCE_H_

#define RLIMIT_FSIZE 1
#define RLIMIT_CORE 2

typedef unsigned long int rlim_t;
struct rlimit { rlim_t rlim_cur; };

inline int getrlimit(int resource, struct rlimit *rlim) { rlim->rlim_cur = -1; return 0; }

#endif /* _SYS_RESOURCE_H_ */
