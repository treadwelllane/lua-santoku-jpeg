ifeq ($($(VPFX)_WASM),1)
$(VPFX)_LOCAL_JPEG = 1
endif

ifeq ($($(VPFX)_LOCAL_JPEG),1)

$(VPFX)_JPEG_VERSION ?= 9e
JPEG_URL = https://ijg.org/files/jpegsrc.v$($(VPFX)_JPEG_VERSION).tar.gz
JPEG_ARCHIVE = jpegsrc.v$($(VPFX)_JPEG_VERSION).tar.gz
JPEG_DIR = $(CURDIR)/jpeg-$($(VPFX)_JPEG_VERSION)
JPEG_LIB = $(JPEG_DIR)/.libs/libjpeg.a

results.mk: Makefile $(JPEG_LIB)
	truncate -s0 results.mk
	echo "LIB_CFLAGS += -I$(JPEG_DIR)" >> results.mk
	echo "LIB_LDFLAGS += -L$(JPEG_DIR)/.libs -ljpeg" >> results.mk

$(JPEG_LIB): $(JPEG_ARCHIVE)
	tar xf "$(JPEG_ARCHIVE)"
	cd "$(JPEG_DIR)" && ./configure
	cd "$(JPEG_DIR)" && $(MAKE)

$(JPEG_ARCHIVE):
	curl -LsSo "$(JPEG_ARCHIVE)" "$(JPEG_URL)"

else

$(VPFX)_JPEG_CFLAGS ?=
$(VPFX)_JPEG_LDFLAGS ?= -ljpeg

results.mk: Makefile
	truncate -s0 results.mk
	echo "LIB_CFLAGS += $($(VPFX)_JPEG_CFLAGS)" >> results.mk
	echo "LIB_LDFLAGS += $($(VPFX)_JPEG_LDFLAGS)" >> results.mk

endif
