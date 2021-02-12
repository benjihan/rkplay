/**
 * @file   rklib.c
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

#include "rkplay.h"
#include "rkpriv.h"
#include <string.h>

// GB: XXX DEBUG
#include <stdio.h>

const char * rk_version(void)
{
  return "rkplay " VERSION;
}

int rk_sizeof(void)
{
  return sizeof(rkpla_t);
}

/* ----------------------------------------------------------------------
 *  Data
 * ---------------------------------------------------------------------- */

static const uint16_t Tper[] = {
  0x1AC0,0x1940,0x17D0,0x1680,0x1530,0x1400,0x12E0,0x11D0,
  0x10D0,0x0FE0,0x0F00,0x0E20,

  0x0D60,0x0CA0,0x0BE8,0x0B40,
  0x0A98,0x0A00,0x0970,0x08E8,0x0868,0x07F0,0x0780,0x0710,

  0x06B0,0x0650,0x05F4,0x05A0,0x054C,0x0500,0x04B8,0x0474,
  0x0434,0x03F8,0x03C0,0x0388,

  0x0358,0x0328,0x02FA,0x02D0,
  0x02A6,0x0280,0x025C,0x023A,0x021A,0x01FC,0x01E0,0x01C4,

  0x01AC,0x0194,0x017D,0x0168,0x0153,0x0140,0x012E,0x011D,
  0x010D,0x00FE,0x00F0,0x00E2,

  0x00D6,0x00CA,0x00BE,0x00B4,
  0x00AA,0x00A0,0x0097,0x008F,0x0087,0x007F,0x0078,0x0071,

  0x0071,0x0071,0x0071,0x0071,0x0071,0x0071,0x0071,0x0071,
  0x0071,0x0071,0x0071,0x0071
};

static uint16_t
period(int not, int tra,  int arp)
{
  const int note_min = 0;
  const int note_max = sizeof(Tper) / sizeof(*Tper) - 1;
  int note = not + tra + arp;

  if (note < note_min) {
    note = note_min;
  } else if (note > note_max) {
    note = note_max;
  }
  return Tper[ note ];
}

/* ----------------------------------------------------------------------
 *  Init
 * ---------------------------------------------------------------------- */

static void
init_inst(rkpla_t * P, rkf_ins_t * idef, uint8_t num)
{
  int k;
  rkins_t * I = &P->ins[num];
  rkf_spl_t * def;
  u32_t len;

  I->num = num;

  /* Get sample info */
  def = self(idef->tospl);
  if (!def) {
    return;
  }

  assert( ! memcmp(idef->magic,"inst",4) );

  /* SID */
  I->sidSpd = U8(idef->sidSpd);
  I->sidLen = U8(idef->sidLen);
  I->sidPcm = U8(idef->sidPcm);
  I->sidAlt = U8(idef->sidAlt);
  I->sidPos = U8(idef->sidPos);

  /* ADSR info */
  for ( k=0; k<4; ++k ) {
    I->adsr[k].vol = U8(idef->volVal+k);
    I->adsr[k].inc = U8(idef->volInc+k);
  }

  /* PCM info */
  I->pcmAdr = self(def->todat);
  assert( I->pcmAdr );
  len = U16(def->len) << 1;
  I->sidBas = U16(def->sid);          /* SID mid point or something */

  I->pcmEnd = I->pcmAdr + len;
  if (!U8(idef->one)) {
    I->lpAdr = I->pcmAdr;
    I->lpEnd = I->pcmEnd;
  }

  /* Vibrato info */
  def = self(idef->tovib);
  if (def) {
    I->vibDat = self(def->todat);
    I->vibLen = U16(def->len) << 1;
    I->vibW8t = U8(idef->vibW8t);
    I->vibAmp = U8(idef->vibAmp);
    I->vibSpd = U8(idef->vibSpd);
    assert( I->vibSpd > 0 && I->vibSpd <= I->vibLen );
  }
}

