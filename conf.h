
// Headers
#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/zlib.h"
#include "include/ciso.h"
#include "include/menu.h"

//Const defines
#define MAX_CHAR_FILENAME_INF 256
#define MAX_ENTRY 1024
#define MAX_PATH 256
#define printf pspDebugScreenPrintf

// Error msgs
#define error_err_err "error"
#define file_form_err "Ciso file format error\n"
#define file_read_err "File read error       \n"
#define file_open_err "Can't open %s         \n"
#define file_crea_err "Can't create %s\n"
#define file_writ_err "Error writing!        \n"
#define file_cach_err "Read error (caching)  \n"
#define file_size_err "Can't get file size   \n"
#define mem_alloc_err "Can't allocate memory \n"
#define defl_init_err "deflateInit : %s  \n"
#define defl_end_err  "deflateEnd : %s  \n"
#define defl_blck_err "Block %d:deflate : %s[%d]\n"
#define infl_init_err "inflateInit : %s\n"
#define infl_end_err  "inflateEnd : %s\n"
#define infl_blck_err "Block %d:inflate : %s[%d]\n"
#define blck_size_err "Block %d : block size error %d != %d\n"

// Decompress Info
#define dec_file_file "Decompress '%s' to '%s'   \n"
#define dec_blck_blck "Decompressed %i of %i blocks       \n"
#define dec_deco_comp "\nCiso decompress completed\n"

// Compress Info
#define com_file_file "Compress '%s' to '%s'\n"
#define com_com_level "Compress level  %d\n"
#define com_com_avera "Compressed %3d%% avarage rate %3d%%\n"
#define com_comp_comp "ciso compress completed , total size = %8d bytes , rate %d%%\n"
#define com_wr_header "Writing header..."

// General Info
#define inf_keys_fun1  "_________________________________ "
#define inf_keys_fun2  " X  -> Select File | Numblocks:  \\"
#define inf_keys_fun3a " O  -> Parent Dir  |      1      |"
#define inf_keys_fun3b " O  -> Parent Dir  |    %i     |"
#define inf_keys_fun4  " LT -> Numblocks-- |             |"
#define inf_keys_fun5  " RT -> Numblocks++ | Compr. Lv:  |"
#define inf_keys_fun6 " /\\ -> Exit        |      %i      |"
#define inf_file_size "Total File Size %ld bytes \n"
#define inf_blck_size "Block size      %d  bytes \n"
#define inf_blck_tota "Total blocks    %d  blocks\n"
#define inf_indx_algn "Index align     %d        \n"
#define inf_stat_cach "Caching...      "
#define inf_stat_deco "Decompressing..."
#define inf_stat_comp "Compressing...  "
#define inf_stat_writ "Writing...      "
#define inf_stat_clos "Closing Files..."
