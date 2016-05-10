#pragma once

#define FRAME_MIN_SZ 40


/* -----------------------------------------------
	function declarations: main functions
	----------------------------------------------- */

void reset_ex_buffers(int default_next_offset);
HRESULT seek_mp3(ISequentialInStream *inStream, int *processedSize);
HRESULT read_mp3(ISequentialInStream *inStream, UInt64 *inSizeProcessed);
bool write_mp3(ISequentialOutStream *outStream, UInt64 *outSizeProcessed);
bool analyze_frames(void);
bool compress_mp3(ISequentialOutStream *outStream, UInt64 *outSizeProcessed);
int check_archive_type(ISequentialInStream *inStream, bool *remained);
bool uncompress_pmp(ISequentialInStream *inStream, UInt64 *inSizeProcessed);


/* -----------------------------------------------
	LIBBSC C=f E=2 B=20
	----------------------------------------------- */

/*
 * which
 *  0 : ex_data, processedSize
 *  1 : ex_backup, ex_back_size
 **/
HRESULT compress_replacement(ISequentialOutStream *outStream, int which, UInt64 *inSizeProcessed, UInt64 *outSizeProcessed);
HRESULT uncompress_replacement(ISequentialInStream * inStream, ISequentialOutStream *outStream, UInt64 *inSizeProcessed, UInt64 *outSizeProcessed);


/* -----------------------------------------------
global variables: data storage
----------------------------------------------- */

// extern mp3Frame*      mp3_firstframe;	    // first physical frame
// extern mp3Frame*      mp3_lastframe;	    // last physical frame
// extern unsigned char* mp3_main_data;	    // (mainly) huffman coded data
// extern int            mp3_main_data_size;	// size of main data
// extern int            mp3_n_bad_first;    // # of bad first frames (should be zero!)


