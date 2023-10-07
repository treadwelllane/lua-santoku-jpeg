#include "lua.h"
#include "lauxlib.h"

#include "jpeglib.h"

#include <string.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>

int luaopen_santoku_jpeg (lua_State *);

struct my_comp_error_mgr {
  struct jpeg_error_mgr err;
  jmp_buf setjmp_buffer;
};

struct my_decomp_error_mgr {
  struct jpeg_error_mgr err;
  jmp_buf setjmp_buffer;
};

struct my_source_mgr {
  struct jpeg_source_mgr src;
  void *origin;
  size_t skip;
  int row_stride;
  JSAMPARRAY buffer;
};

struct my_destination_mgr {
  struct jpeg_destination_mgr dest;
  void *origin;
};

struct my_decompress_struct {
  struct jpeg_decompress_struct decomp;
  struct my_decomp_error_mgr *err;
  struct my_source_mgr *src;
  lua_State *L;
  int denom;
  size_t bufsize;
};

struct my_compress_struct {
  struct jpeg_compress_struct comp;
  struct my_comp_error_mgr *err;
  struct my_destination_mgr *dest;
  lua_State *L;
  size_t bufsize;
};

void my_comp_error_exit (j_common_ptr comp)
{
  (*comp->err->output_message)(comp);
}

void my_comp_output_message (j_common_ptr comp)
{
  struct my_comp_error_mgr *myerr = (struct my_comp_error_mgr *) comp->err;
  struct my_compress_struct *mycomp = (struct my_compress_struct *) comp;
  char buffer[JMSG_LENGTH_MAX];
  (*comp->err->format_message)(comp, buffer);
  lua_pushboolean(mycomp->L, 0);
  lua_pushstring(mycomp->L, "compression error: ");
  lua_pushstring(mycomp->L, buffer);
  lua_concat(mycomp->L, 2);
  longjmp(myerr->setjmp_buffer, 1);
}

void my_decomp_error_exit (j_common_ptr decomp)
{
  (*decomp->err->output_message)(decomp);
}

void my_decomp_output_message (j_common_ptr decomp)
{
  struct my_decomp_error_mgr *myerr = (struct my_decomp_error_mgr *) decomp->err;
  struct my_decompress_struct *mydecomp = (struct my_decompress_struct *) decomp;
  char buffer[JMSG_LENGTH_MAX];
  (*decomp->err->format_message)(decomp, buffer);
  lua_pushboolean(mydecomp->L, 0);
  lua_pushstring(mydecomp->L, "decompression error: ");
  lua_pushstring(mydecomp->L, buffer);
  lua_concat(mydecomp->L, 2);
  longjmp(myerr->setjmp_buffer, 1);
}

void my_init_source (j_decompress_ptr decomp) {
  struct my_decompress_struct *mydecomp = (struct my_decompress_struct *) decomp;
  struct my_source_mgr *src = (struct my_source_mgr *) decomp->src;
  decomp->src->bytes_in_buffer = 0;
  src->origin = malloc(mydecomp->bufsize);
  decomp->src->next_input_byte = src->origin;
}

boolean my_fill_input_buffer (j_decompress_ptr decomp) {
  return FALSE;
}

void my_skip_input_data (j_decompress_ptr decomp, long n) {
  struct my_source_mgr *src = (struct my_source_mgr *) decomp->src;
  if (decomp->src->bytes_in_buffer < n) {
    src->skip += n - decomp->src->bytes_in_buffer;
    decomp->src->bytes_in_buffer = 0;
  } else {
    decomp->src->next_input_byte += n;
    decomp->src->bytes_in_buffer -= n;
  }
}

void my_term_source (j_decompress_ptr decomp) {
  struct my_source_mgr *src = (struct my_source_mgr *) decomp->src;
  free(src->origin);
}

void my_init_destination (j_compress_ptr comp) {
  struct my_compress_struct *mycomp = (struct my_compress_struct *) comp;
  struct my_destination_mgr *dest = (struct my_destination_mgr *) comp->dest;
  comp->dest->free_in_buffer = mycomp->bufsize;
  dest->origin = malloc(mycomp->bufsize);
  comp->dest->next_output_byte = dest->origin;
}

