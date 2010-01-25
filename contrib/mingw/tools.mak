include common.mak

shared_SOURCES+=$(wildcard $(top)/shared/*.c)
shared_SOURCES:=$(subst $(top)/shared/actions.c,$(msvc)/src/shared/win32actions.c,$(shared_SOURCES))
shared_SOURCES:=$(subst $(top)/shared/optparser.c,,$(shared_SOURCES))
shared_OBJECTS=$(shared_SOURCES:.c=.o)

clamd_SOURCES=$(wildcard $(top)/clamd/*.c)
clamd_SOURCES+=$(msvc)/src/helpers/cw_main.c
clamd_SOURCES+=$(msvc)/src/helpers/service.c
clamd_SOURCES+=$(msvc)/src/helpers/win32poll.c
clamd_SOURCES:=$(subst $(top)/clamd/localserver.c,,$(clamd_SOURCES))
clamd_SOURCES:=$(subst $(top)/clamd/dazukofs.c,,$(clamd_SOURCES))
clamd_OBJECTS=$(clamd_SOURCES:.c=.o)
clamd_OBJECTS+=$(msvc)/resources/clamd-rc.o
clamd.exe: libclamav.dll $(clamd_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32

clamdscan_SOURCES=$(wildcard $(top)/clamdscan/*.c)
clamdscan_SOURCES+=$(msvc)/src/helpers/cw_main.c
clamdscan_SOURCES+=$(msvc)/src/helpers/dresult.c
clamdscan_OBJECTS=$(clamdscan_SOURCES:.c=.o)
clamdscan_OBJECTS+=$(msvc)/resources/clamdscan-rc.o
clamdscan.exe: libclamav.dll $(clamdscan_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32

clamscan_SOURCES=$(wildcard $(top)/clamscan/*.c)
clamscan_SOURCES+=$(msvc)/src/helpers/cw_main.c
clamscan_SOURCES+=$(msvc)/src/helpers/scanmem.c
clamscan_SOURCES+=$(msvc)/src/helpers/exeScanner.c
clamscan_OBJECTS=$(clamscan_SOURCES:.c=.o)
clamscan_OBJECTS+=$(msvc)/resources/clamscan-rc.o
clamscan.exe: libclamav.dll $(clamscan_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32

freshclam_SOURCES=$(wildcard $(top)/freshclam/*.c)
freshclam_SOURCES+=$(msvc)/src/helpers/cw_main.c
freshclam_SOURCES+=$(msvc)/src/helpers/service.c
freshclam_SOURCES+=$(msvc)/src/helpers/dnsquery.c
freshclam_SOURCES:=$(subst $(top)/freshclam/dns.c,,$(freshclam_SOURCES))
freshclam_OBJECTS=$(freshclam_SOURCES:.c=.o)
freshclam_OBJECTS+=$(msvc)/resources/freshclam-rc.o
freshclam.exe: libclamav.dll $(freshclam_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32 -liphlpapi

sigtool_SOURCES=$(wildcard $(top)/sigtool/*.c)
sigtool_SOURCES+=$(msvc)/src/helpers/cw_main.c
sigtool_OBJECTS=$(sigtool_SOURCES:.c=.o)
sigtool_OBJECTS+=$(msvc)/resources/sigtool-rc.o
sigtool.exe: libclamav.dll $(sigtool_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32

clambc_SOURCES=$(wildcard $(top)/clambc/*.c)
clambc_SOURCES+=$(msvc)/src/helpers/cw_main.c
clambc_OBJECTS=$(clambc_SOURCES:.c=.o)
clambc_OBJECTS+=$(msvc)/resources/clambc-rc.o
clambc.exe: libclamav.dll $(clambc_OBJECTS) $(shared_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lws2_32

profiler_SOURCES=$(msvc)/tools/profiler.c
profiler_OBJECTS=$(msvc)/tools/profiler.o
profiler_OBJECTS+=$(msvc)/tools/profiler-rc.o
profiler.exe: libclamav.dll $(profiler_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ libclamav.dll.a -lwinmm

exeScanner_OBJECTS+=$(msvc)/tools/exeScanner-rc.o
exeScanner.exe: $(exeScanner_OBJECTS) $(msvc)/tools/exeScanner_app.c $(msvc)/src/helpers/exeScanner.c
	$(CC) $(CFLAGS) -DEXESCANNER_STANDALONE $(LDFLAGS) $(msvc)/tools/exeScanner_app.c $(msvc)/src/helpers/exeScanner.c $(exeScanner_OBJECTS) -o $@

clean:
	@-rm -f *.exe $(clamd_OBJECTS) $(clamdscan_OBJECTS) $(clamscan_OBJECTS) $(freshclam_OBJECTS) $(sigtool_OBJECTS) $(clambc_OBJECTS) $(shared_OBJECTS)
	@-rm -f $(profiler_OBJECTS) $(exeScanner_OBJECTS)
	@echo Object files removed
