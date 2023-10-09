#include "lua.h"
#include "lauxlib.h"

#include "jpeglib.h"

#include <string.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>

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
  TK_JPEG_STATE_VALUE_PULL = 1,
  TK_JPEG_STATE_VALUE_PUSH = 2,
  TK_JPEG_STATE_VALUE_TOTAL = 2

} TK_JPEG_STATE_VALUE;

typedef enum
{
  TK_JPEG_STATE_READ_HEADER,
  TK_JPEG_STATE_START_DECOMPRESS,
  TK_JPEG_STATE_START_COMPRESS,
  TK_JPEG_STATE_PROCESS_BODY,
  TK_JPEG_STATE_FINISH,

} TK_JPEG_STATE;

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
  lua_pushstring(L, "compression error: ");
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
  lua_pushstring(L, "decompression error: ");
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
  tk_jpeg_term_destination(&state->comp.comp);
  jpeg_destroy_decompress(&state->decomp.decomp);
  jpeg_destroy_compress(&state->comp.comp);
}

int tk_jpeg_loop (lua_State *);

int tk_jpeg_push_data_cont (lua_State *L, int status, lua_KContext ctx)
{
  tk_jpeg_state_t *state = lua_touserdata(L, -1);

  state->dest.next_output_byte = state->dest_origin;
  state->dest.free_in_buffer = state->bufsize;

  if (status == LUA_YIELD)
    return tk_jpeg_loop(L);
  else
    return 0;
}

void tk_jpeg_push_data (lua_State *L)
{
  tk_jpeg_state_t *state = lua_touserdata(L, -1);

  state->dest_rows[0] = state->src_buffer[0];

  while (jpeg_write_scanlines(&state->comp.comp, state->dest_rows, 1) != 1);

  ptrdiff_t len = (void *)state->dest.next_output_byte - state->dest_origin;

  if (len > 0) {
    lua_getiuservalue(L, -1, TK_JPEG_STATE_VALUE_PUSH);
    lua_pushlstring(L, (char *)state->dest_origin, len);
    lua_callk(L, 1, 0, 0, tk_jpeg_push_data_cont);
  }

  tk_jpeg_push_data_cont(L, LUA_OK, 0);
}

int tk_jpeg_pull_data_cont (lua_State *L, int status, lua_KContext ctx)
{
  tk_jpeg_state_t *state = lua_touserdata(L, -2);
  luaL_checktype(L, -1, LUA_TSTRING);

  size_t size;
  const char *data = lua_tolstring(L, -1, &size);
  lua_pop(L, 1);

  if (state->src_skip > 0) {
    assert(state->src.bytes_in_buffer == 0);
    if (state->src_skip >= size) {
      state->src_skip -= size;
      return 0;
    } else {
      data += state->src_skip;
      size -= state->src_skip;
      state->src_skip = 0;
    }
  }

  // TODO: check for overflow
  // TODO: Use jpegs support for multiple
  // buffers instead of this
  memmove(state->src_origin, state->src.next_input_byte, state->src.bytes_in_buffer);
  state->src.next_input_byte = state->src_origin;
  memcpy((void *)state->src.next_input_byte + state->src.bytes_in_buffer, data, size);
  state->src.bytes_in_buffer += size;

  if (status == LUA_YIELD)
    return tk_jpeg_loop(L);
  else
    return 0;
}

void tk_jpeg_pull_data (lua_State *L)
{
  lua_getiuservalue(L, -1, TK_JPEG_STATE_VALUE_PULL);
  lua_callk(L, 0, 1, 0, tk_jpeg_pull_data_cont);
  tk_jpeg_pull_data_cont(L, LUA_OK, 0);
}

