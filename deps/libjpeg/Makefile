ifneq ($(VPFX)_LOCAL_JPEG,1)

all: env.mk

env.mk:
	truncate -s0 env.mk
	echo "JPEG_CFLAGS = $(shell pkg-config --cflags libjpeg)" >> env.mk
	echo "JPEG_LDFLAGS = $(shell pkg-config --libs libjpeg)" >> env.mk

else

LIB = libjpeg.a

all: jpeg-9e/.libs/$(LIB) env.mk

jpeg-9e/.libs/$(LIB):
	[ ! -f jpegsrc.v9e.tar.gz  ] && \
		wget https://ijg.org/files/jpegsrc.v9e.tar.gz
	rm -rf jpeg-9e || true
	tar xf jpegsrc.v9e.tar.gz
	cd jpeg-9e && ./configure
	cd jpeg-9e && make

env.mk:
	truncate -s0 env.mk
	echo "JPEG_CFLAGS = -I$(CURDIR)/jpeg-9e/ $(CURDIR)/jpeg-9e/.libs/$(LIB)" >> env.mk
	echo "JPEG_LDFLAGS = -L$(CURDIR)/jpeg-9e/.libs " >> env.mk

endif

.PHONY: all
