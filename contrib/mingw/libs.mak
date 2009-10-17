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
libclamav_SOURCES+=$(msvc)/pthreads/pthread.c
libclamav_SOURCES+=$(wildcard $(msvc)/src/dllmain/*.c)

libclamav_SOURCES+=$(wildcard $(msvc)/zlib/*.c)
libclamav_SOURCES+=$(wildcard $(msvc)/bzip2/*.c)

libclamav_SOURCES+=$(wildcard $(top)/libclamav/7z/*.c)
libclamav_SOURCES+=$(wildcard $(top)/libclamav/7z/Archive/7z/*.c)

# Exclusions
libclamav_SOURCES:=$(subst $(top)/libclamav/regex/engine.c,,$(libclamav_SOURCES))

libclamav_OBJECTS=$(libclamav_SOURCES:.c=.o)
libclamav_OBJECTS+=$(msvc)/resources/libclamav-rc.o
libclamav.dll: $(libclamav_OBJECTS) $(gnulib_OBJECTS)
	$(DLLWRAP) $(LDFLAGS) --def $(msvc)/libclamav.def --implib $@.a -o $@ $^ -lz -lbz2 -lws2_32

clean:
	@-rm -f *.dll *.a $(gnulib_OBJECTS) $(libclamunrar_OBJECTS) $(libclamunrar_iface_OBJECTS) $(libclamav_OBJECTS)
	@echo Object files removed
