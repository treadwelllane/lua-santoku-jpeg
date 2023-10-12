#include "lua.h"
#include "lauxlib.h"

#include "jpeglib.h"

#include <string.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>

#define debug(...) \
  printf("%s:%d\t", __FILE__, __LINE__); \
  printf(__VA_ARGS__); \
  printf("\n"); \
  fflush(stderr);

#define MTS "santoku_jpeg_scaler"

int luaopen_santoku_jpeg (lua_State *);

struct tk_jpeg_state;

typedef struct
{
  struct jpeg_decompress_struct decomp;
  struct tk_jpeg_state *state;

} tk_jpeg_decompress_t;

typedef struct
{
  struct jpeg_compress_struct comp;
  struct tk_jpeg_state *state;

} tk_jpeg_compress_t;

typedef enum
{
  TK_JPEG_STATE_READ_HEADER,
  TK_JPEG_STATE_START_DECOMPRESS,
  TK_JPEG_STATE_PROCESS_BODY,
  TK_JPEG_STATE_FINISH,
  TK_JPEG_STATE_DONE,

} TK_JPEG_STATE;

typedef enum
{
  TK_JPEG_STATUS_WRITE,
  TK_JPEG_STATUS_READ,
  TK_JPEG_STATUS_DONE,

} TK_JPEG_STATUS;

typedef struct tk_jpeg_state
{
  TK_JPEG_STATE state;

  tk_jpeg_compress_t comp;
  struct jpeg_error_mgr comp_err;
  struct jpeg_source_mgr src;
  long src_skip;
  void *src_origin;
  JSAMPARRAY src_buffer;

  tk_jpeg_decompress_t decomp;
  struct jpeg_error_mgr decomp_err;
  struct jpeg_destination_mgr dest;
  void *dest_origin;
  JSAMPROW dest_rows[1];

  int row_stride;
  jmp_buf setjmp_buffer;

  lua_State *L;
  lua_Integer bufsize;
  lua_Integer scale_denom;
  lua_Integer scale_num;
  lua_Integer scale_quality;

} tk_jpeg_state_t;

void tk_jpeg_comp_err_exit (j_common_ptr jp)
{
  (*jp->err->output_message)(jp);
}

void tk_jpeg_decomp_err_exit (j_common_ptr jp)
{
  (*jp->err->output_message)(jp);
}

void tk_jpeg_comp_output_message (j_common_ptr jp)
{
  tk_jpeg_compress_t *comp = (tk_jpeg_compress_t *) jp;
  lua_State *L = comp->state->L;

  char buffer[JMSG_LENGTH_MAX];
  (*jp->err->format_message)(jp, buffer);

  lua_pushboolean(L, 0);
  lua_pushstring(L, "Compression error: ");
  lua_pushstring(L, buffer);
  lua_concat(L, 2);

  longjmp(comp->state->setjmp_buffer, 1);
}

void tk_jpeg_decomp_output_message (j_common_ptr jp)
{
  tk_jpeg_decompress_t *decomp = (tk_jpeg_decompress_t *) jp;
  lua_State *L = decomp->state->L;

  char buffer[JMSG_LENGTH_MAX];
  (*jp->err->format_message)(jp, buffer);

  lua_pushboolean(L, 0);
  lua_pushstring(L, "Decompression error: ");
  lua_pushstring(L, buffer);
  lua_concat(L, 2);

  longjmp(decomp->state->setjmp_buffer, 1);
}

void tk_jpeg_init_source (j_decompress_ptr jp)
{
  tk_jpeg_decompress_t *decomp = (tk_jpeg_decompress_t *) jp;

  decomp->state->src.bytes_in_buffer = 0;
  decomp->state->src_origin = malloc(decomp->state->bufsize);
  decomp->state->src.next_input_byte = decomp->state->src_origin;
}

boolean tk_jpeg_fill_input_buffer (j_decompress_ptr jp)
{
  return FALSE;
}

