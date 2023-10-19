NAME ?= santoku-jpeg
VERSION ?= 0.0.9-1
GIT_URL ?= git@github.com:treadwelllane/lua-santoku-jpeg.git
HOMEPAGE ?= https://github.com/treadwelllane/lua-santoku-jpeg
LICENSE ?= MIT

BUILD_DIR ?= $(PWD)/build
WORK_DIR ?= $(BUILD_DIR)/work
TEST_DIR ?= $(BUILD_DIR)/test
CONFIG_DIR ?= $(PWD)/config
SRC_DIR ?= $(PWD)/src
TEST_SRC_DIR ?= $(PWD)/test

SRC_LUA ?= $(shell find $(SRC_DIR) -name '*.lua')
SRC_C ?= $(shell find $(SRC_DIR) -name '*.c')

BUILD_LUA ?= $(patsubst $(SRC_DIR)/%.lua, $(WORK_DIR)/%.lua, $(SRC_LUA))
BUILD_C ?= $(patsubst $(SRC_DIR)/%.c, $(WORK_DIR)/%.so, $(SRC_C))

INST_LUA ?= $(patsubst $(SRC_DIR)/%.lua, $(INST_LUADIR)/%.lua, $(SRC_LUA))
INST_C ?= $(patsubst $(SRC_DIR)/%.c, $(INST_LIBDIR)/%.so, $(SRC_C))

ROCKSPEC ?= $(WORK_DIR)/$(NAME)-$(VERSION).rockspec
ROCKSPEC_T ?= $(CONFIG_DIR)/template.rockspec

LUAROCKS ?= luarocks

LIBFLAG ?= -shared

LIB_CFLAGS ?= $(if $(LUA_INCDIR), -I$(LUA_INCDIR)) -Wall
LIB_LDFLAGS ?= $(if $(LUA_LIBDIR), -L$(LUA_LIBDIR)) $(LIBFLAG) -ljpeg -Wall

TOKU_BUNDLE ?= toku bundle -M -i debug -l luacov -l luacov.hook
TOKU_TEST ?= toku test -s

DEPS ?= $(ROCKSPEC) $(BUILD_C) $(BUILD_LUA)

# Default to local libjpeg
LOCAL_JPEG ?= 1

CONFIGURE ?= ./configure
MAKE_LUA ?= make

ifneq ($(filter-out test luarocks-test-run, $(MAKECMDGOALS)), $(MAKECMDGOALS))
LOCAL_LUAROCKS = 1
LOCAL_JPEG = 1
endif

ifeq ($(EMSCRIPTEN),1)

ifneq ($(SANITIZE),0)
SANITIZER_FLAGS ?= -fsanitize=address -fsanitize=undefined -fsanitize-address-use-after-return=always -fsanitize-address-use-after-scope
SANITIZER_VARS ?= ASAN_SYMBOLIZER_PATH="$(shell which llvm-symbolizer)"
TEST_CFLAGS := $(SANITIZER_FLAGS) $(TEST_CFLAGS)
TEST_LDFLAGS := $(SANITIZER_FLAGS) $(TEST_LDFLAGS)
LIB_CFLAGS := $(SANITIZER_FLAGS) $(TEST_CFLAGS)
LIB_LDFLAGS := $(SANITIZER_FLAGS) $(TEST_LDFLAGS)
endif

BUILD_DIR := $(BUILD_DIR)/emscripten

TOKU_BUNDLE += -C
TOKU_TEST += -i 'node --expose-gc --trace-gc'

TEST_CFLAGS += -gsource-map --bind
TEST_LDFLAGS += -gsource-map

LIB_CFLAGS += -gsource-map
LIB_LDFLAGS += -gsource-map

CC = emcc
LD = emcc
AR = emar
AR_LUA = emar rcu
NM = emnm
RANLIB = emranlib

LDFLAGS += -sALLOW_MEMORY_GROWTH -lnodefs.js -lnoderawfs.js

# Annoying that this is necessary for separate
# AR definition for compiling lua
EM_VARS_LUA = CC="$(CC)" LD="$(CC)" AR="$(AR_LUA)" NM="$(NM)" RANLIB="$(RANLIB)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"
EM_VARS = CC="$(CC)" LD="$(CC)" AR="$(AR)" NM="$(NM)" RANLIB="$(RANLIB)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"
MAKE = emmake make $(EM_VARS)
MAKE_LUA = emmake make $(EM_VARS_LUA)