int rk_init(rkpla_t * const P, rkmod_t * const M, int num)
{
  uint8_t * song;
  rkf_ins_t * idef;
  int k;
  const int maxins = sizeof(P->ins) / sizeof(*P->ins);

  if (!P || !M || num < 1 || num > M->nbs)
    return -1;

  memset(P,0,sizeof(*P));

  /* Copy mod info */
  P->mod = M;
  P->arp = (rkarp_t*)M->_arp;
  P->frq = M->frq;
  /* P->end = 0; */
  /* P->tic = 0; */
  P->num = num;

  /* Init instruments */
  assert( M->nbi <= maxins );
  idef = (rkf_ins_t *) M->_ins;
  for (k=0; k<M->nbi; ++k, ++idef)
    init_inst(P, idef, k);
  /* memset(P->ins+k, 0, (maxins-k)*sizeof(*P->ins)); */

  /* Init song and sequences */
  song = (uint8_t*) M->_sng + (num-1)*8;
  for ( k=0; k<4; ++k, song+=2 ) {
    static uint8_t ff = 0xff;
    rkchn_t * C = &P->chn[k];

    /* memset(C,0,sizeof(*C)); */
    C->num    = k;
    C->msk    = 0x111 << k;
    C->sngAdr = self(song);
    C->seqPtr = &ff;            /* Trigger a new sequence read */
    C->sngPtr = C->sngAdr-4;    /* -4 because it's post incremented */
    C->seqRep = 1;
    C->arpAdr = P->arp[0];
    C->curIns = P->ins;
    C->seqW8t = 1;
  }

  return P->frq;
}

static void
trigr_sample(rkchn_t * const C)
{
  assert( C->curIns->pcmAdr );
  C->voice.pcm   = C->curIns->pcmAdr;
  C->voice.end   = C->curIns->pcmEnd;
  C->voice.lpadr = C->curIns->lpAdr;
  C->voice.lpend = C->curIns->lpEnd;
  C->voice.acu   = 0;

  C->trg = 1;
}

static void
seq_read(rkpla_t * P, rkchn_t * C)
{
  int bit = 0;
  assert( ! C->seqW8t );

  C->ptaStp = 0;

  for (;;) {

    if ( C->seqPtr[0] == 0x80 ) {
      /* Arpeggios 0x80,NUM */
      C->arpAdr = P->arp[C->seqPtr[1]];
      C->arpIdx = 0;
      C->seqPtr += 2;
    }

    if ( C->seqPtr[0] == 0x81 ) {
      /* Portamento 0x81,GOAL,SPEED,WAIT */
      C->envIdx = 0;                    /* !!! Note trigger */
      C->ptaNot = C->seqPtr[1];         /* Goal note */
      C->ptaPer = period(C->ptaNot, C->seqTra, 0);
      C->ptaStp = C->seqPtr[2];
      C->seqW8t = C->seqPtr[3] << 2;
      assert( C->seqW8t );
      C->seqPtr += 4;
      break;
    }

    if ( C->seqPtr[0] == 0x82 ) {
      /* Set instrument */
      C->curVol = 0;
      C->envIdx = 0;                    /* Reset ADSR */
      C->curIns = P->ins + C->seqPtr[1];
      trigr_sample(C);

      C->seqPtr += 2;
    }

    if ( C->seqPtr[0] == 0x83 ) {
      assert(!"unsupported command 0x83");
      P->err = 0x83;
      C->seqPtr = 0;
      break;
    }

    if ( C->seqPtr[0] == 0x84 ) {
      /* Envelop speed 0x84,PAR  */
      C->envSpd = C->seqPtr[1];
      C->seqPtr += 2;
    }

    if ( C->seqPtr[0] == 0x85 ) {
      assert(!"unsupported command 0x85");
      P->err = 0x85;
      C->seqPtr = 0;
      break;
    }

    if ( C->seqPtr[0] & 0x80 ) {
      /* GB: Normally sequences end with 0xFF however the original
       * replay only test the MSB. It happens (in Title) that indeed
       * the byte at this point is not 0xFF (but 0x80). While I
       * thought it was a bug it in fact affect the music in a bad way
       * if we try to make the replay more consistent.  I finally
       * chose to correct the data so that all sequences actually end
       * with 0xFF but keep the code working with the unmodified data.
       */

      /* if ( C->seqPtr[0] != 0xFF ) { */
      /*   printf("%c@%05u CMD $%02x instead of $FF\n", */
      /*          'A'+C->num, P->tic, C->seqPtr[0] ); */
      /* } */
      assert ( C->seqPtr[0] == 0xFF );

      if ( -- C->seqRep ) {
        C->seqPtr = self(C->sngPtr);
        C->seqTra = C->sngPtr[2];
        P->evt |= 0xF00 & C->msk;
      } else {
        C->sngPtr += 4;
        C->seqPtr = self(C->sngPtr);
        if (!C->seqPtr) {
          C->seqPtr = self(C->sngPtr = C->sngAdr);
          if ( ! (0x00F & P->evt & C->msk) )
            C->ticLen = P->tic - 1;
          P->evt |= 0x0FF & C->msk;
        }
        C->seqTra = C->sngPtr[2];
        C->seqRep = C->sngPtr[3];
      }

      bit = 0;

    } else {
      i16_t w8t;

      C->seqNot = C->seqPtr[0];
      w8t = C->seqPtr[1];
      C->seqPtr += 2;
      C->curPer = period(C->seqNot, C->seqTra, 0);
      if (w8t) {
        trigr_sample(C);
        C->seqW8t = w8t << 2;
        assert( C->seqW8t );
        C->envIdx = 0;
        C->curVol = 0;
        break;
      }
    }
  }
  assert ( C->seqW8t );

  C->vibIdx = 0;
  if (C->curIns->vibW8t)
    C->vibW8t = (C->curIns->vibW8t << 2) - 1;
}

