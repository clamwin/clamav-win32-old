top=../..
msvc=$(top)/contrib/msvc

CFLAGS=-I$(msvc) -I$(msvc)/include -I$(msvc)/gnulib
CFLAGS+=-I$(top) -I$(top)/shared -I$(top)/libclamav -I$(top)/libclamav/nsis
CFLAGS+=-I$(top)/win32/3rdparty/bzip2 -I$(top)/win32/3rdparty/pthreads -I$(top)/win32/3rdparty/zlib
CFLAGS+=-DHAVE_CONFIG_H
CFLAGS+=-Wall -Wextra -Wno-unused -Wno-sign-compare -Wno-switch -Wno-format -pipe
CFLAGS+=-fno-strict-aliasing
CFLAGS+=-O3 -mtune=generic -march=i686 -fomit-frame-pointer -ffast-math

LLVM=-I$(top)/libclamav/c++ -I$(top)/libclamav/c++/llvm/include -I$(top)/libclamav/c++/llvm/lib/Target/X86 -I$(top)/win32/llvmbuild/include
LLVM+=-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -DHAVE_INTTYPES_H -DHAVE_UINT64_T -DHAVE_ISINF_IN_MATH_H -DHAVE_ISNAN_IN_MATH_H

CC=$(MINGW32_CROSS_PREFIX)gcc
CXX=$(MINGW32_CROSS_PREFIX)g++
WINDRES=$(MINGW32_CROSS_PREFIX)windres
DLLWRAP=$(MINGW32_CROSS_PREFIX)dllwrap
AR=$(MINGW32_CROSS_PREFIX)ar
RANLIB=$(MINGW32_CROSS_PREFIX)ranlib

CLAMAV_PROGRAMS=clamd.exe clamdscan.exe clamscan.exe freshclam.exe sigtool.exe
CLAMAV_LIBS=libclamunrar.dll libclamunrar_iface.dll libclamav_llvm.dll
CLAMAV_TOOLS=profiler.exe exeScanner.exe

%.o: %.c
	$(CC) $(CFLAGS) -Wno-pointer-sign $(DEFINES) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CFLAGS) $(DEFINES) $(LLVM) -c -o $@ $<

%-rc.o: %.rc
	$(WINDRES) -I$(msvc)/resources -I$(msvc)/tools -o $@ $<
