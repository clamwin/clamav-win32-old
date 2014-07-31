top=../..
msvc=$(top)/contrib/msvc

CFLAGS=-I$(msvc) -I$(msvc)/include -I$(msvc)/gnulib
CFLAGS+=-I$(top) -I$(top)/shared -I$(top)/libclamav -I$(top)/libclamav/nsis
CFLAGS+=-I$(top)/win32/3rdparty/bzip2 -I$(top)/win32/3rdparty/pthreads -I$(top)/win32/3rdparty/zlib
CFLAGS+=-DHAVE_CONFIG_H -DNDEBUG -DWIN32_LEAN_AND_MEAN
CFLAGS+=-Wall
CFLAGS+=-Wno-unused -Wno-format -Wno-uninitialized -Wno-attributes
CFLAGS+=-Wno-switch
CFLAGS+=-pipe -fno-strict-aliasing -mno-ms-bitfields
CFLAGS+=-O3 -mtune=generic -fomit-frame-pointer

LDFLAGS=-Wl,--enable-stdcall-fixup

LLVM=-I$(top)/libclamav/c++ -I$(top)/libclamav/c++/llvm/include
LLVM+=-I$(top)/libclamav/c++/llvm/lib/Target/X86 -I$(msvc)/include/llvmbuild
LLVM+=-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS

CC=$(MINGW32_CROSS_PREFIX)gcc
CXX=$(MINGW32_CROSS_PREFIX)g++
WINDRES=$(MINGW32_CROSS_PREFIX)windres
DLLWRAP=$(MINGW32_CROSS_PREFIX)dllwrap
AR=$(MINGW32_CROSS_PREFIX)ar
RANLIB=$(MINGW32_CROSS_PREFIX)ranlib

ifneq (,$(findstring x86_64,$(shell $(CC) -dumpmachine)))
CFLAGS+=-D_TIMESPEC_DEFINED -D_TIMEZONE_DEFINED
else
CFLAGS+=-march=i686
endif

CLAMAV_PROGRAMS=clamd.exe clamdscan.exe clamscan.exe freshclam.exe sigtool.exe clambc.exe
CLAMAV_LIBS=libclamunrar.dll libclamunrar_iface.dll
CLAMAV_TOOLS=profiler.exe exeScanner.exe sigcheck.exe

%.o: %.c
	$(CC) $(CFLAGS) -Wno-pointer-sign $(DEFINES) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CFLAGS) $(DEFINES) $(LLVM) -c -o $@ $<

%-rc.o: %.rc
	$(WINDRES) -I$(msvc)/resources -I$(msvc)/tools -o $@ $<
