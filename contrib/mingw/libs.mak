DEFINES=-DLIBCLAMAV_EXPORTS -D_LZMA_IN_CB -DPTW32_BUILD_INLINED -DPTW32_STATIC_LIB -DCLEANUP=__CLEANUP_C
include common.mak

gnulib_SOURCES=$(wildcard $(msvc)/gnulib/*.c)
gnulib_OBJECTS=$(gnulib_SOURCES:.c=.o)

libclamunrar_SOURCES=$(wildcard $(top)/libclamunrar/*.c)
libclamunrar_OBJECTS=$(libclamunrar_SOURCES:.c=.o)
libclamunrar_OBJECTS+=$(msvc)/resources/libclamunrar-rc.o
libclamunrar.dll: $(libclamunrar_OBJECTS)
	$(DLLWRAP) $(LDFLAGS) --def $(msvc)/libclamunrar.def --implib $@.a -o $@ $^

libclamunrar_iface_SOURCES=$(wildcard $(top)/libclamunrar_iface/*.c)
libclamunrar_iface_OBJECTS=$(libclamunrar_iface_SOURCES:.c=.o)
libclamunrar_iface_OBJECTS+=$(msvc)/resources/libclamunrar_iface-rc.o
libclamunrar_iface.dll: $(libclamunrar_iface_OBJECTS) $(gnulib_OBJECTS) libclamunrar.dll
	$(DLLWRAP) $(LDFLAGS) --def $(msvc)/libclamunrar_iface.def --implib $@.a -o $@ $^ libclamunrar.dll.a

# Libclamav
libclamav_SOURCES=$(wildcard $(top)/libclamav/*.c)
libclamav_SOURCES+=$(wildcard $(top)/libclamav/regex/*.c)
libclamav_SOURCES+=$(wildcard $(top)/libclamav/lzma/*.c)
libclamav_SOURCES+=$(wildcard $(top)/libclamav/nsis/*.c)
libclamav_SOURCES+=$(wildcard $(top)/libclamav/jsparse/js-norm.c)
libclamav_SOURCES+=$(wildcard $(top)/shared/*.c)
libclamav_SOURCES:=$(subst $(top)/shared/actions.c,,$(libclamav_SOURCES))
libclamav_SOURCES+=$(msvc)/pthreads/pthread.c
libclamav_SOURCES+=$(wildcard $(msvc)/src/dllmain/*.c)

libclamav_SOURCES+=$(wildcard $(msvc)/zlib/*.c)
libclamav_SOURCES+=$(wildcard $(msvc)/bzip2/*.c)

libclamav_SOURCES+=$(top)/libclamav/7zip/7zip.c \
$(top)/libclamav/7zip/lzma/Archive/7z/7zAlloc.c \
$(top)/libclamav/7zip/lzma/Archive/7z/7zBuffer.c \
$(top)/libclamav/7zip/lzma/7zCrc.c \
$(top)/libclamav/7zip/lzma/Archive/7z/7zDecode.c \
$(top)/libclamav/7zip/lzma/Archive/7z/7zExtract.c \
$(top)/libclamav/7zip/lzma/Archive/7z/7zHeader.c \
$(top)/libclamav/7zip/lzma/Archive/7z/7zIn.c \
$(top)/libclamav/7zip/lzma/Archive/7z/7zItem.c \
$(top)/libclamav/7zip/lzma/Archive/7z/7zMethodID.c \
$(top)/libclamav/7zip/lzma/Compress/Lzma/LzmaDecode.c \
$(top)/libclamav/7zip/lzma/Compress/Branch/BranchX86.c \
$(top)/libclamav/7zip/lzma/Compress/Branch/BranchX86_2.c

# Exclusions
libclamav_SOURCES:=$(subst $(top)/libclamav/regex/engine.c,,$(libclamav_SOURCES))

libclamav_OBJECTS=$(libclamav_SOURCES:.c=.o)
libclamav_OBJECTS+=$(msvc)/resources/libclamav-rc.o
libclamav.dll: $(libclamav_OBJECTS) $(gnulib_OBJECTS)
	$(DLLWRAP) $(LDFLAGS) --def $(msvc)/libclamav.def --implib $@.a -o $@ $^ -lws2_32

clean:
	@-rm -f *.dll *.a $(gnulib_OBJECTS) $(libclamunrar_OBJECTS) $(libclamunrar_iface_OBJECTS) $(libclamav_OBJECTS)
	@echo Object files removed
