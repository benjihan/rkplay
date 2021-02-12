#! /usr/bin/make -f
#
# ----------------------------------------------------------------------
#
# rkplay - Makefile
#
# by Ben G. AKA Ben/OVR
#
# ----------------------------------------------------------------------

$(info DEBUG=$(or $D,-DNDEBUG=1))

objects = rklib.o rkload.o

vpath %.c src

all: rkplay
clean:; rm -f -- rkplay $(objects)
rkplay: LDLIBS=$(shell $(or $(PKGCONFIG),pkg-config) ao --cflags --libs)
rkplay: CPPFLAGS += $(if $D,,-DNDEBUG=1)
rkplay: $(objects)
rklib.o: CPPFLAGS += -DVERSION='"$(shell date -u +%F)"'
.PHONY: clean all

# Dependencies
MAKEFILE = $(lastword $(MAKEFILE_LIST))
rklib.o: src/rklib.c src/rkpriv.h src/rkplay.h $(MAKEFILE)
rkload.o: src/rkload.c src/rkpriv.h src/rkplay.h $(MAKEFILE)