void tk_jpeg_skip_input_data (j_decompress_ptr jp, long n)
{
  tk_jpeg_decompress_t *decomp = (tk_jpeg_decompress_t *) jp;

  if (decomp->state->src.bytes_in_buffer < n) {
    decomp->state->src_skip += n - decomp->state->src.bytes_in_buffer;
    decomp->state->src.next_input_byte = decomp->state->src_origin;
    decomp->state->src.bytes_in_buffer = 0;
  } else {
    decomp->state->src.next_input_byte += n;
    decomp->state->src.bytes_in_buffer -= n;
  }
}

void tk_jpeg_term_source (j_decompress_ptr jp)
{
  tk_jpeg_decompress_t *decomp = (tk_jpeg_decompress_t *) jp;

  free(decomp->state->src_origin);
  decomp->state->src_origin = NULL;
}

void tk_jpeg_init_destination (j_compress_ptr jp)
{
  tk_jpeg_compress_t *comp = (tk_jpeg_compress_t *) jp;

  comp->state->dest.free_in_buffer = comp->state->bufsize;
  comp->state->dest_origin = malloc(comp->state->bufsize);
  comp->state->dest.next_output_byte = comp->state->dest_origin;
}

boolean tk_jpeg_empty_output_buffer (j_compress_ptr jp)
{
  return FALSE;
}

void tk_jpeg_term_destination (j_compress_ptr jp)
{
  tk_jpeg_compress_t *comp = (tk_jpeg_compress_t *) jp;

  free(comp->state->dest_origin);
  comp->state->dest_origin = NULL;
}

void tk_jpeg_destroy (tk_jpeg_state_t *state)
{
  tk_jpeg_term_source(&state->decomp.decomp);
  jpeg_destroy_decompress(&state->decomp.decomp);

  if (state->state >= TK_JPEG_STATE_START_DECOMPRESS) {
    tk_jpeg_term_destination(&state->comp.comp);
    jpeg_destroy_compress(&state->comp.comp);
  }
}

int tk_jpeg_emit (lua_State *L)
{
  tk_jpeg_state_t *state = lua_touserdata(L, -1);

  state->dest_rows[0] = state->src_buffer[0];
  while (jpeg_write_scanlines(&state->comp.comp, state->dest_rows, 1) != 1);

  ptrdiff_t len = (void *)state->dest.next_output_byte - state->dest_origin;

  if (len > 0) {
    lua_pushboolean(L, 1);
    lua_pushinteger(L, TK_JPEG_STATUS_READ);
    lua_pushlstring(L, (char *)state->dest_origin, len);
    state->dest.next_output_byte = state->dest_origin;
    state->dest.free_in_buffer = state->bufsize;
    return 3;
  } else {
    lua_pushboolean(L, 1);
    lua_pushinteger(L, TK_JPEG_STATUS_WRITE);
    return 2;
  }
}

