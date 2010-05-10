include common.mak
CFLAGS+=-DPTW32_STATIC_LIB -D_WINDLL

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
libclamav_SOURCES+=$(wildcard $(msvc)/src/dllmain/*.c)
libclamav_SOURCES+=$(top)/libclamav/jsparse/js-norm.c

libclamav_SOURCES+=$(top)/win32/3rdparty/pthreads/pthread.c
libclamav_SOURCES+=$(addprefix $(top)/win32/3rdparty/bzip2/,blocksort.c bzlib.c compress.c \
	crctable.c decompress.c huffman.c randtable.c)
libclamav_SOURCES+=$(addprefix $(top)/win32/3rdparty/zlib/,adler32.c compress.c crc32.c \
	deflate.c gzio.c infback.c inffast.c inflate.c inftrees.c trees.c uncompr.c zutil.c)

libclamav_SOURCES+=$(wildcard $(top)/libclamav/7z/*.c)
libclamav_SOURCES+=$(wildcard $(top)/libclamav/7z/Archive/7z/*.c)

libclamav_SOURCES:=$(subst $(top)/libclamav/regex/engine.c,,$(libclamav_SOURCES))
libclamav_SOURCES:=$(subst $(top)/libclamav/bytecode_nojit.c,,$(libclamav_SOURCES))
libclamav_SOURCES:=$(subst $(top)/libclamav/others.c,$(msvc)/src/dllmain/win32others.c,$(libclamav_SOURCES))

libclamav_OBJECTS=$(libclamav_SOURCES:.c=.o)
libclamav_OBJECTS+=$(msvc)/resources/libclamav-rc.o
libclamav.dll: $(libclamav_OBJECTS) $(gnulib_OBJECTS)
	$(DLLWRAP) $(LDFLAGS) --def $(msvc)/libclamav.def --implib $@.a -o $@ $^ -lws2_32

# LLVM
include llvm.mak
libclamav_llvm_OBJECTS=$(libclamav_llvm_SOURCES:.cpp=.o)
libclamav_llvm_OBJECTS:=$(libclamav_llvm_OBJECTS:.c=.o)
libclamav_llvm_OBJECTS+=$(msvc)/resources/libclamav_llvm-rc.o
libclamav_llvm.dll: $(libclamav_llvm_OBJECTS) libclamav.dll.a
	$(DLLWRAP) --driver-name=$(CXX) $(LDFLAGS) --def $(msvc)/libclamav_llvm.def -o $@ $^ -limagehlp -lpsapi

llvm: libclamav_llvm.dll
llvm: CFLAGS+=-fno-omit-frame-pointer

llvm-clean:
	@rm -f libclamav_llvm.dll $(libclamav_llvm_OBJECTS)
	@echo LLVM objects cleaned

clean:
	@rm -f libclamav.dll libclamav.dll.a
	@rm -f $(CLAMAV_LIBS) $(addsuffix .a,$(CLAMAV_LIBS))
	@rm -f $(gnulib_OBJECTS) $(libclamunrar_OBJECTS) $(libclamunrar_iface_OBJECTS) $(libclamav_OBJECTS)
	@echo Project cleaned
