# Makefile — MaeShare system Share plugin for Maemo 5 (Fremantle / N900)
#
# The plugin needs no libsharing headers, no glib and no Qt: a C compiler plus the
# target sysroot's libc and libdl are enough.
#
#   make                 build libmaeshare.so
#   make deploy          push the .so + service XML to a device (set DEVICE/SSH_KEY)
#   make clean
#
# Cross-compiling from a desktop toolchain:
#   make CROSS=/path/to/arm-linux-gnueabi- \
#        SYSROOT=/path/to/fremantle/sysroot \
#        HYBRID=/path/to/hybrid-bin \
#        DL=/path/to/sysroot/lib/libdl-2.5.so
#
# Building on the device itself (or inside Scratchbox) needs no overrides:
#   make CROSS= SYSROOT= HYBRID= DL=-ldl
#
# Note: a sysroot whose /usr/lib/libdl.so is an absolute symlink will not link via
# -ldl; point DL at the real libdl-2.5.so instead.

CROSS   ?= arm-linux-gnueabi-
SYSROOT ?=
HYBRID  ?=
DL      ?= $(SYSROOT)/lib/libdl-2.5.so

CC      := $(CROSS)gcc
CFLAGS  := -O2 -Wall -fPIC -fvisibility=default
LDFLAGS := $(if $(SYSROOT),--sysroot=$(SYSROOT),) \
           $(if $(HYBRID),-B$(HYBRID),) \
           $(if $(SYSROOT),-B$(SYSROOT)/usr/lib -L$(SYSROOT)/usr/lib -L$(SYSROOT)/lib,) \
           -march=armv7-a -mfpu=neon -mfloat-abi=softfp \
           -shared -Wl,-soname,libmaeshare.so

TARGET  := libmaeshare.so
SRC     := libmaeshare-plugin.c

# For "make deploy": point these at your own device.
DEVICE  ?= root@YOUR-N900-IP
SSH_KEY ?=
SSH_OPTS ?= -o ConnectTimeout=12 $(if $(SSH_KEY),-i $(SSH_KEY),)

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(DL)

deploy: $(TARGET)
	scp -O $(SSH_OPTS) $(TARGET) $(DEVICE):/usr/lib/sharing/plugins/
	scp -O $(SSH_OPTS) maeshare.service.xml $(DEVICE):/usr/share/sharing/services/
	ssh -n $(SSH_OPTS) $(DEVICE) 'chmod 644 /usr/lib/sharing/plugins/libmaeshare.so /usr/share/sharing/services/maeshare.service.xml; \
	  ls -l /usr/lib/sharing/plugins/libmaeshare.so /usr/share/sharing/services/maeshare.service.xml'

clean:
	rm -f $(TARGET)

.PHONY: all deploy clean