boolean my_empty_output_buffer (j_compress_ptr comp) {
  return FALSE;
}

void my_term_destination (j_compress_ptr comp) {
  struct my_destination_mgr *dest = (struct my_destination_mgr *) comp->dest;
  free(dest->origin);
}

void push_data (struct my_compress_struct *comp, struct my_decompress_struct *decomp, int i_push_data) {

  while (1) {

    JSAMPROW rows[1]; rows[0] = decomp->src->buffer[0];
    int rc = jpeg_write_scanlines(&comp->comp, rows, 1);

    size_t len = (void *)comp->dest->dest.next_output_byte - comp->dest->origin;

    if (len <= 0)
      return;

    lua_pushvalue(comp->L, i_push_data);
    lua_pushlstring(comp->L, (char *)comp->dest->origin, len);
    lua_pushnumber(comp->L, len);
    lua_call(comp->L, 2, 0);

    comp->dest->dest.next_output_byte = comp->dest->origin;
    comp->dest->dest.free_in_buffer = comp->bufsize;

    if (rc == 1)
      break;

  }

}

void pull_data (struct my_decompress_struct *decomp, int i_pull_data) {

  lua_pushvalue(decomp->L, i_pull_data);
  lua_call(decomp->L, 0, 2);
  luaL_checktype(decomp->L, -2, LUA_TSTRING);
  const char *data = lua_tostring(decomp->L, -2);
  luaL_checktype(decomp->L, -1, LUA_TNUMBER);
  long size = lua_tointeger(decomp->L, -1);

  if (decomp->src->skip > 0) {

    assert(decomp->src->src.bytes_in_buffer == 0);

    if (decomp->src->skip >= size) {

      decomp->src->skip -= size;
      goto next;

    } else {

      data += decomp->src->skip;
      size -= decomp->src->skip;

      decomp->src->skip = 0;

    }

  }

  // TODO: check for overflow
  // TODO: Use jpegs support for multiple
  // buffers instead of this
  memmove(decomp->src->origin, decomp->src->src.next_input_byte, decomp->src->src.bytes_in_buffer);
  decomp->src->src.next_input_byte = decomp->src->origin;
  memcpy((void *)decomp->src->src.next_input_byte + decomp->src->src.bytes_in_buffer, data, size);
  decomp->src->src.bytes_in_buffer += size;

next:
  lua_pop(decomp->L, 2);

}

