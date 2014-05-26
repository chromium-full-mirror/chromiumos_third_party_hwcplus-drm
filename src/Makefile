include config

HWC_HDR = $(shell $(PKG_CONFIG) --cflags hwcplus)
# uncomment to use local source instead instead of installed headers
#HWC_HDR = -I$(HOME)/chrome_root/src/third_party/hwcplus/include

LIB_HWCPLUS   = $(shell $(PKG_CONFIG) --libs hwcplus)
LIB_DRM_INTEL = $(shell $(PKG_CONFIG) --libs libdrm_intel)

INC = \
$(HWC_HDR) \
$(shell $(PKG_CONFIG) --cflags libdrm) \
-Idrm_gralloc \
-I. \


GRALLOC_OBJ = \
drm_gralloc/gralloc.o \


LIBGRALLOC_DRM_OBJ = \
drm_gralloc/gralloc_drm.o \
drm_gralloc/gralloc_drm_kms.o \
drm_gralloc/gralloc_drm_intel.o \


OBJ = \
$(GRALLOC_OBJ) \
$(LIBGRALLOC_DRM_OBJ) \


BINDEST=$(DESTDIR)/usr/bin
INCDEST=$(DESTDIR)/usr/include/hwcplus-drm
LIBDEST=$(DESTDIR)/usr/$(LIBDIR)
HALDEST=$(LIBDEST)/hwcplus
PKGDEST=$(LIBDEST)/pkgconfig

CC = $(CHOST)-gcc
CXX = $(CHOST)-g++

CFLAGS += $(INC) -fPIC
CFLAGS += -DCHROMEOS -DANDROID_SMP=0 -DENABLE_INTEL -DHAVE_SYS_UIO_H
CFLAGS += -O0 -g
CXXFLAGS := $(CFLAGS)
CFLAGS += -std=gnu99

all: libgralloc_drm.so gralloc.default.so hwcplus-drm.pc tests/rings-gralloc

gralloc.default.so: $(GRALLOC_OBJ) libgralloc_drm.so
	$(CC) -shared -o $@ $^

libgralloc_drm.so: $(LIBGRALLOC_DRM_OBJ)
	$(CC) -shared -o $@ $^ $(LIB_DRM_INTEL) -lm

tests/rings-gralloc: tests/rings-gralloc.c
	$(CC) $(CFLAGS) -o $@ $^ -L. $(LIB_HWCPLUS) -lm -ldl

hwcplus-drm.pc: hwcplus-drm.pc.in
	sed -e "s/@LIBDIR@/$(LIBDIR)/" < $^ > $@

clean:
	rm -f $(OBJ) hwcplus-drm.pc libgralloc_drm.so gralloc.default.so tests/rings-gralloc

install:
	install -d $(INCDEST)
	install drm_gralloc/gralloc_drm.h $(INCDEST)
	install drm_gralloc/gralloc_drm_handle.h $(INCDEST)

	install -d $(PKGDEST)
	install hwcplus-drm.pc $(PKGDEST)

	install -d $(LIBDEST)
	install libgralloc_drm.so $(LIBDEST)

	install -d $(HALDEST)
	install gralloc.default.so $(HALDEST)

	install -d $(BINDEST)
	install tests/rings-gralloc $(BINDEST)
