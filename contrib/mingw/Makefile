include common.mak

all: libclamav.dll $(CLAMAV_LIBS) $(CLAMAV_PROGRAMS) $(CLAMAV_TOOLS)

libclamav.dll $(CLAMAV_LIBS):
	make -f libs.mak $@

$(CLAMAV_PROGRAMS) $(CLAMAV_TOOLS):
	make -f tools.mak $@

clean:
	make -f libs.mak clean
	make -f tools.mak clean

.PHONY: clean