int mt_scale (lua_State *L) {

  if (lua_gettop(L) != 6)
    luaL_error(L, "expected 6 arguments");

  int i_pull_data = lua_absindex(L, -6);
  int i_push_data = lua_absindex(L, -5);
  int i_scale_num = lua_absindex(L, -4);
  int i_scale_denom = lua_absindex(L, -3);
  int i_quality = lua_absindex(L, -2);
  int i_bufsize = lua_absindex(L, -1);

  // function returning (string, #string) to get
  // more data to decompress
  luaL_checktype(L, i_pull_data, LUA_TFUNCTION);

  // function accepting (string, #string) to use
  // output data
  luaL_checktype(L, i_push_data, LUA_TFUNCTION);

  // scale fraction numerator
  luaL_checktype(L, i_scale_num, LUA_TNUMBER);

  // scale fraction denominator
  luaL_checktype(L, i_scale_denom, LUA_TNUMBER);

  // quality
  luaL_checktype(L, i_quality, LUA_TNUMBER);

  // byte size of buffers (must be larger than
  // 2K for good performance)
  luaL_checktype(L, i_bufsize, LUA_TNUMBER);

  struct my_comp_error_mgr comp_err;
  struct my_decomp_error_mgr decomp_err;

  struct my_source_mgr src;
  src.skip = 0;
  src.src.init_source = &my_init_source;
  src.src.fill_input_buffer = &my_fill_input_buffer;
  src.src.skip_input_data = &my_skip_input_data;
  src.src.resync_to_restart = &jpeg_resync_to_restart;
  src.src.term_source = &my_term_source;

  struct my_destination_mgr dest;
  dest.dest.init_destination = &my_init_destination;
  dest.dest.empty_output_buffer = &my_empty_output_buffer;
  dest.dest.term_destination = &my_term_destination;

  struct my_compress_struct comp;
  comp.bufsize = lua_tointeger(L, -1);
  comp.L = L;
  comp.dest = &dest;
  comp.err = &comp_err;
  comp.comp.err = jpeg_std_error(&comp_err.err);
  jpeg_create_compress(&comp.comp);
  comp.comp.dest = &dest.dest;

  comp_err.err.error_exit = my_comp_error_exit;
  comp_err.err.output_message = my_comp_output_message;

  struct my_decompress_struct decomp;
  decomp.bufsize = lua_tointeger(L, -1);
  decomp.L = L;
  decomp.src = &src;
  decomp.err = &decomp_err;
  jpeg_create_decompress(&decomp.decomp);
  decomp.decomp.src = &src.src;
  decomp.decomp.err = jpeg_std_error(&decomp_err.err);

  decomp_err.err.error_exit = my_decomp_error_exit;
  decomp_err.err.output_message = my_decomp_output_message;

  if (setjmp(comp_err.setjmp_buffer)) {
    jpeg_destroy_decompress(&decomp.decomp);
    my_term_source(&decomp.decomp);
    jpeg_destroy_compress(&comp.comp);
    my_term_destination(&comp.comp);
    return 2;
  }

  if (setjmp(decomp_err.setjmp_buffer)) {
    jpeg_destroy_decompress(&decomp.decomp);
    my_term_source(&decomp.decomp);
    jpeg_destroy_compress(&comp.comp);
    my_term_destination(&comp.comp);
    return 2;
  }

  while (JPEG_SUSPENDED == jpeg_read_header(&decomp.decomp, 1))
    pull_data(&decomp, i_pull_data);

  while (FALSE == jpeg_start_decompress(&decomp.decomp))
    pull_data(&decomp, i_pull_data);

  comp.comp.image_width = decomp.decomp.output_width;
  comp.comp.image_height = decomp.decomp.output_height;
  comp.comp.input_components = decomp.decomp.output_components;
  comp.comp.in_color_space = decomp.decomp.out_color_space;
  jpeg_set_defaults(&comp.comp);
  jpeg_set_quality(&comp.comp, lua_tointeger(L, i_quality), 1);
  comp.comp.scale_num = lua_tointeger(L, i_scale_num);
  comp.comp.scale_denom = lua_tointeger(L, i_scale_denom);

  jpeg_start_compress(&comp.comp, TRUE);

  src.row_stride = decomp.decomp.output_width * decomp.decomp.output_components;
  src.buffer = (*comp.comp.mem->alloc_sarray)((j_common_ptr) &comp.comp, JPOOL_IMAGE, src.row_stride, 1);

  while (decomp.decomp.output_scanline < decomp.decomp.output_height) {
    if (jpeg_read_scanlines(&decomp.decomp, src.buffer, 1)) {
      push_data(&comp, &decomp, i_push_data);
    } else {
      pull_data(&decomp, i_pull_data);
    }
  }

  while (FALSE == jpeg_finish_decompress(&decomp.decomp))
    pull_data(&decomp, i_pull_data);

  jpeg_finish_compress(&comp.comp);

  jpeg_destroy_compress(&comp.comp);
  my_term_destination(&comp.comp);

  jpeg_destroy_decompress(&decomp.decomp);
  my_term_source(&decomp.decomp);

  lua_pushboolean(L, 1);
  return 1;
}

luaL_Reg mt_fns[] = {
  { "scale", mt_scale },
  { NULL, NULL }
};

int luaopen_santoku_jpeg (lua_State *L) {
  lua_newtable(L);
  luaL_setfuncs(L, mt_fns, 0);
  return 1;
}
