<% push(wasm) %>

<% return var("VERSION") %> ?= 9e
JPEG_URL = https://ijg.org/files/jpegsrc.v$(<% return var("VERSION") %>).tar.gz
JPEG_ARCHIVE = jpegsrc.v$(<% return var("VERSION") %>).tar.gz
JPEG_DIR = $(CURDIR)/jpeg-$(<% return var("VERSION") %>)
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

<% pop() push(not wasm) %>

<% return var("CFLAGS") %> ?=
<% return var("LDFLAGS") %> ?= -ljpeg

results.mk: Makefile
	truncate -s0 results.mk
	echo "LIB_CFLAGS += $(<% return var("CFLAGS") %>)" >> results.mk
	echo "LIB_LDFLAGS += $(<% return var("LDFLAGS") %>)" >> results.mk

<% pop() %>
