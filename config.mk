ifeq ($(TK_JPEG_WASM),1)
export LIB_LDFLAGS += --bind
endif