CONFIGURE = emconfigure ./configure

LIB_CFLAGS += --bind

LOCAL_LUA = 1
LOCAL_JPEG = 1

endif

ifeq ($(LOCAL_LUA),1)

LUA_VERSION ?= 5.4.4
LUA_MINMAJ ?= $(shell echo $(LUA_VERSION) | grep -o ".\..")
LUA_ARCHIVE ?= lua-$(LUA_VERSION).tar.gz
LUA_DL ?= $(WORK_DIR)/$(LUA_ARCHIVE)
LUA_DIR ?= $(WORK_DIR)/lua-$(LUA_VERSION)
LUA_URL ?= https://www.lua.org/ftp/$(LUA_ARCHIVE)
LUA_DIST_DIR ?= $(LUA_DIR)/install
LUA_INC_DIR ?= $(LUA_DIST_DIR)/include
LUA_LIB_DIR ?= $(LUA_DIST_DIR)/lib
LUA_LIB ?= $(LUA_DIST_DIR)/lib/liblua.a
LUA_INTERP ?= $(LUA_DIST_DIR)/bin/lua

LUA_INCDIR = $(LUA_INC_DIR)
LUA_LIBDIR = $(LUA_LIB_DIR)

CFLAGS += -I$(LUA_INC_DIR)
LDFLAGS += -L$(LUA_LIB_DIR)
LIB_CFLAGS += -I$(LUA_INC_DIR)
LIB_LDFLAGS += -L$(LUA_LIB_DIR)
TEST_CFLAGS += -I$(LUA_INC_DIR)
TEST_LDFLAGS += -L$(LUA_LIB_DIR)

DEPS += $(LUA_DIST_DIR)

$(LUA_DIST_DIR): $(LUA_DL)
	# rm -rf "$(LUA_DIR)"
	mkdir -p "$(dir $(LUA_DIR))"
	tar xf "$(LUA_DL)" -C "$(dir $(LUA_DIR))"
	cd "$(LUA_DIR)" && $(MAKE_LUA)
	cd "$(LUA_DIR)" && $(MAKE_LUA) local
	[ "$(EMSCRIPTEN)" == "1" ] && \
		cp "$(LUA_DIR)/src/"*.wasm "$(LUA_DIST_DIR)/bin/" || true

$(LUA_DL):
	mkdir -p "$(dir $(LUA_DL))"
	curl -LsSo "$(LUA_DL)" "$(LUA_URL)"

else

# TODO: Should this check use luarocks instead?
LUA_MINMAJ ?= $(shell lua -v | grep -Po 'Lua\s*\K\d+\.\d+')

endif

ifeq ($(LOCAL_JPEG),1)

JPEG_VERSION ?= 9e
JPEG_URL ?= https://ijg.org/files/jpegsrc.v$(JPEG_VERSION).tar.gz
JPEG_ARCHIVE ?= jpegsrc.v$(JPEG_VERSION).tar.gz
JPEG_DL ?= $(WORK_DIR)/$(JPEG_ARCHIVE)
JPEG_DIR ?= $(WORK_DIR)/jpeg-$(JPEG_VERSION)
JPEG_LIB ?= $(JPEG_DIR)/.libs/libjpeg.a

LIB_CFLAGS += -I$(JPEG_DIR)
LIB_LDFLAGS += -L$(JPEG_DIR)/.libs
TEST_CFLAGS += -I$(JPEG_DIR) $(JPEG_LIB)

ifneq ($(EMSCRIPTEN),1)
LIB_LDFLAGS += -Wl,-rpath,$(JPEG_DIR)/.libs
CFLAGS += -fPIC
endif

TEST_LDFLAGS += -L$(JPEG_DIR)/.libs

DEPS += $(JPEG_LIB)

$(JPEG_LIB): $(JPEG_DL)
	# rm -rf "$(JPEG_DIR)"
	mkdir -p "$(dir $(JPEG_DIR))"
	tar xf "$(JPEG_DL)" -C "$(dir $(JPEG_DIR))"
	cd "$(JPEG_DIR)" && $(CONFIGURE)
	cd "$(JPEG_DIR)" && $(MAKE)

$(JPEG_DL):
	mkdir -p "$(dir $(JPEG_DL))"
	curl -LsSo "$(JPEG_DL)" "$(JPEG_URL)"

endif

