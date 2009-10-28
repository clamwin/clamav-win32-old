top=../..
msvc=$(top)/contrib/msvc

CFLAGS=-I$(msvc) -I$(msvc)/include -I$(msvc)/gnulib -I$(msvc)/pthreads
CFLAGS+=-I$(top) -I$(top)/shared -I$(top)/libclamav -I$(top)/libclamav/lzma -I$(top)/libclamav/nsis
CFLAGS+=-DHAVE_CONFIG_H
CFLAGS+=-Wall -Wextra -Wno-unused -Wno-sign-compare -Wno-switch -Wno-pointer-sign -Wno-format -pipe
CFLAGS+=-fno-strict-aliasing
CFLAGS+=-O2 -mtune=generic -march=i686 -fomit-frame-pointer -ffast-math

CC=$(MINGW32_CROSS_PREFIX)gcc
WINDRES=$(MINGW32_CROSS_PREFIX)windres
DLLWRAP=$(MINGW32_CROSS_PREFIX)dllwrap
AR=$(MINGW32_CROSS_PREFIX)ar
RANLIB=$(MINGW32_CROSS_PREFIX)ranlib

CLAMAV_PROGRAMS=clamd.exe clamdscan.exe clamscan.exe freshclam.exe sigtool.exe
CLAMAV_LIBS=libclamunrar.dll libclamunrar_iface.dll
CLAMAV_TOOLS=profiler.exe exeScanner.exe

%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

%-rc.o: %.rc
	$(WINDRES) -I$(msvc)/resources -I$(msvc)/tools -o $@ $<
