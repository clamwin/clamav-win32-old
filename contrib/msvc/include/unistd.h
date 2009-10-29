/*
 * Clamav Native Windows Port: unistd.h for msvc
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

#ifndef _UNISTD_H
#define _UNISTD_H

#include <io.h>
#include <sys/stat.h>

#ifndef __GNUC__
#define R_OK    4               /* Test for read permission */
#define W_OK    2               /* Test for write permission */
#define X_OK    0               /* Test for execute permission - 1 bombs msvcrt */
#define F_OK    0               /* Test for existence */
#endif

#define sleep(x) Sleep(x * 1000)
#define alarm(x)
#define pause()

extern int ftruncate(int fd, off_t length);

#define chown(a, b, c) ((void) (0))

#define getuid()  (0)
#define geteuid() (0)
#define getgid()  (0)
#define setgid(x) (0)
#define setuid(x) (0)

struct sigaction { void *sa_handler; };

#define sigprocmask(a, b, c)
#define sigaction(a, b, c)
#define sigfillset(x)
#define sigemptyset(x)
#define sigaddset(a, b)
#define sigdelset(a, b)
#define pthread_sigmask(a, b, c)

#endif /* _UNISTD_H */