static void
do_asid(rkchn_t * const C)
{
  rkins_t * const I = C->curIns;

  if (I->sidSpd) {
    if (C->sidW8t)
      --C->sidW8t;
    else {
      int8_t * pcm;

      C->sidW8t = I->sidSpd - 1;

      pcm  = I->pcmAdr;
      pcm += I->sidBas;
      pcm -= I->sidLen;

      assert (I->sidPos >= 0 && I->sidPos <= I->sidLen*2);

      if(!I->sidAlt) {
        pcm[I->sidPos] = I->sidPcm;
        if (++I->sidPos == I->sidLen*2) {
          I->sidAlt = ~I->sidAlt;
          I->sidPcm = ~I->sidPcm;
        }
      } else {
        pcm[I->sidPos] = I->sidPcm;
        if (--I->sidPos == 0) {
          I->sidAlt = ~I->sidAlt;
          I->sidPcm = ~I->sidPcm;
        }
      }
    }
  }
}

static i16_t
do_arpeggio(rkchn_t * const C)
{
  i16_t per;
  assert( C->arpAdr && C->arpIdx >= 0 && C->arpIdx < 12);
  per = period( C->seqNot, C->seqTra, C->arpAdr[ C->arpIdx ] );
  if ( --C->arpIdx < 0 )
    C->arpIdx = 11;
  return per;
}

static i16_t
do_portamento(rkchn_t * const C)
{
  i16_t per = C->curPer;
  assert( C->ptaStp );
  if (per < C->ptaPer) {
    per += C->ptaStp;
    if (per >= C->ptaPer)
      per = C->ptaPer;
  } else {
    per -= C->ptaStp;
    if (per <= C->ptaPer)
      per = C->ptaPer;
  }
  return per;
}

static i16_t
do_vibrato(rkchn_t * const C)
{
  i16_t per = 0;

  if ( C->curIns->vibSpd ) {
    if (C->vibW8t > 0)
      --C->vibW8t;
    else {
      const rkins_t * const I = C->curIns;

      assert( I->vibLen > 2 );
      assert( I->vibDat );
      assert( C->vibIdx >= 0 && C->vibIdx < I->vibLen );

      per = I->vibDat[C->vibIdx] * I->vibAmp;
      C->vibIdx -= I->vibSpd;
      if (C->vibIdx < 0)
        C->vibIdx += I->vibLen;
    }
  }
  return per;
}

static i16_t
do_period(rkchn_t * const C)
{
  C->curPer = C->ptaStp
    ? do_portamento(C)
    : do_arpeggio(C)
    ;
  return C->curPer + do_vibrato(C);
}

static i16_t
do_envelop(rkchn_t * const C)
{
  if ( --C->envW8t < 0 ) {
    int vol = C->curVol;
    int idx = C->envIdx;
    const int aim = C->curIns->adsr[idx].vol;
    const int inc = C->curIns->adsr[idx].inc;

    assert( idx >= 0 && idx < 4 );
    assert( vol >= 0 && vol < 0x40 );
    assert( aim >= 0 && aim < 0x40 );

    C->envW8t = C->envSpd;

    if (vol >= aim) {
      vol -= inc;
      if (vol <= aim) {
        vol = aim;
        ++idx;
      }
    } else {
      vol += inc;
      if (vol >= aim) {
        vol = aim;
        ++idx;
      }
    }
    if (idx > 3) idx = 3;
    assert( idx >= 0 && idx < 4 );
    assert( vol >= 0 && vol < 0x40 );
    C->curVol = vol;
    C->envIdx = idx;
  }

  return C->curVol;
}