TEST_SPEC_DIST_DIR ?= $(TEST_DIR)/spec
TEST_SPEC_SRC_DIR ?= $(TEST_SRC_DIR)/spec

TEST_SPEC_SRCS ?= $(shell find $(TEST_SPEC_SRC_DIR) -type f -name '*.lua')
TEST_SPEC_DISTS ?= $(patsubst $(TEST_SPEC_SRC_DIR)/%.lua, $(TEST_SPEC_DIST_DIR)/%.test, $(TEST_SPEC_SRCS))

TEST_CFLAGS ?= -Wall
TEST_LDFLAGS ?= -Wall

ifdef TEST

TESTED_FILES := $(patsubst $(TEST_SPEC_SRC_DIR)/%.lua, $(TEST_SPEC_DIST_DIR)/%.test, $(PWD)/$(TEST))

# $(error "Test set: $(TESTED_FILES)")

else

TESTED_FILES = $(TEST_SPEC_DISTS)

# $(error "Test unset: $(TESTED_FILES)")

endif

ifneq ($(EMSCRIPTEN),1)

TEST_SPEC_DISTS := $(filter-out %/web.test, $(TEST_SPEC_DISTS))
TESTED_FILES := $(filter-out %/web.test, $(TEST_SPEC_DISTS))

endif

TEST_LUA_PATH ?= $(WORK_DIR)/?.lua;$(LUAROCKS_TREE)/share/lua/$(LUA_MINMAJ)/?.lua;$(LUAROCKS_TREE)/share/lua/$(LUA_MINMAJ)/?/init.lua
TEST_LUA_CPATH ?= $(WORK_DIR)/?.so;$(LUAROCKS_TREE)/lib/lua/$(LUA_MINMAJ)/?.so

ifeq ($(LOCAL_LUAROCKS),1)

LUAROCKS_CFG ?= $(WORK_DIR)/luarocks.config.local.lua
LUAROCKS_CFG_T ?= $(CONFIG_DIR)/luarocks.config.local.lua
LUAROCKS_TREE ?= $(WORK_DIR)/luarocks

LUA_PATH = $(LUAROCKS_TREE)/share/lua/$(LUA_MINMAJ)/?.lua;$(LUAROCKS_TREE)/share/lua/$(LUA_MINMAJ)/?/init.lua
LUA_CPATH = $(LUAROCKS_TREE)/lib/lua/$(LUA_MINMAJ)/?.so
LUAROCKS = LUAROCKS_CONFIG="$(LUAROCKS_CFG)" luarocks --tree "$(LUAROCKS_TREE)"

DEPS += $(LUAROCKS_CFG)

$(LUAROCKS_CFG): $(LUAROCKS_CFG_T)
	mkdir -p "$(dir $@)"
	ROCKS_TREE="$(LUAROCKS_TREE)" \
  LUA_INCDIR="$(LUA_INCDIR)" \
  LUA_LIBDIR="$(LUA_LIBDIR)" \
  CC="$(CC)" \
  LD="$(LD)" \
  AR="$(AR)" \
  NM="$(NM)" \
  RANLIB="$(RANLIB)" \
  CFLAGS="$(CFLAGS)" \
  LDFLAGS="$(LDFLAGS)" \
  LIBFLAG="$(LIBFLAG)" \
		toku template \
			-f "$(LUAROCKS_CFG_T)" \
			-o "$(LUAROCKS_CFG)"

endif

LUACHECK_CFG ?= $(TEST_SRC_DIR)/luacheck.lua
LUACHECK_SRCS ?= src

LUACOV_CFG ?= $(WORK_DIR)/luacov.lua
LUACOV_CFG_T ?= $(TEST_SRC_DIR)/luacov.lua
LUACOV_STATS_FILE ?= $(WORK_DIR)/luacov.stats.out
LUACOV_REPORT_FILE ?= $(WORK_DIR)/luacov.report.out
LUACOV_INCLUDE ?= $(SRC_DIR)

build: $(DEPS) $(BUILD_C)

install: $(DEPS)
	$(LUAROCKS) make $(ROCKSPEC)

upload: $(ROCKSPEC)
	@if test -z "$(LUAROCKS_API_KEY)"; then echo "Missing LUAROCKS_API_KEY variable"; exit 1; fi
	@if ! git diff --quiet; then echo "Commit your changes first"; exit 1; fi
	git tag "$(VERSION)"
	git push --tags
	git push
	$(LUAROCKS) upload --skip-pack --api-key "$(LUAROCKS_API_KEY)" "$(ROCKSPEC)"

