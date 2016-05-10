// PackpnmCoder.cpp

#include "CPP/7zip/Compress/StdAfx.h"

#include "C/Alloc.h"

#include "CPP/7zip/Common/CWrappers.h"
#include "CPP/7zip/Common/StreamUtils.h"

#include "PackpnmCoder.h"
#include "ppnmtbl.h"
#include "PackjpgCoder.h"

namespace NCompress {
namespace NPackjpg {
namespace NPackpnm {

//using namespace NCompress::NPackjpg;

CCoder::CCoder()
{
  memset(cmask, 0, sizeof(cmask));

  use_rle = 0;    // use RLE compression for HDR output
}

/* -----------------------------------------------
  set each variable to its initial value
  ----------------------------------------------- */

bool CCoder::reset_buffers(void)
{
  imgwidth = 0;   // width of image
  imgheight = 0;  // height of image
  imgwidthv = 0;  // visible width of image
  imgbpp = 0;     // bit per pixel
  cmpc = 0;       // component count
  pnmax = 0;      // maximum pixel value
  endian_l = 0;   // endian
  bmpsize = 0;    // file size according to header
  for (int i = 0; i < 5; i++) {
    if (cmask[i] != NULL) delete(cmask[i]);
    cmask[i] = NULL;
  }

  filetype = 0;   // type of current file
  subtype = 0;    // sub type of file

  return true;
}


/* ----------------------- Begin of side functions -------------------------- */


/* -----------------------------------------------
  PPN PJG type RGBA/E encoding
  ----------------------------------------------- */
bool CCoder::ppn_encode_imgdata_rgba(aricoder *enc, ISequentialInStream *stream, UInt64 *inSizeProcessed)
{
  pjg_model *mod[4];
  int *storage; // storage array
  int *val[4][2] = { { NULL } };
  int *err[4][2] = { { NULL } };
  int *dta[4] = { NULL };
  int c, x, y;

  // init models
  for (c = 0; c < cmpc; c++)
    mod[c] = new pjg_model(
    (pnmax == 0) ? cmask[c]->m : pnmax,
    ((c < 3) && (cmpc >= 3)) ? 3 : 2);

  // allocate storage memory
  storage = (int*)calloc((imgwidth + 2) * 4 * cmpc, sizeof(int));
  if (storage == NULL) {
    sprintf(errormessage, MEM_ERRMSG);
    errorlevel = 2;
    return false;
  }

  // arithmetic compression loop
  for (y = 0; y < imgheight; y++) {
    // set pointers
    for (c = 0; c < cmpc; c++) {
      val[c][0] = storage + 2 + ((imgwidth + 2) * (0 + (4 * c) + ((y + 0) % 2)));
      val[c][1] = storage + 2 + ((imgwidth + 2) * (0 + (4 * c) + ((y + 1) % 2)));
      err[c][0] = storage + 2 + ((imgwidth + 2) * (2 + (4 * c) + ((y + 0) % 2)));
      err[c][1] = storage + 2 + ((imgwidth + 2) * (2 + (4 * c) + ((y + 1) % 2)));
      dta[c] = val[c][0];
    }
    // read line
    errorlevel = pnm_read_line(stream, dta, inSizeProcessed);
    if (errorlevel == 1) sprintf(errormessage, (subtype == S_HDR) ?
      "bitwise reconstruction of HDR RLE not guaranteed" : "excess data or bad data found");
    else if (errorlevel == 2) {
      sprintf(errormessage, "unexpected end of file");
      free(storage);
      for (c = 0; c < cmpc; c++) delete(mod[c]);
      return false;
    }
    // encode pixel values
    for (x = 0; x < imgwidth; x++) {
      if (cmpc == 1) {
        ppn_encode_pjg(enc, mod[0], val[0], err[0], -1);
      }
      else { // cmpc >= 3
        if (cmpc > 3) ppn_encode_pjg(enc, mod[3], val[3], err[3], -1);
        ppn_encode_pjg(enc, mod[0], val[0], err[0], BITLENB16N(err[2][0][-1]));
        ppn_encode_pjg(enc, mod[2], val[2], err[2], BITLENB16N(err[0][0][0]));
        ppn_encode_pjg(enc, mod[1], val[1], err[1],
          (BITLENB16N(err[0][0][0]) + BITLENB16N(err[2][0][0]) + 1) / 2);
      }
      // advance values
      for (c = 0; c < cmpc; c++) {
        val[c][0]++; val[c][1]++; err[c][0]++; err[c][1]++;
      }
    }
  }

  // free storage / clear models
  free(storage);
  for (c = 0; c < cmpc; c++) delete(mod[c]);

  return true;
}


/* -----------------------------------------------
  PPN PJG type RGBA/E decoding
  ----------------------------------------------- */
bool CCoder::ppn_decode_imgdata_rgba(aricoder *dec, ISequentialOutStream *stream, UInt64 *outSizeProcessed)
{
  pjg_model *mod[4];
  int *storage; // storage array
  int *val[4][2] = { { NULL } };
  int *err[4][2] = { { NULL } };
  int *dta[4] = { NULL };
  int c, x, y;

  // init models
  for (c = 0; c < cmpc; c++)
    mod[c] = new pjg_model(
    (pnmax == 0) ? cmask[c]->m : pnmax,
    ((c < 3) && (cmpc >= 3)) ? 3 : 2);

  // allocate storage memory
  storage = (int*)calloc((imgwidth + 2) * 4 * cmpc, sizeof(int));
  if (storage == NULL) {
    sprintf(errormessage, MEM_ERRMSG);
    errorlevel = 2;
    return false;
  }

  // arithmetic compression loop
  for (y = 0; y < imgheight; y++) {
    // set pointers
    for (c = 0; c < cmpc; c++) {
      val[c][0] = storage + 2 + ((imgwidth + 2) * (0 + (4 * c) + ((y + 0) % 2)));
      val[c][1] = storage + 2 + ((imgwidth + 2) * (0 + (4 * c) + ((y + 1) % 2)));
      err[c][0] = storage + 2 + ((imgwidth + 2) * (2 + (4 * c) + ((y + 0) % 2)));
      err[c][1] = storage + 2 + ((imgwidth + 2) * (2 + (4 * c) + ((y + 1) % 2)));
      dta[c] = val[c][0];
    }
    // decode pixel values
    for (x = 0; x < imgwidth; x++) {
      if (cmpc == 1) {
        ppn_decode_pjg(dec, mod[0], val[0], err[0], -1);
      }
      else { // cmpc >= 3
        if (cmpc > 3) ppn_decode_pjg(dec, mod[3], val[3], err[3], -1);
        ppn_decode_pjg(dec, mod[0], val[0], err[0], BITLENB16N(err[2][0][-1]));
        ppn_decode_pjg(dec, mod[2], val[2], err[2], BITLENB16N(err[0][0][0]));
        ppn_decode_pjg(dec, mod[1], val[1], err[1],
          (BITLENB16N(err[0][0][0]) + BITLENB16N(err[2][0][0]) + 1) / 2);
      }
      // advance values
      for (c = 0; c < cmpc; c++) {
        val[c][0]++; val[c][1]++; err[c][0]++; err[c][1]++;
      }
    }
    // write line
    pnm_write_line(stream, dta, outSizeProcessed);
  }

  // free storage / clear models
  free(storage);
  for (c = 0; c < cmpc; c++) delete(mod[c]);
  
  return true;
}


/* -----------------------------------------------
  PPN special mono encoding
  ----------------------------------------------- */
bool CCoder::ppn_encode_imgdata_mono(aricoder *enc, ISequentialInStream *stream, UInt64 *inSizeProcessed)
{
  model_b* mod;
  int *storage; // storage array
  int *val[3];
  int *dta;
  int x, y;

  // init model
  mod = INIT_MODEL_B((1 << 12), 1);

  // allocate storage memory
  storage = (int*)calloc((imgwidth + 5) * 3, sizeof(int));
  if (storage == NULL) {
    sprintf(errormessage, MEM_ERRMSG);
    errorlevel = 2;
    return false;
  }

  // arithmetic compression loop
  for (y = 0; y < imgheight; y++) {
    // set pointers
    val[0] = storage + 3 + ((imgwidth + 2 + 3) * ((y + 2) % 3));
    val[1] = storage + 3 + ((imgwidth + 2 + 3) * ((y + 1) % 3));
    val[2] = storage + 3 + ((imgwidth + 2 + 3) * ((y + 0) % 3));
    dta = val[0];
    // read line
    if (pnm_read_line(stream, &dta, inSizeProcessed) != 0) {
      sprintf(errormessage, "unexpected end of file");
      errorlevel = 2;
      free(storage);
      delete(mod);
      return false;
    }
    // encode pixel values
    for (x = 0; x < imgwidth; x++, val[0]++, val[1]++, val[2]++) {
      mod->shift_context(get_context_mono(x, y, val));
      encode_ari(enc, mod, **val);
    }
  }

  // free storage / clear models
  free(storage);
  delete(mod);

  return true;
}


/* -----------------------------------------------
  PPN special mono decoding
  ----------------------------------------------- */
bool CCoder::ppn_decode_imgdata_mono(aricoder *dec, ISequentialOutStream *stream, UInt64 *outSizeProcessed)
{
  model_b* mod;
  int* storage; // storage array
  int* val[3];
  int* dta;
  int x, y;


  // init model
  mod = INIT_MODEL_B((1 << 12), 1);

  // allocate storage memory
  storage = (int*)calloc((imgwidth + 5) * 3, sizeof(int));
  if (storage == NULL) {
    sprintf(errormessage, MEM_ERRMSG);
    errorlevel = 2;
    return false;
  }

  // arithmetic compression loop
  for (y = 0; y < imgheight; y++) {
    // set pointers
    val[0] = storage + 3 + ((imgwidth + 2 + 3) * ((y + 2) % 3));
    val[1] = storage + 3 + ((imgwidth + 2 + 3) * ((y + 1) % 3));
    val[2] = storage + 3 + ((imgwidth + 2 + 3) * ((y + 0) % 3));
    dta = val[0];
    // decode pixel values
    for (x = 0; x < imgwidth; x++, val[0]++, val[1]++, val[2]++) {
      mod->shift_context(get_context_mono(x, y, val));
      **val = decode_ari(dec, mod);
    }
    // write line
    pnm_write_line(stream, &dta, outSizeProcessed);
  }

  // free storage / clear models
  free(storage);
  delete(mod);
  
  return true;
}


/* -----------------------------------------------
  PPN encoding for palette based image data
  ----------------------------------------------- */
bool CCoder::ppn_encode_imgdata_palette(aricoder *enc, ISequentialInStream *stream, UInt64 *inSizeProcessed)
{
  model_s* mod;
  int *storage; // storage array
  int *val[2];
  int *dta;
  int x, y;

  // init model
  mod = (pnmax) ? INIT_MODEL_S(pnmax + 1, pnmax + 1, 2) :
    INIT_MODEL_S(cmask[0]->m + 1, cmask[0]->m + 1, 2);

  // allocate storage memory
  storage = (int*)calloc((imgwidth + 1) * 2, sizeof(int));
  if (storage == NULL) {
    sprintf(errormessage, MEM_ERRMSG);
    errorlevel = 2;
    return false;
  }

  // arithmetic compression loop
  for (y = 0; y < imgheight; y++) {
    // set pointers
    val[0] = storage + 1 + ((imgwidth + 1) * ((y + 0) % 2));
    val[1] = storage + 1 + ((imgwidth + 1) * ((y + 1) % 2));
    dta = val[0];
    // read line
    errorlevel = pnm_read_line(stream, &dta, inSizeProcessed);
    if (errorlevel == 1) sprintf(errormessage, "excess data or bad data found");
    else if (errorlevel == 2) {
      sprintf(errormessage, "unexpected end of file");
      free(storage);
      delete(mod);
      return false;
    }
    // encode pixel values
    for (x = 0; x < imgwidth; x++, val[0]++, val[1]++) {
      shift_model(mod, val[0][-1], val[1][0]); // shift in context
      encode_ari(enc, mod, **val);
    }
  }

  // free storage / clear models
  free(storage);
  delete(mod);

  return true;
}


/* -----------------------------------------------
  PPN decoding for palette based image data
  ----------------------------------------------- */
bool CCoder::ppn_decode_imgdata_palette(aricoder *dec, ISequentialOutStream *stream, UInt64 *outSizeProcessed)
{
  model_s* mod;
  int* storage; // storage array
  int* val[2];
  int* dta;
  int x, y;

  // init model
  mod = (pnmax) ? INIT_MODEL_S(pnmax + 1, pnmax + 1, 2) :
    INIT_MODEL_S(cmask[0]->m + 1, cmask[0]->m + 1, 2);

  // allocate storage memory
  storage = (int*)calloc((imgwidth + 1) * 2, sizeof(int));
  if (storage == NULL) {
    sprintf(errormessage, MEM_ERRMSG);
    errorlevel = 2;
    return false;
  }

  // arithmetic compression loop
  for (y = 0; y < imgheight; y++) {
    // set pointers
    val[0] = storage + 1 + ((imgwidth + 1) * ((y + 0) % 2));
    val[1] = storage + 1 + ((imgwidth + 1) * ((y + 1) % 2));
    dta = val[0];
    // decode pixel values
    for (x = 0; x < imgwidth; x++, val[0]++, val[1]++) {
      shift_model(mod, val[0][-1], val[1][0]); // shift in context
      **val = decode_ari(dec, mod);
    }
    // write line
    pnm_write_line(stream, &dta, outSizeProcessed);
  }

  // free storage / clear models
  free(storage);
  delete(mod);

  return true;
}


/* -----------------------------------------------
  PPN packJPG type encoding
  ----------------------------------------------- */
void CCoder::ppn_encode_pjg(aricoder* enc, pjg_model *mod, int **val, int **err, int ctx3) {
  int ctx_sgn; // context for sign
  int clen, absv, sgn;
  int bt, bp;

  // calculate prediction error
  **err = **val - plocoi(val[0][-1], val[1][0], val[1][-1]);

  // encode bit length
  clen = BITLENB16N(**err);
  if (ctx3 < 0) shift_model(mod->len, BITLENB16N(err[0][-1]), BITLENB16N(err[1][0]));
  else shift_model(mod->len, BITLENB16N(err[0][-1]), BITLENB16N(err[1][0]), ctx3);
  encode_ari(enc, mod->len, clen);

  // encode residual, sign only if bit length > 0
  if (clen > 0)	{
    // compute absolute value, sign
    absv = ABS(**err);
    sgn = (**err > 0) ? 0 : 1;
    // compute sign context
    ctx_sgn = 0x0;
    if (err[0][-2] > 0) ctx_sgn |= 0x1 << 0;
    if (err[1][-1] > 0) ctx_sgn |= 0x1 << 1;
    if (err[0][-1] > 0) ctx_sgn |= 0x1 << 2;
    if (err[1][0] > 0) ctx_sgn |= 0x1 << 3;
    // first set bit must be 1, so we start at clen - 2
    for (bp = clen - 2; bp >= 0; bp--) {
      shift_model(mod->res, clen, bp); // shift in context
      // encode/get bit
      bt = BITN(absv, bp);
      encode_ari(enc, mod->res, bt);
    }
    // encode sign
    mod->sgn->shift_context(ctx_sgn);
    encode_ari(enc, mod->sgn, sgn);
  }
}


/* -----------------------------------------------
  PPN packJPG type decoding
  ----------------------------------------------- */
void CCoder::ppn_decode_pjg(aricoder* dec, pjg_model* mod, int** val, int** err, int ctx3)
{
  int ctx_sgn; // context for sign
  int clen, absv, sgn;
  int bt, bp;


  // decode bit length (of prediction error)
  if (ctx3 < 0) shift_model(mod->len, BITLENB16N(err[0][-1]), BITLENB16N(err[1][0]));
  else shift_model(mod->len, BITLENB16N(err[0][-1]), BITLENB16N(err[1][0]), ctx3);
  clen = decode_ari(dec, mod->len);

  // decode residual, sign only if bit length > 0
  if (clen > 0)	{
    // compute sign context
    ctx_sgn = 0x0;
    if (err[0][-2] > 0) ctx_sgn |= 0x1 << 0;
    if (err[1][-1] > 0) ctx_sgn |= 0x1 << 1;
    if (err[0][-1] > 0) ctx_sgn |= 0x1 << 2;
    if (err[1][0] > 0) ctx_sgn |= 0x1 << 3;
    // first set bit must be 1, so we start at clen - 2
    for (bp = clen - 2, absv = 1; bp >= 0; bp--) {
      shift_model(mod->res, clen, bp); // shift in context
      // decode bit
      bt = decode_ari(dec, mod->res);
      // update value
      absv = absv << 1;
      if (bt) absv |= 1;
    }
    // decode sign
    mod->sgn->shift_context(ctx_sgn);
    sgn = decode_ari(dec, mod->sgn);
    // store data
    **err = (sgn == 0) ? absv : -absv;
  }
  else **err = 0;

  // decode prediction error
  **val = **err + plocoi(val[0][-1], val[1][0], val[1][-1]);
}


/* -----------------------------------------------
  special context for mono color space
  ----------------------------------------------- */
int CCoder::get_context_mono(int x, int y, int **val)
{
  int ctx_mono = 0;

  // this is the context modeling used here:
  //			[i]	[f]	[j]
  //		[g]	[c]	[b]	[d]	[h]
  //	[k]	[e]	[a]	[x]
  // [x] is to be encoded
  // this function calculates and returns coordinates for a simple 2D context

  // base context calculation
  ctx_mono <<= 1; if (val[0][-1]) ctx_mono |= 1; // a
  ctx_mono <<= 1; if (val[1][0])  ctx_mono |= 1; // b
  ctx_mono <<= 1; if (val[1][-1]) ctx_mono |= 1; // c
  ctx_mono <<= 1; if (val[1][1])  ctx_mono |= 1; // d
  ctx_mono <<= 1; if (val[0][-2]) ctx_mono |= 1; // e
  ctx_mono <<= 1; if (val[2][0])  ctx_mono |= 1; // f
  ctx_mono <<= 1; if (val[1][-2]) ctx_mono |= 1; // g
  ctx_mono <<= 1; if (val[1][2])  ctx_mono |= 1; // h
  ctx_mono <<= 1; if (val[2][-1]) ctx_mono |= 1; // i
  ctx_mono <<= 1; if (val[2][1])  ctx_mono |= 1; // j
  ctx_mono <<= 1; if (val[0][-3]) ctx_mono |= 1; // k

  // special flags (edge treatment)
  if (x <= 2) {
    if (x == 0)
      ctx_mono |= 0x804; // bits 11 (z) &  3 (c)
    else if (x == 1)
      ctx_mono |= 0x840; // bits 11 (z) &  6 (g)
    else ctx_mono |= 0xC00; // bits 11 (z) & 10 (k)
  }
  else if (y <= 1) {
    if (y == 0)
      ctx_mono |= 0x810; // bits 11 (z) &  4 (d)
    else ctx_mono |= 0xA00; // bits 11 (z) &  9 (j)
  }

  return ctx_mono;
}


/* -----------------------------------------------
  loco-i predictor
  ----------------------------------------------- */
int CCoder::plocoi(int a, int b, int c)
{
  // a -> left; b -> above; c -> above-left
  int min, max;

  min = (a < b) ? a : b;
  max = (a > b) ? a : b;

  if (c >= max) return min;
  if (c <= min) return max;

  return a + b - c;
}


/* -----------------------------------------------
  PNM read line
  ----------------------------------------------- */
int CCoder::pnm_read_line(ISequentialInStream *stream, int **line, UInt64 *inSizeProcessed)
{
  unsigned int rgb[4] = { 0, 0, 0, 0 }; // RGB + A
  unsigned char bt = 0;
  unsigned int dt = 0;
  int w_excess = 0;
  int x, n, c;

  if (cmpc == 1) switch (imgbpp) { // single component data
  case  1:
    for (x = 0; x < imgwidth; x += 8) {
      if (!prepare_ex_data(stream, 1)) return 2;
      bt = ex_data[ex_next_offset++];
      ++*inSizeProcessed;
      for (n = 8 - 1; n >= 0; n--, bt >>= 1) line[0][x + n] = bt & 0x1;
    }
    break;

  case  4:
    for (x = 0; x < imgwidth; x += 2) {
      if (!prepare_ex_data(stream, 1)) return 2;
      bt = ex_data[ex_next_offset++];
      ++*inSizeProcessed;
      line[0][x + 1] = (bt >> 0) & 0xF;
      line[0][x + 0] = (bt >> 4) & 0xF;
      if (pnmax > 0) {
        if (line[0][x + 0] > (signed)pnmax) { line[0][x + 0] = pnmax; w_excess = 1; }
        if (line[0][x + 1] > (signed)pnmax) { line[0][x + 1] = pnmax; w_excess = 1; }
      }
    }
    break;

  case  8:
    for (x = 0; x < imgwidth; x++) {
      if (!prepare_ex_data(stream, 1)) return 2;
      bt = ex_data[ex_next_offset++];
      ++*inSizeProcessed;
      if (pnmax > 0) if (bt > pnmax) { bt = pnmax; w_excess = 1; }
      line[0][x] = bt;
    }
    break;

  case 16:
    for (x = 0; x < imgwidth; x++) {
      for (n = 0, dt = 0; n < 16; n += 8) {
        if (!prepare_ex_data(stream, 1)) return 2;
        bt = ex_data[ex_next_offset++];
        ++*inSizeProcessed;
        dt = (endian_l) ? dt | (bt << n) : (dt << 8) | bt;
      }
      if (pnmax > 0) if (dt > pnmax) { dt = pnmax; w_excess = 1; }
      line[0][x] = dt;
    }
    break;
  }
  else { // multi component data
    for (x = 0; x < imgwidth; x++) {
      if (subtype == S_BMP) { // 16/24/32bpp BMP
        for (n = 0, dt = 0; n < imgbpp; n += 8) {
          if (!prepare_ex_data(stream, 1)) return 2;
          bt = ex_data[ex_next_offset++];
          ++*inSizeProcessed;
          dt = (endian_l) ? dt | (bt << n) : (dt << 8) | bt;
        }
        for (c = 0; c < cmpc; c++) {
          rgb[c] = (dt >> cmask[c]->p) & cmask[c]->m;
          if (pnmax > 0) if (rgb[c] > pnmax) { rgb[c] = pnmax; w_excess = 1; }
        }
        if (cmask[4] != NULL) if ((dt >> cmask[4]->p) & cmask[4]->m) w_excess = 1;
      }
      else if (subtype == S_PPM) { // 24/48bpp PPM
        for (c = 0; c < 3; c++) {
          if (imgbpp == 48) for (n = 0, dt = 0; n < 16; n += 8) {
            if (!prepare_ex_data(stream, 1)) return 2;
            bt = ex_data[ex_next_offset++];
            ++*inSizeProcessed;
            dt = (endian_l) ? dt | (bt << n) : (dt << 8) | bt;
          }
          else { // imgbpp == 24
            if (!prepare_ex_data(stream, 1)) return 2;
            bt = ex_data[ex_next_offset++];
            ++*inSizeProcessed;
            dt = bt;
          }
          if (pnmax > 0) if (dt > pnmax) { dt = pnmax; w_excess = 1; }
          rgb[c] = dt;
        }
      }
      else { // 32bpp HDR
        for (c = 0; c < 4; c++) { // try uncompressed reading
          if (!prepare_ex_data(stream, 1)) return 2;
          bt = ex_data[ex_next_offset++];
          ++*inSizeProcessed;
          rgb[c] = bt;
        }
        if (x == 0) { // check for RLE compression
          for (c = 0, dt = 0; c < 4; dt = (dt << 8) | rgb[c++]);
          if (dt == (unsigned)(0x02020000 | imgwidth))
            return hdr_decode_line_rle(stream, line, inSizeProcessed);
        }
      }
      // RGB color component prediction & copy to line
      rgb_process(rgb);
      for (c = 0; c < cmpc; c++) line[c][x] = rgb[c];
    }
  }

  // bmp line alignment at 4 byte
  if (subtype == S_BMP)	{
    for (x = imgwidth * imgbpp; (x % 32) != 0; x += 8) {
      if (!prepare_ex_data(stream, 1)) return 2;
      bt = ex_data[ex_next_offset++];
      ++*inSizeProcessed;
      if (bt) w_excess = 1;
    }
  }

  return w_excess;
}


/* -----------------------------------------------
  PNM write line
  ----------------------------------------------- */
HRESULT CCoder::pnm_write_line(ISequentialOutStream *stream, int **line, UInt64 *outSizeProcessed)
{
  unsigned int rgb[4]; // RGB + A
  unsigned char bt = 0;
  unsigned int dt = 0;
  int x, n, c;

  if (cmpc == 1) switch (imgbpp) { // single component data
  case  1:
    for (x = 0; x < imgwidth; x += 8) {
      for (n = 0, bt = 0; n < 8; n++) {
        bt <<= 1; if (line[0][x + n]) bt |= 0x1;
      }
      RINOK(WriteStream(stream, &bt, 1));
      ++*outSizeProcessed;
    }
    break;

  case  4:
    for (x = 0; x < imgwidth; x += 2) {
      bt = (line[0][x + 0] << 4) | line[0][x + 1];
      RINOK(WriteStream(stream, &bt, 1));
      ++*outSizeProcessed;
    }
    break;

  case  8:
    for (x = 0; x < imgwidth; x++) {
      bt = line[0][x];
      RINOK(WriteStream(stream, &bt, 1));
      ++*outSizeProcessed;
    }
    break;

  case 16:
    for (x = 0; x < imgwidth; x++) {
      for (n = 0, dt = line[0][x]; n < 16; n += 8) {
        bt = ((endian_l) ? (dt >> n) : (dt >> ((16 - 8) - n))) & 0xFF;
        RINOK(WriteStream(stream, &bt, 1));
        ++*outSizeProcessed;
      }
    }
    break;
  }
  else { // multi component data
    if (use_rle && (subtype == S_HDR)) { // HDR RLE encoding
      bt = 0x02; RINOK(WriteStream(stream, &bt, 1)); RINOK(WriteStream(stream, &bt, 1));
      *outSizeProcessed += 2;
      bt = (imgwidth >> 8) & 0xFF;  RINOK(WriteStream(stream, &bt, 1)); ++*outSizeProcessed;
      bt = imgwidth & 0xFF;         RINOK(WriteStream(stream, &bt, 1)); ++*outSizeProcessed;
      return hdr_encode_line_rle(stream, line, outSizeProcessed);
    }
    for (x = 0; x < imgwidth; x++) {
      // copy & RGB color component prediction undo
      for (c = 0; c < cmpc; c++) rgb[c] = line[c][x];
      // RGB color component prediction & copy to line
      rgb_unprocess(rgb);
      if (subtype == S_BMP) { // 16/24/32bpp BMP
        for (c = 0, dt = 0; c < cmpc; c++) dt |= rgb[c] << cmask[c]->p;
        for (n = 0; n < imgbpp; n += 8) {
          bt = ((endian_l) ? (dt >> n) : (dt >> ((imgbpp - 8) - n))) & 0xFF;
          RINOK(WriteStream(stream, &bt, 1)); ++*outSizeProcessed;
        }
      }
      else if (subtype == S_PPM) { // 24/48bpp PPM
        for (c = 0; c < 3; c++) {
          if (imgbpp == 48) for (n = 0, dt = rgb[c]; n < 16; n += 8) {
            bt = ((endian_l) ? (dt >> n) : (dt >> ((16 - 8) - n))) & 0xFF;
            RINOK(WriteStream(stream, &bt, 1)); ++*outSizeProcessed;
          }
          else { // imgbpp == 24
            bt = rgb[c];
            RINOK(WriteStream(stream, &bt, 1)); ++*outSizeProcessed;
          }
        }
      }
      else { // 32bpp HDR
        for (c = 0; c < 4; c++) {
          bt = rgb[c];
          RINOK(WriteStream(stream, &bt, 1)); ++*outSizeProcessed;
        }
      }
    }
  }

  // bmp line alignment at 4 byte
  if (subtype == S_BMP)	{
    for (x = imgwidth * imgbpp, bt = 0; (x % 32) != 0; x += 8) {
      RINOK(WriteStream(stream, &bt, 1)); ++*outSizeProcessed;
    }
  }

  return 0;
}


/* -----------------------------------------------
  HDR decode RLE
  ----------------------------------------------- */
int CCoder::hdr_decode_line_rle(ISequentialInStream *stream, int **line, UInt64 *inSizeProcessed)
{
  static unsigned int *data = NULL;
  static int prev_width = 0;
  unsigned int *rgb; // RGB + E
  unsigned char bt = 0;
  int r, rl;
  int x, c;

  // allocate memory for line storage
  if (prev_width != imgwidth) {
    prev_width = imgwidth;
    data = (unsigned int*)calloc(imgwidth * 4, sizeof(int));
    if (data == NULL) return 2; // bad, but unlikely to happen anyways
  }

  // RLE compressed reading
  for (c = 0; c < 4; c++) {
    for (x = 0, rgb = data + c; x < imgwidth; x += rl) {
      if (!prepare_ex_data(stream, 1)) return 2;
      bt = ex_data[ex_next_offset++];
      ++*inSizeProcessed;
      if (bt > 0x80) { // run of value
        rl = bt ^ 0x80; // run length: absolute of bt
        if (x + rl > imgwidth) return 2; // sanity check
        if (!prepare_ex_data(stream, 1)) return 2;
        bt = ex_data[ex_next_offset++];
        ++*inSizeProcessed;
        for (r = 0; r < rl; r++, rgb += 4) *rgb = bt;
      }
      else { // dump
        rl = bt; // dump length: same as bt
        if (x + rl > imgwidth) return 2; // sanity check
        for (r = 0; r < rl; r++, rgb += 4) {
          if (!prepare_ex_data(stream, 1)) return 2;
          bt = ex_data[ex_next_offset++];
          ++*inSizeProcessed;
          *rgb = bt;
        }
      }
    }
  }

  // prediction and copy
  for (x = 0, rgb = data; x < imgwidth; x++, rgb += 4) {
    rgb_process(rgb); // prediction
    for (c = 0; c < 4; c++) line[c][x] = rgb[c];
  }

  return 1;
}


/* -----------------------------------------------
  HDR encode RLE
  ----------------------------------------------- */
int CCoder::hdr_encode_line_rle(ISequentialOutStream *stream, int **line, UInt64 *outSizeProcessed)
{
  static unsigned int* data = NULL;
  static int prev_width = 0;
  unsigned int* rgb; // RGB + E
  unsigned int* dt;
  unsigned char bt = 0;
  int rl, nd;
  int x, rm, c;

  // allocate memory for line storage
  if (prev_width != imgwidth) {
    prev_width = imgwidth;
    data = (unsigned int*)calloc(imgwidth * 4, sizeof(int));
    if (data == NULL) return 2; // bad, but unlikely to happen anyways
  }

  // undo prediction and copy
  for (x = 0, rgb = data; x < imgwidth; x++, rgb += 4) {
    for (c = 0; c < 4; c++) rgb[c] = line[c][x];
    rgb_unprocess(rgb); // prediction undo
  }

  // RLE compressed writing
  for (c = 0; c < 4; c++) {
    for (rm = imgwidth, dt = rgb = data + c; rm;) {
      if (rm > 1) {
        rm -= 2; dt += 8;
        if (*(dt - 8) == *(dt - 4)) { // start of a run
          for (rl = 2;
            (rm) && (rl < 0x7F) && (*dt == *(dt - 4));
            rl++, rm--, dt += 4);
          bt = rl | 0x80; RINOK(WriteStream(stream, &bt, 1)); ++*outSizeProcessed;
          bt = *rgb;      RINOK(WriteStream(stream, &bt, 1)); ++*outSizeProcessed;
          rgb = dt;
        }
        else { // start of a dump
          for (nd = 2, rl = 1;
            (rm) && (nd < 0x80);
            nd++, rm--, dt += 4) {
            if (*dt == *(dt - 4)) {
              if (++rl > 2) {
                nd -= (rl - 1);
                rm += (rl - 1);
                dt -= ((rl - 1) * 4);
                break;
              }
            }
            else rl = 1;
          }
          bt = nd; RINOK(WriteStream(stream, &bt, 1)); ++*outSizeProcessed;
          for (; rgb < dt; rgb += 4) {
            bt = *rgb; RINOK(WriteStream(stream, rgb, 1)); ++*outSizeProcessed;
          }
        }
      }
      else { // only one remains
        bt = 0x01;  RINOK(WriteStream(stream, &bt, 1)); ++*outSizeProcessed;
        bt = *dt;   RINOK(WriteStream(stream, &bt, 1)); ++*outSizeProcessed;
        break;
      }
    }
  }

  return 0;
}


/* -----------------------------------------------
  apply RGB prediction
  ----------------------------------------------- */
void CCoder::rgb_process(unsigned int* rgb) {
  // RGB color component prediction
  for (int c = 0; c < 3; c++) if (c != 1) {
    if (pnmax == 0) {
      rgb[c] |= 0x40000000;
      rgb[c] -= (rgb[1] >> (cmask[1]->s - cmask[c]->s));
      rgb[c] &= cmask[c]->m;
    }
    else {
      rgb[c] -= rgb[1];
      if (rgb[c] < 0) rgb[c] += pnmax + 1;
    }
  }
}


/* -----------------------------------------------
  undo RGB prediction
  ----------------------------------------------- */
void CCoder::rgb_unprocess(unsigned int* rgb)
{
  // RGB color component prediction undo
  for (int c = 0; c < 3; c++) if (c != 1) {
    if (pnmax == 0) {
      rgb[c] += (rgb[1] >> (cmask[1]->s - cmask[c]->s));
      rgb[c] &= cmask[c]->m;
    }
    else {
      rgb[c] += rgb[1];
      if (rgb[c] > pnmax) rgb[c] -= pnmax + 1;
    }
  }
}


/* -----------------------------------------------
  identify file from 2 bytes
  ----------------------------------------------- */
void CCoder::identify(const char* id, int* ft, int* st)
{
  *ft = F_UNK; *st = S_UNK;
  switch (id[0]) {
  case 'S':
    switch (id[1]) {
    case '4': *st = S_PBM; break;
    case '5': *st = S_PGM; break;
    case '6': *st = S_PPM; break;
    case 'M': *st = S_BMP; break;
    case '?': *st = S_HDR; break;
    }
    if (*st != S_UNK) *ft = F_PPN;
    break;

  case 'P':
    switch (id[1]) {
    case '4': *st = S_PBM; break;
    case '5': *st = S_PGM; break;
    case '6': *st = S_PPM; break;
    }
    if (*st != S_UNK) *ft = F_PNM;
    break;

  case 'B':
    if (id[1] == 'M') {
      *st = S_BMP;
      *ft = F_PNM;
    }
    break;

  case '#':
    if (id[1] == '?') {
      *st = S_HDR;
      *ft = F_PNM;
    }
    break;
  }
}


/* -----------------------------------------------
  scans headers of input file types (decision)
  ----------------------------------------------- */
char *CCoder::scan_header(ISequentialInStream *stream)
{
  char* imghdr;
  char id[2];

  if (!prepare_ex_data(stream, 2)) return NULL;
  memcpy(id, ex_data + ex_next_offset, 2);
  identify( id, &filetype, &subtype );

  // do the actual work
  switch (subtype) {
  case S_PBM:
  case S_PGM:
  case S_PPM:
    imghdr = scan_header_pnm(stream);
    break;
  case S_BMP:
    imghdr = scan_header_bmp(stream);
    break;
  case S_HDR:
    imghdr = scan_header_hdr(stream);
    break;
  default: imghdr = NULL;
  }

  // check image dimensions	
  if ((errorlevel < 2) && ((imgwidth <= 0) || (imgheight <= 0))) {
    sprintf(errormessage, "image contains no data");
    errorlevel = 2;
    free(imghdr);
    return NULL;
  }

  return imghdr;
}


/* -----------------------------------------------
  scans headers of input file types (PNM)
  ----------------------------------------------- */
char *CCoder::scan_header_pnm(ISequentialInStream *stream)
{
  char* imghdr;
  char* ptr0;
  char* ptr1;
  char ws;
  int u, c;
  int cur_ex_offset = ex_next_offset;

  // endian
  endian_l = E_BIG;

  // preset image header (empty string)
  imghdr = (char*)calloc(1024, sizeof(char));
  if (imghdr == NULL) {
    sprintf(errormessage, MEM_ERRMSG);
    errorlevel = 2;
    return NULL;
  }

  // parse and store header
  for (ptr0 = imghdr, ptr1 = imghdr, u = 0; ; ptr1++) {
    if (!prepare_ex_data(stream, cur_ex_offset + 1 - ex_next_offset))
      break;
    *ptr1 = ex_data[cur_ex_offset++];
    if (ptr1 - imghdr >= 1023)
      break;

    if (((*ptr1 == ' ') && (*ptr0 != '#')) || (*ptr1 == '\n') || (*ptr1 == '\r')) {
      ws = *ptr1;
      *ptr1 = '\0';
      if ((strlen(ptr0) > 0) && (*ptr0 != '#')) {
        switch (u) {
        case 0: // first item (f.e. "P5")
          if (strlen(ptr0) == 2) {
            switch (ptr0[1]) {
            case '4': imgbpp = 1; cmpc = 1; u++; break;
            case '5': imgbpp = 8; cmpc = 1; u++; break;
            case '6': imgbpp = 24; cmpc = 3; u++; break;
            default: u = -1; break;
            }
          }
          else u = -1;
          break;
        case 1: // image width
          (sscanf(ptr0, "%i", &imgwidthv) == 1) ? u++ : u = -1; break;
        case 2: // image height
          (sscanf(ptr0, "%i", &imgheight) == 1) ? u++ : u = -1; break;
        case 3: // maximum pixel value
          (sscanf(ptr0, "%ui", &pnmax) == 1) ? u++ : u = -1; break;
        }
        if ((u == 3) && (imgbpp == 1)) u = 4;
        else if (u == -1) break;
      }
      *ptr1 = ws;
      ptr0 = ptr1 + 1;
      if (u == 4) break;
    }
  }

  // check data for trouble
  if (u != 4) {
    sprintf(errormessage, "error in header structure (#%i)", u);
    errorlevel = 2;
    free(imghdr);
    return NULL;
  }
  else *ptr0 = '\0';

  // process data - actual line width
  imgwidth = (imgbpp == 1) ? (((imgwidthv + 7) / 8) * 8) : imgwidthv;

  // process data - pixel maximum value
  if (pnmax > 0) {
    if (pnmax >= 65536) {
      sprintf(errormessage, "maximum value (%i) not supported", (int)pnmax);
      errorlevel = 2;
      free(imghdr);
      return NULL;
    }
    if (pnmax >= 256) imgbpp *= 2;
    if ((pnmax == 0xFF) || (pnmax == 0xFFFF)) pnmax = 0;
  }

  // process data - masks
  switch (imgbpp) {
  case  1: cmask[0] = new cmp_mask(0x1); break;
  case  8: cmask[0] = new cmp_mask(0xFF); break;
  case 16: cmask[0] = new cmp_mask(0xFFFF); break;
  case 24:
    cmask[0] = new cmp_mask(0xFF0000);
    cmask[1] = new cmp_mask(0x00FF00);
    cmask[2] = new cmp_mask(0x0000FF);
    break;
  case 48:
    for (c = 0; c < 3; c++) { // bpp == 48
      cmask[c] = new cmp_mask();
      cmask[c]->m = 0xFFFF;
      cmask[c]->s = 16;
      cmask[c]->p = (2 - c) * 16;
    }
    break;
  default: break;
  }

  ex_next_offset = cur_ex_offset;
  return imghdr;
}


/* -----------------------------------------------
  scans headers of input file types (BMP)
  ----------------------------------------------- */
char *CCoder::scan_header_bmp(ISequentialInStream *stream)
{
  unsigned int bmask[4] = {
    0x00FF0000,
    0x0000FF00,
    0x000000FF,
    0xFF000000
  };
  unsigned int xmask = 0x00000000;
  char* imghdr;
  int bmpv = 0;
  int offs = 0;
  int hdrs = 0;
  int cmode = 0;
  int c;

  // endian
  endian_l = E_LITTLE;

  // preset image header (empty array)
  imghdr = (char*)calloc(2048, sizeof(char));
  if (imghdr == NULL) {
    sprintf(errormessage, MEM_ERRMSG);
    errorlevel = 2;
    return NULL;
  }

  // read the first 18 byte, determine BMP version and validity
  if (prepare_ex_data(stream, 18)) {
    memcpy(imghdr, ex_data + ex_next_offset, 18);
    offs = INT_LE(imghdr + 0x0A);
    hdrs = INT_LE(imghdr + 0x0E);
    switch (hdrs) {
    case 12:  bmpv = 2; break;
    case 40:  bmpv = 3; break;
    case 56:  // special v4
    case 108: bmpv = 4; break;
    case 124: bmpv = 5; break;
    default:  bmpv = 0; break;
    }
  }

  // check plausibility
  if ((bmpv == 0) || (offs > 2048) || (offs < hdrs) || !prepare_ex_data(stream, offs)) {
    sprintf(errormessage, "unknown BMP version or unsupported file type");
    errorlevel = 2;
    free(imghdr);
    return NULL;
  }

  // read remainder data
  memcpy(imghdr + 18, ex_data + ex_next_offset + 18, offs - 18);
  if (bmpv == 2) { // BMP version 2
    imgwidthv = SHORT_LE(imghdr + 0x12);
    imgheight = SHORT_LE(imghdr + 0x14);
    imgbpp = SHORT_LE(imghdr + 0x18);
    cmode = 0;
  }
  else { // BMP versions 3 and 4
    imgwidthv = INT_LE(imghdr + 0x12);
    imgheight = INT_LE(imghdr + 0x16);
    imgbpp = SHORT_LE(imghdr + 0x1C);
    cmode = SHORT_LE(imghdr + 0x1E);
    bmpsize = INT_LE(imghdr + 0x22) + offs;
    pnmax = INT_LE(imghdr + 0x2E);
  }

  // actual width and height
  if (imgheight < 0) imgheight = -imgheight;
  switch (imgbpp) {
    //case 1: imgwidth = ( ( imgwidthv + 7 ) / 8 ) * 8; break;
    //case 4: imgwidth = ( ( imgwidthv + 1 ) / 2 ) * 2; break;
  case  1: imgwidth = ((imgwidthv + 31) / 32) * 32; break;
  case  4: imgwidth = ((imgwidthv + 7) / 8) * 8; break;
  case  8: imgwidth = ((imgwidthv + 3) / 4) * 4; break;
  case 16: imgwidth = ((imgwidthv + 1) / 2) * 2; break;
  default: imgwidth = imgwidthv; break;
  }

  // BMP compatibility check
  if (((cmode != 0) && (cmode != 3)) ||
    ((cmode == 3) && (imgbpp <= 8)) ||
    ((cmode == 0) && (imgbpp == 16))) {
    switch (cmode) {
    case 1: sprintf(errormessage, "BMP RLE8 decoding is not supported"); break;
    case 2: sprintf(errormessage, "BMP RLE4 decoding is not supported"); break;
    default: sprintf(errormessage, "probably not a proper BMP file"); break;
    }
    errorlevel = 2;
    free(imghdr);
    return NULL;
  }

  // read and check component masks
  cmpc = (imgbpp <= 8) ? 1 : ((imgbpp < 32) ? 3 : 4);
  if ((cmode == 3) && (imgbpp > 8)) {
    for (c = 0; c < ((bmpv == 3) ? 3 : 4); c++)
      bmask[c] = INT_LE(imghdr + 0x36 + (4 * c));
    cmpc = (bmask[3]) ? 4 : 3;
    // check mask integers
    for (c = 0, xmask = 0; c < cmpc; xmask |= bmask[c++]);
    if (pnmax > xmask) {
      sprintf(errormessage, "bad BMP \"color used\" value: %i", (int)pnmax);
      errorlevel = 2;
      free(imghdr);
      return NULL;
    }
    xmask ^= 0xFFFFFFFF >> (32 - imgbpp);
  }

  // generate masks
  if (imgbpp <= 8) cmask[0] = new cmp_mask((imgbpp == 1) ? 0x1 : (imgbpp == 4) ? 0xF : 0xFF);
  else for (c = 0; c < cmpc; c++) {
    cmask[c] = new cmp_mask(bmask[c]);
    if ((cmask[c]->p == -1) || ((cmask[c]->p + cmask[c]->s) > imgbpp) || (cmask[c]->s > 16)) {
      sprintf(errormessage, "corrupted BMP component mask %i:%08X", c, bmask[c]);
      errorlevel = 2;
      free(imghdr);
      return NULL;
    }
  }
  if (xmask) cmask[4] = new cmp_mask(xmask);

  ex_next_offset += offs;
  return imghdr;
}


/* -----------------------------------------------
  scans headers of input file types (HDR)
  ----------------------------------------------- */
char *CCoder::scan_header_hdr(ISequentialInStream *stream)
{
  char* imghdr;
  char* ptr0;
  char* ptr1;
  char res[4];
  char ws;
  int cur_ex_offset = ex_next_offset;

  // preset image header (empty string)
  imghdr = (char*)calloc(4096, sizeof(char));
  if (imghdr == NULL) {
    sprintf(errormessage, MEM_ERRMSG);
    errorlevel = 2;
    return NULL;
  }

  // parse and store header
  for (ptr0 = ptr1 = imghdr;; ptr1++) {
    if (!prepare_ex_data(stream, cur_ex_offset + 1 - ex_next_offset) || (ptr1 - imghdr >= 4096)) {
      sprintf(errormessage, "unknown file or corrupted HDR header");
      break;
    }
    *ptr1 = ex_data[cur_ex_offset++];
    if (*ptr1 == '\n') {
      ws = *ptr1;
      *ptr1 = '\0';
      if (ptr0 == imghdr) { // check magic number
        if (strncmp(ptr0 + 1, "?RADIANCE", 9) != 0) {
          sprintf(errormessage, "looked like HDR, but unknown");
          break;
        }
      }
      else {
        if (strncmp(ptr0, "FORMAT", 6) == 0) { // check format
          for (; (ptr0 < ptr1) && (*ptr0 != '='); ptr0++);
          for (ptr0++; (ptr0 < ptr1) && (*ptr0 == ' '); ptr0++);
          if (ptr0 >= ptr1) {
            sprintf(errormessage, "bad HDR FORMAT string");
            break;
          }
          if ((strncmp(ptr0, "32-bit_rle_rgbe", 15) != 0) &&
            (strncmp(ptr0, "32-bit_rle_xyze", 15) != 0)) {
            sprintf(errormessage, "unknown HDR format: %s", ptr0);
            break;
          }
        }
        else if (((ptr0[0] == '-') || (ptr0[0] == '+')) &&
          ((ptr0[1] == 'X') || (ptr0[1] == 'Y'))) {
          if (sscanf(ptr0, "%c%c %i %c%c %i",
            &res[0], &res[1], &imgheight,
            &res[2], &res[3], &imgwidthv) == 6)
            imgwidth = imgwidthv;
          if ((!imgwidth) || ((res[2] != '-') && (res[2] != '+')) ||
            ((res[3] != 'X') && (res[3] != 'Y')) || (res[1] == res[3])) {
            sprintf(errormessage, "bad HDR resolution string: %s", ptr0);
            imgwidth = 0;
            break;
          }
        }
      }
      *ptr1 = ws;
      ptr0 = ptr1 + 1;
      if (imgwidth) {
        *ptr0 = '\0';
        break;
      }
    }
  }

  // check for trouble
  if (!imgwidth) {
    errorlevel = 2;
    free(imghdr);
    return NULL;
  }

  // imgbpp, cmpc, endian
  imgbpp = 32;
  cmpc = 4;
  endian_l = E_BIG;

  // build masks
  cmask[0] = new cmp_mask(0xFF000000);
  cmask[1] = new cmp_mask(0x00FF0000);
  cmask[2] = new cmp_mask(0x0000FF00);
  cmask[3] = new cmp_mask(0x000000FF);

  ex_next_offset = cur_ex_offset;
  return imghdr;
}

/* ----------------------- End of side functions -------------------------- */



} // namespace NPackpnm
} // namespace NPackjpg
} // namespace NCompress