static void
rk_play_chan(rkpla_t * const P, rkchn_t * const C)
{
  C->trg    = 0;
  C->oldVol = C->endVol;
  C->oldPer = C->endPer;
  C->oldIns = C->curIns;

  assert ( C->seqW8t >= 1 );

  if (!--C->seqW8t) {
    seq_read(P, C);
    C->endPer = C->curPer;
    C->endVol = C->curVol;
  }
  else {
    do_asid(C);
    C->endPer = do_period(C);
    C->endVol = do_envelop(C);
  }
}

int rk_play(rkpla_t * const P)
{
  u8_t k;
  ++ P->tic;
  P->evt &= 0x0F;
  for (k=0; k<4 && !P->err; ++k)
    rk_play_chan(P,&P->chn[k]);
  return P->err ? -P->err : P->evt;
}

static inline int16_t lagrange(i32_t p1, i32_t p2, i32_t p3, u32_t idx)
{
  const i32_t j = (idx >> 9) & 0x7F; /* the mid point is f(.5) */

  /* f(x) = ax^2+bx+c */
  const i32_t c =    p1            ;
  const i32_t b = -3*p1 +4*p2 -  p3;
  const i32_t a =  2*p1 -4*p2 +2*p3;

  /* x is fp8; x^2 is fp16; r is fp16 => 24bit */
  i32_t r =
    ( ( a * j * j) +
      ( b * j << 8 ) +
      ( c << 16 )
      );
  r >>= 8;
  if (r < -0x8000)
    return -0x8000;
  else if (r > 0x7fff)
    return 0x7fff;
  return r;
}


static void
mix_voice(int16_t * mix, int n, voice_t * V)
{
  const u32_t lplen = V->lpend - V->lpadr;

  if (V->pcm) {
    while (--n >= 0) {
      *(mix) += (*V->pcm * V->vol) >> 7;
      mix += 2;
      V->vol += V->vtp;
      V->acu += V->stp;
      V->pcm += V->acu >> 16;
      V->acu &= 0xFFFF;
      if ( V->pcm >= V->end ) {
        if (!V->lpadr) {
          V->pcm = 0;
          V->acu = 0;
          break;
        } else {
          V->pcm = V->lpadr + ( V->pcm - V->end ) % lplen;
          V->end = V->lpend;
        }
      }
    }
  }
}

static inline u32_t
calc_step(u32_t per, u32_t spr)
{
  const uint64_t clk = 7093789LL << 15;
  assert( per );
  return clk / (per * spr);
}

static void
rk_mix_chan(rkchn_t * const C, int16_t * mix, u16_t ppt, u32_t spr)
{
#if 1
  C->voice.vol = C->oldVol << 8;
  C->voice.vtp = ( (C->endVol-C->oldVol) << 8 ) / (int)ppt;
#else
  C->voice.vol = C->endVol << 8;
  C->voice.vtp = 0;
#endif

  if (C->voice.pcm) {
    struct stat * st = & C->curIns->stats[C->num];

    if (!st->count++) {
      st->perMin = st->perMax = C->endPer;
      st->volMin = st->volMax = C->endVol;
    }
    else if (C->endPer < st->perMin)
      st->perMin = C->endPer;
    else if (C->endPer > st->perMax)
      st->perMax = C->endPer;
    else if (C->endVol < st->volMin)
      st->volMin = C->endVol;
    else if (C->endVol > st->volMax)
      st->volMax = C->endVol;
  }

  C->voice.stp = calc_step(C->endPer, spr);
  mix_voice(mix, ppt, &C->voice);
}

void rk_mix(rkpla_t * const P, void * mix, int ppt, int spr, int mute)
{
  int16_t * const m16 = mix;

  memset(mix, 0, ppt*4);
  if (P->err) return;
  P->spr = spr;

  if ( ! (mute & 1) ) rk_mix_chan(&P->chn[0], m16+0, ppt, spr);
  if ( ! (mute & 2) ) rk_mix_chan(&P->chn[1], m16+1, ppt, spr);
  if ( ! (mute & 4) ) rk_mix_chan(&P->chn[2], m16+1, ppt, spr);
  if ( ! (mute & 8) ) rk_mix_chan(&P->chn[3], m16+0, ppt, spr);
}
