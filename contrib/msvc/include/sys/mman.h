/*
 * Clamav Native Windows Port: mman.h (mmap) for msvc
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

#ifndef _SYS_MMAN_H
#define _SYS_MMAN_H

#include <sys/types.h>

#define PROT_READ   0x1     /* Page can be read */
#define PROT_WRITE  0x2     /* Page can be written */
/* #define PROT_EXEC   0x4 */    /* Page can be executed */
/* #define PROT_NONE   0x0 */    /* Page can not be accessed */
#define PROT_EXEC   mmap_prot_exec_not_implemented
#define PROT_NONE   mmap_prot_none_not_implemented

#define MAP_SHARED  0x01    /* Share changes */
#define MAP_PRIVATE 0x02    /* Changes are private */
/* #define MAP_FIXED   0x10 */   /* Interpret addr exactly */
#define MAP_FIXED   mmap_map_fixed_not_implemented

/* Return value of 'mmap' in case of an error */
#define MAP_FAILED  ((void *) -1)

extern void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
extern int munmap(void *addr, size_t len);

#endif /* _SYS_MMAN_H */
