/**
 * @file   rkplay.h
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

#ifndef RKPLAY_H
#define RKPLAY_H

typedef struct rkpla rkpla_t;
typedef struct rkmod rkmod_t;

const char * rk_version(void);
int rk_sizeof(void);
int rk_init(rkpla_t * P, rkmod_t * M, int num);
int rk_play(rkpla_t * const P);
void rk_mix(rkpla_t * P, void * mix, int ppt, int spr, int mute);
rkmod_t * rk_load(const char * fname, int *perr);

#endif /* #ifndef RKPLAY_H */