clean:
	rm -rf build

test: $(DEPS) $(TEST_SPEC_DISTS)
	$(LUAROCKS) test $(ROCKSPEC)

luarocks-build: $(BUILD_C)

luarocks-install: $(INST_LUA) $(INST_C)

# TODO: 'install' should be luarocks-install, but how to we set INST_LIB/BINDIR?
luarocks-test: $(DEPS) $(LUACOV_CFG) $(LUACHECK_CFG)
	if SANITIZE="$(SANITIZE)" $(TOKU_TEST) $(TESTED_FILES); then \
		luacov -c "$(LUACOV_CFG)"; \
		cat "$(LUACOV_REPORT_FILE)" | \
			awk '/^Summary/ { P = NR } P && NR > P + 1'; \
		echo; \
		luacheck --config "$(LUACHECK_CFG)" $(LUACHECK_SRCS) || true; \
		echo; \
	fi

luarocks-test-run: $(ROCKSPEC)
	$(LUAROCKS) $(ARGS)

iterate: $(ROCKSPEC)
	@while true; do \
		$(MAKE) $(MAKEFLAGS) test; \
		inotifywait -qqr -e close_write -e create -e delete -e delete \
			Makefile $(SRC_DIR) $(CONFIG_DIR) $(TEST_SPEC_SRC_DIR); \
	done

$(INST_LUADIR)/%.lua: $(SRC_DIR)/%.lua
	@if test -z "$(INST_LUADIR)"; then echo "Missing INST_LUADIR variable"; exit 1; fi
	mkdir -p "$(dir $@)"
	cp "$<" "$@"

$(INST_LIBDIR)/%.so: $(WORK_DIR)/%.so
	@if test -z "$(INST_LIBDIR)"; then echo "Missing INST_LIBDIR variable"; exit 1; fi
	mkdir -p "$(dir $@)"
	cp "$<" "$@"

$(WORK_DIR)/%.so: $(SRC_DIR)/%.c $(JPEG_LIB)
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(LIB_CFLAGS) $(LDFLAGS) $(LIB_LDFLAGS) $(LIBFLAG) "$<" -o "$@"

$(ROCKSPEC): $(ROCKSPEC_T)
	mkdir -p "$(dir $@)"
	NAME="$(NAME)" \
	VERSION="$(VERSION)" \
	GIT_URL="$(GIT_URL)" \
	HOMEPAGE="$(HOMEPAGE)" \
	LICENSE="$(LICENSE)" \
	EMSCRIPTEN="$(EMSCRIPTEN)" \
		toku template \
			-f "$(ROCKSPEC_T)" \
			-o "$(ROCKSPEC)"

$(TEST_SPEC_DIST_DIR)/%.test: $(TEST_SPEC_SRC_DIR)/%.lua
	mkdir -p "$(dir $@)"
	CC="$(CC)" \
	LD="$(LD)" \
	AR="$(AR)" \
	NM="$(NM)" \
	RANLIB="$(RANLIB)" \
	LIBFLAG="$(LIBFLAG)" \
		$(TOKU_BUNDLE) \
			-E SANITIZE "$(SANITIZE)" \
			-E LUACOV_CONFIG "$(LUACOV_CFG)" \
			-e LUA_PATH "$(TEST_LUA_PATH)" \
			-e LUA_CPATH "$(TEST_LUA_CPATH)" \
			--cflags " $(TEST_CFLAGS) $(CFLAGS)" \
			--ldflags " $(TEST_LDFLAGS) $(LDFLAGS)" \
			-f "$<" -o "$(dir $@)" -O "$(notdir $@)" \

$(LUACOV_CFG): $(LUACOV_CFG_T)
	mkdir -p "$(dir $@)"
	STATS_FILE="$(LUACOV_STATS_FILE)" \
	REPORT_FILE="$(LUACOV_REPORT_FILE)" \
	INCLUDE="$(LUACOV_INCLUDE)" \
		toku template \
			-f "$(LUACOV_CFG_T)" \
			-o "$(LUACOV_CFG)"

include $(shell find $(WORK_DIR) -type f -name '*.d')

.PHONY: build install upload clean test iterate luarocks-build luarocks-install luarocks-test luarocks-test-run