int tk_jpeg_mts_read (lua_State *L)
{
  tk_jpeg_state_t *state = (tk_jpeg_state_t *) luaL_checkudata(L, -1, MTS);

  // TODO: Will this cause problems with lua's
  // coroutines?
  if (setjmp(state->setjmp_buffer)) {
    tk_jpeg_destroy(state);
    return 2;
  }

  while (1) {
    switch (state->state) {

      case TK_JPEG_STATE_READ_HEADER:
        if (jpeg_read_header(&state->decomp.decomp, 1) == JPEG_SUSPENDED) {
          lua_pushboolean(L, 1);
          lua_pushinteger(L, TK_JPEG_STATUS_WRITE);
          return 2;
        }

        state->state = TK_JPEG_STATE_START_DECOMPRESS;
        continue;

      case TK_JPEG_STATE_START_DECOMPRESS:

        if (jpeg_start_decompress(&state->decomp.decomp) == FALSE) {
          lua_pushboolean(L, 1);
          lua_pushinteger(L, TK_JPEG_STATUS_WRITE);
          return 2;
        }

        state->comp.comp.image_width = state->decomp.decomp.output_width;
        state->comp.comp.image_height = state->decomp.decomp.output_height;
        state->comp.comp.input_components = state->decomp.decomp.output_components;
        state->comp.comp.in_color_space = state->decomp.decomp.out_color_space;
        jpeg_set_defaults(&state->comp.comp);
        jpeg_set_quality(&state->comp.comp, state->scale_quality, 1);
        state->comp.comp.scale_num = state->scale_num;
        state->comp.comp.scale_denom = state->scale_denom;

        jpeg_start_compress(&state->comp.comp, TRUE);

        state->row_stride = state->decomp.decomp.output_width * state->decomp.decomp.output_components;
        state->src_buffer = (*state->comp.comp.mem->alloc_sarray)((j_common_ptr) &state->comp.comp, JPOOL_IMAGE, state->row_stride, 1);

        state->state = TK_JPEG_STATE_PROCESS_BODY;
        continue;

      case TK_JPEG_STATE_PROCESS_BODY:

        if (state->decomp.decomp.output_scanline < state->decomp.decomp.output_height) {
          if (jpeg_read_scanlines(&state->decomp.decomp, state->src_buffer, 1) == 1) {
            return tk_jpeg_emit(L);
          } else {
            lua_pushboolean(L, 1);
            lua_pushinteger(L, TK_JPEG_STATUS_WRITE);
            return 2;
          }
        }

        state->state = TK_JPEG_STATE_FINISH;
        continue;

      case TK_JPEG_STATE_FINISH:

        if (jpeg_finish_decompress(&state->decomp.decomp) == FALSE) {
          lua_pushboolean(L, 1);
          lua_pushinteger(L, TK_JPEG_STATUS_WRITE);
          return 2;
        } else {
          jpeg_finish_compress(&state->comp.comp);
          tk_jpeg_destroy(state);
          lua_pushboolean(L, 1);
          lua_pushinteger(L, TK_JPEG_STATUS_DONE);
          state->state = TK_JPEG_STATE_DONE;
          return 2;
        }

      case TK_JPEG_STATE_DONE:

        lua_pushboolean(L, 1);
        lua_pushinteger(L, TK_JPEG_STATUS_DONE);
        return 2;

    }
  }
}

int tk_jpeg_mts_write (lua_State *L)
{
  if (lua_gettop(L) != 2)
    luaL_error(L, "expected 2 arguments to write");

  tk_jpeg_state_t *state = (tk_jpeg_state_t *) luaL_checkudata(L, -2, MTS);

  if (state->state == TK_JPEG_STATE_DONE)
    luaL_error(L, "scaler is done");

  luaL_checktype(L, -1, LUA_TSTRING);

  size_t size;
  const char *data = lua_tolstring(L, -1, &size);
  lua_pop(L, 1);

  if (state->src_skip > 0) {
    assert(state->src.bytes_in_buffer == 0);
    if (state->src_skip >= size) {
      state->src_skip -= size;
      goto end;
    } else {
      data += state->src_skip;
      size -= state->src_skip;
      state->src_skip = 0;
    }
  }

  memmove(state->src_origin, state->src.next_input_byte, state->src.bytes_in_buffer);
  state->src_origin = realloc(state->src_origin, state->src.bytes_in_buffer + size);
  state->src.next_input_byte = state->src_origin;
  memcpy((void *)state->src.next_input_byte + state->src.bytes_in_buffer, data, size);
  state->src.bytes_in_buffer += size;

end:
  lua_pushboolean(L, 1);
  return 1;
}

