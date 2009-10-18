/*
 * Clamav Native Windows Port: Win32 Poll replacement for clamd
 *
 * Copyright (c) 2009 Gianluigi Tiesi <sherpya@netfarm.it>
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

#include <platform.h>
#include <clamd/others.h>
#include <shared/output.h>

extern int read_fd_data(struct fd_buf *buf);

int fds_poll_recv(struct fd_data *data, int timeout, int check_signals)
{
    WSANETWORKEVENTS netev;
    DWORD res;
    int i, ret = -1;
    int num = 0;
    DWORD msec = (timeout == -1) ? INFINITE : (timeout * 1000);

    pthread_mutex_unlock(data->buf_mutex);

    if ((data->nfds < 1) || (data->nfds >= WAIT_TIMEOUT))
    {
        pthread_mutex_lock(data->buf_mutex);
        logg("!Invalid number of file descriptors %d\n", data->nfds);
        return -1;
    }

    for (i = 0; i < data->nfds; i++)
    {
        data->buf[i].got_newdata = 0;
        data->events[i] = data->buf[i].event;
    }

    switch ((res = WaitForMultipleObjects((DWORD) data->nfds, data->events, FALSE, msec)))
    {
        case WAIT_TIMEOUT:
            ret = 0;
            break;
        case WAIT_FAILED:
            ret = -1;
            break;
        default:
            if (res > (WAIT_OBJECT_0 + data->nfds))
            {
                logg("!WaitForMultipleObjects() failed %d\n", GetLastError());
                ret = -1;
            }
            else
            {
                ret = 0;
                for (i = res - WAIT_OBJECT_0; i < data->nfds; i++)
                {
                    /* non sockets */
                    if (!data->buf[i].fd)
                    {
                        ResetEvent(data->buf[i].event);
                        continue;
                    }

                    if (WSAEnumNetworkEvents((SOCKET) data->buf[i].fd, data->buf[i].event, &netev) == SOCKET_ERROR)
                    {
                        logg("!WSAEnumNetworkEvents() failed %d\n", WSAGetLastError());
                        continue;
                    }

                    ResetEvent(data->buf[i].event);

                    if (netev.lNetworkEvents & FD_ACCEPT)
                    {
                        data->buf[i].got_newdata = 1;
                        ret++;
                        continue;
                    }
                    if (netev.lNetworkEvents & FD_CLOSE)
                    {
                        data->buf[i].got_newdata = -1;
                        ret++;
                        continue;
                    }
                    if (netev.lNetworkEvents & FD_READ)
                    {
                        read_fd_data(&data->buf[i]);
                        ret++;
                        continue;
                    }
                }
            }
            break;
    }
    pthread_mutex_lock(data->buf_mutex);
    return ret;
}

int poll_fd(int fd, int timeout_sec, int check_signals)
{
    int ret;
    struct timeval timeout;
    struct timeval *z = NULL;
    FD_SET Reader;
    FD_SET Error;

    FD_ZERO(&Reader);
    FD_ZERO(&Error);

    FD_SET(fd, &Reader);
    FD_SET(fd, &Error);

    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;

    ret = select(42, &Reader, NULL, &Error, (timeout_sec == -1) ? NULL : &timeout);

    if ((ret == SOCKET_ERROR) || FD_ISSET(fd, &Error))
        return -1;

    if ((ret > 0) && FD_ISSET(fd, &Reader))
        return 1;

    return 0;
}


