#include <osdeps.h>

#define RegisterWaitForSingleObject cw_helpers.k32.RegisterWaitForSingleObject
#define UnregisterWaitEx cw_helpers.k32.UnregisterWaitEx

#define wsock2errno cw_wseterrno
#include <win32/compat/net.c>