int tk_jpeg_loop (lua_State *L)
{
  tk_jpeg_state_t *state = lua_touserdata(L, -1);

  // TODO: Will this cause problems with lua's
  // coroutines?
  if (setjmp(state->setjmp_buffer)) {
    tk_jpeg_destroy(state);
    return 2;
  }

  while (1)
  {
    switch (state->state) {

      case TK_JPEG_STATE_READ_HEADER:

        if (jpeg_read_header(&state->decomp.decomp, 1) == JPEG_SUSPENDED) {
          tk_jpeg_pull_data(L);
          continue;
        } else {
          state->state = TK_JPEG_STATE_START_DECOMPRESS;
          continue;
        }

      case TK_JPEG_STATE_START_DECOMPRESS:

        if (jpeg_start_decompress(&state->decomp.decomp) == FALSE) {
          tk_jpeg_pull_data(L);
          continue;
        } else {
          state->state = TK_JPEG_STATE_START_COMPRESS;
          continue;
        }

      case TK_JPEG_STATE_START_COMPRESS:

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

        if (state->decomp.decomp.output_scanline >= state->decomp.decomp.output_height) {
          state->state = TK_JPEG_STATE_FINISH;
          continue;
        } else if (jpeg_read_scanlines(&state->decomp.decomp, state->src_buffer, 1)) {
          tk_jpeg_push_data(L);
          continue;
        } else {
          tk_jpeg_pull_data(L);
          continue;
        }

      case TK_JPEG_STATE_FINISH:

        if (jpeg_finish_decompress(&state->decomp.decomp) == FALSE) {
          tk_jpeg_pull_data(L);
          continue;
        } else {
          jpeg_finish_compress(&state->comp.comp);
          tk_jpeg_destroy(state);
          lua_pop(L, 1);
          lua_pushboolean(L, 1);
          return 1;
        }

    }
  }
}

int tk_jpeg_mt_scale (lua_State *L)
{
  if (lua_gettop(L) != 6)
    luaL_error(L, "expected 6 arguments");

  int i_pull_data = lua_absindex(L, -6);
  int i_push_data = lua_absindex(L, -5);
  int i_scale_num = lua_absindex(L, -4);
  int i_scale_denom = lua_absindex(L, -3);
  int i_scale_quality = lua_absindex(L, -2);
  int i_bufsize = lua_absindex(L, -1);

  // function returning (string, #string) to get
  // more data to decompress
  if (lua_type(L, i_pull_data) != LUA_TTABLE)
      luaL_checktype(L, i_pull_data, LUA_TFUNCTION);

  // function accepting (string, #string) to use
  // output data
  if (lua_type(L, i_push_data) != LUA_TTABLE)
      luaL_checktype(L, i_push_data, LUA_TFUNCTION);

  // scale fraction numerator
  luaL_checktype(L, i_scale_num, LUA_TNUMBER);

  // scale fraction denominator
  luaL_checktype(L, i_scale_denom, LUA_TNUMBER);

  // scale quality
  luaL_checktype(L, i_scale_quality, LUA_TNUMBER);

  // byte size of buffers (must be larger than
  // 2K for good performance)
  luaL_checktype(L, i_bufsize, LUA_TNUMBER);

  tk_jpeg_state_t *state = lua_newuserdatauv(L, sizeof(tk_jpeg_state_t), TK_JPEG_STATE_VALUE_TOTAL);
  int i_state = lua_absindex(L, -1);

  lua_pushvalue(L, i_pull_data);
  lua_setiuservalue(L, i_state, TK_JPEG_STATE_VALUE_PULL);

  lua_pushvalue(L, i_push_data);
  lua_setiuservalue(L, i_state, TK_JPEG_STATE_VALUE_PUSH);

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

  lua_insert(L, -7);
  lua_pop(L, 6);

  return tk_jpeg_loop(L);
}

luaL_Reg tk_jpeg_mt_fns[] =
{
  { "scale", tk_jpeg_mt_scale },
  { NULL, NULL }
};

int luaopen_santoku_jpeg (lua_State *L)
{
  lua_newtable(L);
  luaL_setfuncs(L, tk_jpeg_mt_fns, 0);
  return 1;
}
