#include <osdeps.h>

#define poll_with_event poll_with_event_threadpool
#define RegisterWaitForSingleObject cw_helpers.k32.RegisterWaitForSingleObject
#define UnregisterWaitEx cw_helpers.k32.UnregisterWaitEx

#include <win32/compat/net.c>
#undef poll_with_event

static int poll_with_event_compat(struct pollfd *fds, int nfds, int timeout, HANDLE event)
{
    return 0;
}

int poll_with_event(struct pollfd *fds, int nfds, int timeout, HANDLE event)
{
    if (cw_helpers.k32.tpool)
        return poll_with_event_threadpool(fds, nfds, timeout, event);
    else
        return poll_with_event_compat(fds, nfds, timeout, event);
}