int tk_jpeg_mt_scale (lua_State *L)
{
  if (lua_gettop(L) != 4)
    luaL_error(L, "expected 4 arguments to scale");

  int i_scale_num = lua_absindex(L, -4);
  int i_scale_denom = lua_absindex(L, -3);
  int i_scale_quality = lua_absindex(L, -2);
  int i_bufsize = lua_absindex(L, -1);

  // scale fraction numerator
  luaL_checktype(L, i_scale_num, LUA_TNUMBER);

  // scale fraction denominator
  luaL_checktype(L, i_scale_denom, LUA_TNUMBER);

  // scale quality
  luaL_checktype(L, i_scale_quality, LUA_TNUMBER);

  // scale quality
  luaL_checktype(L, i_bufsize, LUA_TNUMBER);

  tk_jpeg_state_t *state = lua_newuserdatauv(L, sizeof(tk_jpeg_state_t), 1);
  lua_pushvalue(L, lua_upvalueindex(1));
  lua_setiuservalue(L, -2, 1);
  luaL_setmetatable(L, MTS);

  state->state = TK_JPEG_STATE_READ_HEADER;
  state->L = L;
  state->bufsize = lua_tointeger(L, i_bufsize);
  state->scale_quality = lua_tointeger(L, i_scale_quality);
  state->scale_num = lua_tointeger(L, i_scale_num);
  state->scale_denom = lua_tointeger(L, i_scale_denom);
  state->src_skip = 0;

  state->src.init_source = &tk_jpeg_init_source;
  state->src.fill_input_buffer = &tk_jpeg_fill_input_buffer;
  state->src.skip_input_data = &tk_jpeg_skip_input_data;
  state->src.resync_to_restart = &jpeg_resync_to_restart;
  state->src.term_source = &tk_jpeg_term_source;

  state->dest.init_destination = &tk_jpeg_init_destination;
  state->dest.empty_output_buffer = &tk_jpeg_empty_output_buffer;
  state->dest.term_destination = &tk_jpeg_term_destination;

  state->comp.state = state;
  state->comp.comp.err = jpeg_std_error(&state->comp_err);
  jpeg_create_compress(&state->comp.comp);
  state->comp.comp.dest = &state->dest;

  state->comp_err.error_exit = &tk_jpeg_comp_err_exit;
  state->comp_err.output_message = &tk_jpeg_comp_output_message;

  state->decomp.state = state;
  jpeg_create_decompress(&state->decomp.decomp);
  state->decomp.decomp.err = jpeg_std_error(&state->decomp_err);
  state->decomp.decomp.src = &state->src;

  state->decomp_err.error_exit = &tk_jpeg_decomp_err_exit;
  state->decomp_err.output_message = &tk_jpeg_decomp_output_message;

  lua_insert(L, -5);
  lua_pop(L, 4);

  tk_jpeg_mts_read(L);
  int t = lua_gettop(L);

  if (lua_toboolean(L, -t + 1)) {
    lua_remove(L, -t + 2);
    lua_insert(L, -t + 1);
  } else {
    lua_remove(L, -t - 1);
  }

  return t - 1;
}

luaL_Reg tk_jpeg_mts_fns[] =
{
  { "write", tk_jpeg_mts_write },
  { "read", tk_jpeg_mts_read },
  { NULL, NULL }
};

luaL_Reg tk_jpeg_mt_fns[] =
{
  { "scale", tk_jpeg_mt_scale },
  { NULL, NULL }
};

int luaopen_santoku_jpeg (lua_State *L)
{
  lua_newtable(L); // mt
  lua_pushvalue(L, -1); // mt mt
  luaL_setfuncs(L, tk_jpeg_mt_fns, 1); // mt

  lua_pushinteger(L, TK_JPEG_STATUS_WRITE); // mt tag
  lua_setfield(L, -2, "WRITE"); // mt

  lua_pushinteger(L, TK_JPEG_STATUS_READ); // mt tag
  lua_setfield(L, -2, "READ"); // mt

  lua_pushinteger(L, TK_JPEG_STATUS_DONE); // mt tag
  lua_setfield(L, -2, "DONE"); // mt

  luaL_newmetatable(L, MTS); // mt mts
  lua_newtable(L); // mt mts idx
  lua_pushvalue(L, -3); // mt mts idx mt
  luaL_setfuncs(L, tk_jpeg_mts_fns, 1); // mt mts idx
  lua_setfield(L, -2, "__index"); // mt mts
  lua_pop(L, 1); // mt

  return 1;
}
