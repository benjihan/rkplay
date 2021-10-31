/**
 * @file   rkplay.c
 * @data   2018-01-28
 * @author Benjamin Gerard
 * @brief  Ron Klaren's Battle Squadron Music Player
 *
 * ----------------------------------------------------------------------
 *
 * MIT License
 *
 * Copyright (c) 2018 Benjamin Gerard AKA Ben^OVR.
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

#include "rkpriv.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int rk_decode_header(rkmod_t * mod)
{
  const rkf_hd_t * const hd = (const rkf_hd_t *)mod->raw;

  /* convert header to host */
  mod->siz = U16(hd->siz);
  mod->frq =  U8(hd->frq);
  mod->nbs =  U8(hd->nbs);
  mod->nba =  U8(hd->nba);
  mod->nbi =  U8(hd->nbi);

  mod->_sng = self(hd->tosng);
  mod->_arp = self(hd->toarp);
  mod->_ins = self(hd->toins);

  return -!(
    mod->siz >= 64 &&
    mod->frq >= 25 && mod->frq < 100 &&
    mod->nbs && mod->nba &&
    mod->nbi && mod->nbi <= RKMAXINST
    );
}

rkmod_t * rk_load(const char * path, int * perr)
{
  int err;
  FILE * f;

  rkmod_t * mod = 0;
  rkf_hd_t hd;
  unsigned int sz;

  assert( sizeof(hd) == 16 );

  err = 1;
  f = fopen(path, "rb");
  if (!f)
    goto error_exit;

  err = 2;
  if (fread(&hd,1,sizeof(hd),f) != sizeof(hd))
    goto error_exit;

  err = 3;
  if (memcmp(hd.rki,"r.k.",4))
    goto error_exit;
  sz = U16(hd.siz);
  if (sz < 64)
    goto error_exit;

  err = 4;
  mod = malloc((intptr_t)(((rkmod_t*)0)->raw)+sz);
  if (!mod)
    goto error_exit;
  memcpy(mod->raw,&hd,sizeof(hd));

  err = 3;
  if (rk_decode_header(mod))
    goto error_exit;

  err = 2;
  sz -= sizeof(hd);
  if (fread(mod->raw+sizeof(hd),1,sz,f) != sz)
    goto error_exit;

  err = 0;
error_exit:
  if (f) fclose(f);
  if (perr) *perr = err;
  if (err) { free(mod); mod = 0; }

  return mod;
}

void rklog(const char * fmt, ...);

static void print_stat(rkins_t *I)
{
  struct stat * st = I->stats;
  int k;

  rklog("\n"
	"I#%02u %c%c%c%c %u bytes%s\n", I->num,
	".A"[!!st[0].count],
	".B"[!!st[1].count],
	".C"[!!st[2].count],
	".D"[!!st[3].count],
	I->pcmEnd - I->pcmAdr,
	!I->lpAdr ? " (1-shot)":"");

  for (k=0; k < 4; ++k, ++st) {
    if (!st->count) continue;
    rklog("   - %c %5ux per{%4d - %4d} vol{%2d - %2d}\n",
	  'A'+k, st->count,
	  st->perMin,st->perMax,
	  st->volMin,st->volMax);
  }
}

void rk_print_stats(rkpla_t * const P)
{
  int i;
  for (i=0; i<P->mod->nbi; ++i)
    if (P->ins[i].pcmAdr)
      print_stat(P->ins+i);
}
