// PackpnmCoder.h

#ifndef __PACKPNM_CODER_H
#define __PACKPNM_CODER_H

#include "C/7zTypes.h"
#include "CPP/7zip/Compress/packjpg/bitops.h"
#include "CPP/7zip/Compress/packjpg/aricoder.h"
#include "ppnmbitlen.h"

#include "CPP/Common/MyCom.h"

#include "CPP/7zip/ICoder.h"

namespace NCompress {
namespace NPackjpg {
namespace NPackpnm {

#define INIT_MODEL_S(a,b,c) new model_s( a, b, c, 255 )
#define INIT_MODEL_B(a,b)   new model_b( a, b, 255 )

#define ABS(v1)         ( (v1 < 0) ? -v1 : v1 )
#define ABSDIFF(v1,v2)  ( (v1 > v2) ? (v1 - v2) : (v2 - v1) )
#define ROUND_F(v1)     ( (v1 < 0) ? (int) (v1 - 0.5) : (int) (v1 + 0.5) )
#define CLAMPED(l,h,v)  ( ( v < l ) ? l : ( v > h ) ? h : v )
#define SHORT_BE(d)     ( ( ( (d)[0] & 0xFF ) << 8 ) | ( (d)[1] & 0xFF ) )
#define SHORT_LE(d)     ( ( (d)[0] & 0xFF ) | ( ( (d)[1] & 0xFF ) << 8 ) )
#define INT_BE(d)       ( ( SHORT_BE( d ) << 16 ) | SHORT_BE( d + 2 ) )
#define INT_LE(d)       ( SHORT_LE( d ) | ( SHORT_LE( d + 2 ) << 16 ) )

#define MEM_ERRMSG      "out of memory error"
#define FRD_ERRMSG      "could not read file / file not found"
#define FWR_ERRMSG      "could not write file / file write-protected"
#define MSG_SIZE        128
#define BARLEN          36

/* -----------------------------------------------
  pjg_model class
  ----------------------------------------------- */

class pjg_model {
public:
  model_s* len;
  model_b* sgn;
  model_b* res;
  pjg_model(int pnmax, int nctx = 2) {
    int ml = BITLENB16N(pnmax);
    len = INIT_MODEL_S(ml + 1, ml + 1, nctx);
    sgn = INIT_MODEL_B(16, 1);
    res = INIT_MODEL_B(ml + 1, 2);
  };
  ~pjg_model() {
    delete(len);
    delete(sgn);
    delete(res);
  };
};


/* -----------------------------------------------
  cmp mask class
  ----------------------------------------------- */

class cmp_mask {
public:
  unsigned int m;
  int p, s;
  cmp_mask(unsigned int mask = 0) {
    if (mask == 0) m = p = s = 0;
    else {
      for (p = 0; (mask & 1) == 0; p++, mask >>= 1);
      m = mask;
      for (s = 0; (mask & 1) == 1; s++, mask >>= 1);
      if (mask > 0) { m = 0; p = -1; s = -1; };
    }
  };
  ~cmp_mask() {};
};


class CCoder
{
protected:
  CCoder();

  /* -----------------------------------------------
    function declarations: main functions
    ----------------------------------------------- */
  bool reset_buffers(void);

  /* -----------------------------------------------
    function declarations: side functions
    ----------------------------------------------- */

  bool ppn_encode_imgdata_rgba(aricoder *enc, ISequentialInStream *stream, UInt64 *inSizeProcessed);
  bool ppn_decode_imgdata_rgba(aricoder *dec, ISequentialOutStream *stream, UInt64 *outSizeProcessed);
  bool ppn_encode_imgdata_mono(aricoder *enc, ISequentialInStream *stream, UInt64 *inSizeProcessed);
  bool ppn_decode_imgdata_mono(aricoder *dec, ISequentialOutStream *stream, UInt64 *outSizeProcessed);
  bool ppn_encode_imgdata_palette(aricoder *enc, ISequentialInStream *stream, UInt64 *inSizeProcessed);
  bool ppn_decode_imgdata_palette(aricoder *dec, ISequentialOutStream *stream, UInt64 *outSizeProcessed);
  inline void ppn_encode_pjg(aricoder *enc, pjg_model *mod, int **val, int **err, int ctx3);
  inline void ppn_decode_pjg(aricoder *dec, pjg_model *mod, int **val, int **err, int ctx3);
  inline int get_context_mono(int x, int y, int **val);
  inline int plocoi(int a, int b, int c);
  inline int pnm_read_line(ISequentialInStream *stream, int **line, UInt64 *inSizeProcessed);
  inline HRESULT pnm_write_line(ISequentialOutStream *stream, int **line, UInt64 *outSizeProcessed);
  inline int hdr_decode_line_rle(ISequentialInStream *stream, int **line, UInt64 *inSizeProcessed);
  inline int hdr_encode_line_rle(ISequentialOutStream *stream, int **line, UInt64 *outSizeProcessed);
  inline void rgb_process(unsigned int *rgb);
  inline void rgb_unprocess(unsigned int *rgb);
  inline void identify(const char* id, int *ft, int *st);
  char* scan_header(ISequentialInStream *stream);
  inline char* scan_header_pnm(ISequentialInStream *stream);
  inline char* scan_header_bmp(ISequentialInStream *stream);
  inline char* scan_header_hdr(ISequentialInStream *stream);

  /* -----------------------------------------------
    global variables: data storage
    ----------------------------------------------- */

  int imgwidth;       // width of image
  int imgheight;      // height of image
  int imgwidthv;      // visible width of image
  int imgbpp;         // bit per pixel
  int cmpc;           // component count
  int endian_l;       // endian of image data
  unsigned int pnmax; // maximum pixel value (PPM/PGM only!)
  cmp_mask* cmask[5]; // masking info for components
  int bmpsize;        // file size according to header

  /* -----------------------------------------------
    global variables: info about files
    ----------------------------------------------- */

  int    filetype;    // type of current file
  int    subtype;     // sub type of file

  /* -----------------------------------------------
    global variables: messages
    ----------------------------------------------- */

  char errormessage[128];
  // INTERN bool (*errorfunction)();
  int  errorlevel;
  // meaning of errorlevel:
  // -1 -> wrong input
  // 0 -> no error
  // 1 -> warning
  // 2 -> fatal error


  /* -----------------------------------------------
    global variables: settings
    ----------------------------------------------- */

  bool use_rle;   // use RLE compression for HDR output


  /* -----------------------------------------------
    global variables: info about program
    ----------------------------------------------- */

  const unsigned char appversion = 16;
};


}}}

#endif
