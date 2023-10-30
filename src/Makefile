SRC_LUA = $(shell find * -name '*.lua')
SRC_C = $(shell find * -name '*.c')
SRC_O = $(SRC_C:.c=.o)
SRC_SO = $(SRC_O:.o=.$(LIB_EXTENSION))

INST_LUA = $(addprefix $(INST_LUADIR)/, $(SRC_LUA))
INST_SO = $(addprefix $(INST_LIBDIR)/, $(SRC_SO))

LIB_CFLAGS = -Wall $(JPEG_CFLAGS) -I$(LUA_INCDIR)
LIB_LDFLAGS = -Wall $(JPEG_LDFLAGS) -L$(LUA_LIBDIR)

ifeq ($($(VPFX)_SANITIZE),1)
LIB_CFLAGS += -fsanitize=address -fsanitize=leak
LIB_LDFLAGS += -fsanitize=address -fsanitize=leak
endif

all: $(SRC_O) $(SRC_SO)

%.o: %.c
	$(CC) $(LIB_CFLAGS) $(CFLAGS) -c -o $@ $<

%.$(LIB_EXTENSION): %.o
	$(CC) $(LIB_LDFLAGS) $(LDFLAGS) $(LIBFLAG) -o $@ $<

install: $(INST_LUA) $(INST_SO)

$(INST_LUADIR)/%.lua: ./%.lua
	mkdir -p $(dir $@)
	cp $< $@

$(INST_LIBDIR)/%.$(LIB_EXTENSION): ./%.$(LIB_EXTENSION)
	mkdir -p $(dir $@)
	cp $< $@

.PHONY: all install
