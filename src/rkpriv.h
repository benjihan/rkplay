/**
 * @file   rkpriv.h
 * @data   2018-01-28
 * @author Benjamin Gerard
 * @brief  Ron Klaren's Battle Squadron Music Player
 *
 * ----------------------------------------------------------------------
 *
 * MIT License
 *
 * Copyright (c) 2018 Benjamin Gerard AKA Ben/OVR.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef RK_PRIV_H
#define RK_PRIV_H

#include <stdint.h>
#include <assert.h>

#define RKMAXINST 24

typedef struct rkf_header rkf_hd_t;
struct rkf_header {
  uint8_t rki[4];
  uint8_t siz[2];
  uint8_t frq[1];
  uint8_t nbs[1];
  uint8_t nba[1];
  uint8_t nbi[1];
  uint8_t tosng[2];
  uint8_t toarp[2];
  uint8_t toins[2];
};

typedef  int_fast8_t  i8_t;
typedef uint_fast8_t  u8_t;
typedef  int_fast16_t i16_t;
typedef uint_fast16_t u16_t;
typedef  int_fast32_t i32_t;
typedef uint_fast32_t u32_t;
typedef uint8_t       rkarp_t[12];

typedef struct rkf_spl rkf_spl_t;
struct rkf_spl {
  uint8_t todat[2];
  uint8_t len[2];
  uint8_t sid[2];
};

typedef struct rkf_ins rkf_ins_t;
struct rkf_ins {
  uint8_t tospl[2];                     /* 00 */
  uint8_t tovib[2];                     /* 02 */
  uint8_t magic[4];                     /* 04 "inst" */
  uint8_t one[1];                       /* 08 */
  uint8_t sidSpd[1];                    /* 09 */
  uint8_t sidLen[1];                    /* 0A */

  uint8_t vibSpd[1];                    /* 0B */
  uint8_t vibAmp[1];                    /* 0C */
  uint8_t vibW8t[1];                    /* 0D */

  uint8_t volVal[4];                    /* 0E */
  uint8_t volInc[4];                    /* 12 */

  uint8_t sidPcm[1];                    /* 16 */
  uint8_t sidAlt[1];                    /* 17 */
  uint8_t sidPos[1];                    /* 18 */
  uint8_t xxx[7];
};

typedef struct rk_ins rkins_t;
struct rk_ins {
  u8_t num;

  int8_t * pcmAdr, * pcmEnd, * lpAdr, * lpEnd, * vibDat;
  int      vibLen, vibSpd, vibAmp, vibW8t;

  i16_t   sidSpd;
  i16_t   sidBas;                       /* from sample def */
  i16_t   sidLen;                       /* half len (I[A]) */
  uint8_t sidAlt;                       /* I[17] */
  i16_t   sidPos;                       /* I[18] */
  int8_t  sidPcm;                       /* I[16] */

  struct adsr {
    uint8_t vol, inc;
  } adsr[4];

  struct stat {
    u32_t count;
    uint16_t perMin, perMax;
    uint16_t volMin, volMax;
  } stats[4];

};

typedef struct rkmod rkmod_t;
struct rkmod {
  uint8_t * _sng, * _arp, * _ins;
  u16_t siz;
  u8_t  frq;
  u8_t  nbs;
  u8_t  nba;
  u8_t  nbi;
  uint8_t raw[1];
};

typedef struct rkf_sng rksng_t;
struct rkf_sng {
  uint8_t seq[2];
  uint8_t tra[1];
  uint8_t rep[1];
};

typedef struct voice voice_t;
struct voice {
  int8_t * pcm, *end, * lpadr, * lpend;
  u16_t vol, vtp;
  u32_t acu, stp;
};

typedef struct rkchn rkchn_t;
struct rkchn {
  u16_t   msk;
  uint8_t num;
  uint8_t trg;

  uint8_t * sngAdr;
  uint8_t * sngPtr;
  uint8_t * seqPtr;

  rkins_t * oldIns;
  rkins_t * curIns;

  i16_t endPer, oldPer, curPer, ptaPer;
  i16_t endVol, oldVol, curVol;

  i16_t seqW8t;

  uint8_t seqNot;
  uint8_t ptaNot;
  uint8_t ptaStp;

  uint8_t seqRep;

  uint8_t seqTra;
  uint8_t fx84_1;

  uint8_t * arpAdr;
  i16_t   arpIdx;

  uint8_t envIdx;
  uint8_t envSpd;

  i16_t   envW8t;

  i16_t   sidW8t;

  i16_t   vibIdx;
  i8_t    vibW8t;

  u32_t   ticLen;
  voice_t voice;
};

typedef struct rkpla rkpla_t;
struct rkpla {
  rkmod_t  * mod;
  uint8_t  * raw;                       /* "r.k. module */
  rkarp_t  * arp;                       /* Arpeggio table in "r.k." */

  u16_t      evt;
  u32_t      tic;                       /* current tic */
  u32_t      spr;                       /* last sampling rate used */
  uint8_t    num;                       /* Currenly playing */
  uint8_t    frq;                       /* Tick rate */
  uint8_t    err;
  rkins_t    ins[RKMAXINST];
  rkchn_t    chn[4];
};

#if defined __m68k__
static u8_t   U8( const uint8_t * v) { return *v; }
static u16_t U16( const uint8_t * v) { return *(uint16_t*)v; }
static i16_t S16( const uint8_t * v) { return *(int16_t*)v; }
#else
static u8_t   U8( const uint8_t * v) { return *v; }
static u16_t U16( const uint8_t * v) { return (v[0]<<8)|v[1]; }
static i16_t S16( const uint8_t * v) { return (int16_t)((v[0]<<8)|v[1]); }
#endif

static void * self(const uint8_t * v) {
  const i16_t off = S16(v);
  return (void *) ( off ? &v[off] : 0 );
}

#endif /* #ifndef RK_PRIV_H */
