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

static const char copyright[] = \
  "Copyright (c) 2018 Benjamin Gerard AKA Ben^OVR";
static const char license[] = \
  "Licensed under MIT license";
static const char bugreport[] = \
  "Report bugs to <https://github.com/benjihan/rkplay/issues>";

#define _DEFAULT_SOURCE
#define _GNU_SOURCE			/* for GNU basename() */

enum {
  SPR_DEF = 48000
};

enum {
  OUT_IS_LIVE, OUT_IS_WAVE, OUT_IS_NULL, OUT_IS_FILE
};

static const char typename[][5] = {
  "live","wave","null","file"
};

enum {
  RK_OK, RK_ERR, RK_ARG, RK_INP, RK_OUT
};

/* ----------------------------------------------------------------------
 * Includes
 * ---------------------------------------------------------------------- */

#include "rkplay.h"

/* stdc */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <getopt.h>

#ifdef WIN32
#ifdef __MINGW32__
# include <libgen.h>	 /* no GNU version of basename() with mingw */
#endif
#include <io.h>
#include <fcntl.h>
#endif

#include "ao/ao.h"

void rk_print_stats(rkpla_t * const P); /* rkload.c */

/* ----------------------------------------------------------------------
 * Local declarations
 * ---------------------------------------------------------------------- */

static long mystrtoul(char **s, const int base);
static int uint_mute(char * arg, char * name);

static int opt_mute, opt_ignore, opt_outtype = OUT_IS_LIVE, opt_stats;
static long opt_spr = SPR_DEF;
static char * opt_output, * opt_input;
static char * prgname;

/* ----------------------------------------------------------------------
 * Usage and version
 * ----------------------------------------------------------------------
 */

/**
 * Print usage message.
 */
static void print_usage(void)
{
  int i;
  const char * name="?", * desc;

  puts (
    "Usage: rkplay [OPTIONS] <song.rk>" "\n"
    "\n"
    "  Ron Klaren's BattleSquadron music player\n"
    "\n"
#ifndef NDEBUG
    "  -------> /!\\ DEBUG BUILD /!\\ <-------\n"
    "\n"
#endif

    "OPTIONS:\n"
    " -h --help --usage  Print this message and exit.\n"
    " -V --version       Print version and copyright and exit.\n"
    " -r --rate=         Set sampling rate (support `k' suffix).\n"
    " -m --mute=CHANS    Mute selected channels (bit-field or string).\n"
    " -o --output=URI    Set output file name (-w or -c).\n"
    " -c --stdout        Output raw PCM to stdout or file (native 16-bit).\n"
    " -n --null          Output to the void.\n"
    " -w --wav           Generated a .wav file.\n"
    " -s --stats         Print various statistics on exit.\n"
    );

  puts(
    "OUTPUT:\n"
    " Options `-n/--null',`-c/--stdout' and `-w/--wav' are used to set the\n"
    " output type. The last one is used. Without it the default output type\n"
    " is used which should be playing sound via the default or configured\n"
    " libao driver.\n"
    "\n"
    " The `-o/--output' option specify the output depending on the output\n"
    " type.\n"
    "\n"
    " `-n/--null'    output is ignored\n"
    " `-c/--stdout'  output to the specified file instead of `stdout'.\n"
    " `-w/--wav'     unless set output is a file based on song filename.\n"
    "\n"
    "CHANS:\n"
    " Select channels to be either muted or ignored. It can be either:\n"
    " . an integer representing a mask of selected channels (C-style prefix)\n"
    " . a string containing the letter A to D (case insensitive) in any order\n"
    );
  puts(copyright);
  puts(license);
  puts(bugreport);
}

/**
 * Print version and copyright message.
 */
static void print_version(void)
{
  printf("%s"
#ifndef NDEBUG
	 " (DEBUG BUILD)"
#endif
	 "\n", rk_version());
  puts(copyright);
  puts(license);
}

/* ----------------------------------------------------------------------
 * Writers
 * ---------------------------------------------------------------------- */

static FILE * infofile = 0;

void rklog_va(const char * fmt, va_list list)
{
  assert(infofile);
  if (infofile)
    vfprintf(infofile,fmt,list);
}

void rklog(const char * fmt, ...)
{
  va_list list;
  va_start(list,fmt);
  rklog_va(fmt,list);
  va_end(list);
}

static int emsg(const char * fmt, ...)
{
  va_list list;
  va_start(list, fmt);
  fprintf(stderr, "%s: ", prgname);
  vfprintf(stderr, fmt, list);
  va_end(list);
  fflush(stderr);
  return RK_ERR;
}

static int
null_write(const void * data, void * cookie, int n)
{
  return n;
}

static int
file_write(const void * data, void * cookie, int n)
{
  return fwrite(data,1,n,cookie);
}

static int
live_write(const void * data, void * cookie, int n)
{
  return ao_play(cookie, (void*)data, n) == 0
    ? 0 : n;
}

static void * cookie;
static int (* writer)(const void *, void *, int n);

/* ----------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------- */

#define RETURN(CODE) while (1) { ecode = (CODE); goto clean_exit; }

int main(int argc, char **argv)
{
  /* Options */
  static char sopts[] = "hV"  "wcno:" "r:m:i:" "s" ;
  static struct option lopts[] = {
    { "help",	 0, 0, 'h' },
    { "usage",	 0, 0, 'h' },
    { "version", 0, 0, 'V' },
    { "wav",	 0, 0, 'w' },
    { "output",	 1, 0, 'o' },
    { "stdout",	 0, 0, 'c' },
    { "null",	 0, 0, 'n' },
    { "rate=",	 1, 0, 'r' },
    { "mute=",	 1, 0, 'm' },
    { "ignore=", 1, 0, 'i' },
    { "stats",	 0, 0, 's' },
    { 0 }
  };

  /* libao */
  int		    aoini = 0;
  ao_device	   *aodev = 0;
  ao_info	   *aoinf = 0;
  ao_sample_format  aofmt;
  int		    aoid;

  int i=1, n, ecode = RK_ERR, c, ppt;
  rkpla_t * P = 0;
  rkmod_t * M = 0;
  void	  * mix = 0;
  unsigned long tics ,msecs, rate;

  prgname = basename(argv[0]);
  if (!prgname) prgname = argv[0];

  argv[0] = prgname;
  while ((c = getopt_long (argc, argv, sopts, lopts, 0)) != -1) {
    switch (c) {
    case 'h': print_usage(); return RK_OK;
    case 'V': print_version(); return RK_OK;
    case 's': opt_stats = 1; break;
    case 'w': opt_outtype = OUT_IS_WAVE; break;
    case 'n': opt_outtype = OUT_IS_NULL; break;
    case 'c': opt_outtype = OUT_IS_FILE; break;
    case 'o':
      if (!opt_output) {
	if (opt_outtype == OUT_IS_LIVE)
	  opt_outtype = OUT_IS_FILE;
	opt_output = strdup(optarg);
	if (!opt_output) abort();
      } else {
	emsg("multiple use of option -- %s=%s\n","output",optarg);
	RETURN (RK_ARG);
      }
      break;
    case 'r': {
      char * errp = optarg;
      opt_spr = mystrtoul(&errp, 0);
      if (opt_spr == -1) {
	emsg("invalid sampling rate -- rate=%s\n", optarg);
	RETURN (RK_ARG);
      }
      if (tolower(*errp) == 'k') {
	opt_spr *= 1000u;
	errp++;
      }
      if (opt_spr < 6000 || opt_spr > 96000) {
	emsg("sampling rate out of range -- rate=%s\n", optarg);
	RETURN (RK_ARG);
      }
    } break;
    case 'm':
      if (-1 == (opt_mute = uint_mute(optarg,"mute")))
	RETURN (RK_ARG);
      break;
    case 'i':
      if (-1 == (opt_ignore = uint_mute(optarg,"ignore")))
	RETURN (RK_ARG);
      break;

    case 0: break;
    case '?':
      RETURN (RK_ARG);
    default:
      abort();
    }
  }

  if (optind >= argc) {
    emsg("too few arguments. Try --help\n");
    RETURN (RK_ARG);
  }

  if (optind != argc-1) {
    emsg("too many arguments. Try --help\n");
    RETURN (RK_ARG);
  }

  opt_input = argv[optind];
  if (opt_outtype == OUT_IS_WAVE && !opt_output) {
    /* Generate .wav filename */
    char * b = basename(opt_input);
    char * e = strrchr(b,'.');
    int len = e ? (e - b) : strlen(b);
    opt_output = malloc(len);
    if (!opt_output) abort();
    memcpy(opt_output,b,len);
    memcpy(opt_output+len,".wav",5);
  }

  M = rk_load(opt_input, &ecode);
  if (!M) {
    emsg("load error(%d) -- %s\n", ecode, opt_input);
    RETURN (RK_INP);
  }

  P = malloc(rk_sizeof());
  if (!P) abort();

  rate = rk_init(P,M,1);
  if (rate < 0) {
    emsg("init error -- %s#%u\n",opt_input,1);
    RETURN (RK_INP);
  }

  ppt = (opt_spr+(rate>>1)) / rate;
  mix = malloc( ppt * 4 );
  if (!mix) abort();

  switch (opt_outtype) {
  case OUT_IS_NULL:
    writer = null_write;
    opt_output = strdup("");
    break;
  case OUT_IS_FILE:
    writer = file_write;
    if (opt_output)
      cookie = fopen(opt_output,"wb");
    else {
#if (defined WIN32 || defined _WIN32) && defined _O_BINARY
      int fd = fileno(stdout);
      if (fd != -1)
	_setmode(fd, _O_BINARY);
#endif
      opt_output = strdup("<stdout>");
      if (!opt_output) abort();
      infofile = stderr;
      cookie = stdout;
    }
    break;

  case OUT_IS_LIVE: case OUT_IS_WAVE:
    ao_initialize();
    aoini = 1;
    memset(&aofmt,0,sizeof(aofmt));
    aofmt.bits	      = 16;
    aofmt.rate	      = opt_spr;
    aofmt.channels    = 2;
    aofmt.byte_format = AO_FMT_NATIVE;

    if (opt_outtype == OUT_IS_WAVE) {
      aoid  = ao_driver_id("wav");
      aodev = ao_open_file(aoid, opt_output, 1, &aofmt, 0);
    } else {
      aoid  = ao_default_driver_id();
      aodev = ao_open_live(aoid, &aofmt, 0);
    }
    aoinf = ao_driver_info(aoid);
    if (!aodev) {
      emsg("failed to open audio device -- %s\n", aoinf->short_name);
      RETURN ( RK_OUT );
    }
    opt_spr = aofmt.rate;

    if (aoinf->type == AO_TYPE_LIVE &&
	-1 == asprintf(&opt_output, "%s@%uhz",aoinf->short_name,opt_spr))
      opt_output = 0;
    if (!opt_output) abort();

    cookie = aodev;
    writer = live_write;
  }
  assert(opt_output);

  if (!infofile)
    infofile = stdout;

  rklog("input  : %s\n", opt_input);
  rklog("output : %s: %s\n",typename[opt_outtype],opt_output);

  for (tics=msecs=0;;++tics) {
    int evt = rk_play(P);
    if (evt < 0) {
      emsg("player error (%d/x%02X)\n",evt,255&-evt);
      RETURN ( RK_ERR );
    }
    /* if (evt & 0xFF0) rklog("%05lu %3X\n", tics, evt); */
    msecs = 1000UL * tics / rate;
    if ( (evt & 15) == 15)
      break;
    rk_mix(P, mix, ppt, opt_spr, opt_mute);
    errno = 0;
    if ( writer(mix, cookie, ppt*4) != ppt*4 ) {
      if (!errno)
	emsg("write error\n");
      else
	emsg("write error (%d) %s\n", errno,strerror(errno));
      RETURN( RK_OUT );
    }
  }

  if (1) {
    unsigned long secs = msecs / 1000UL;
    rklog("length : %u'%02u,%03u\" (%lu ticks at %uHz) \n",
	  (unsigned int)(secs/60u),
	  (unsigned int)(secs%60u),
	  (unsigned int)(msecs%1000UL),
	  tics, rate);
  }

  if (opt_stats) {
    rk_print_stats(P);
  }

  /* while ( */
  /* while ( (15 & g_pla.msk) != 15 ) */
  /* { */
  /*   int k; */

  /*   rk_play(&g_pla); */
  /*   rk_mix(&g_pla, mix, ppt, spr); */
  /*   /\* if (g_pla.tic > 0 /\\* 10753 *\\/) *\/ */
  /*   ao_play(aodev, (void*)mix, ppt*2); */

  /*   if (0 && (g_pla.tic&3) == 1) { */
  /*     printf("%X %c |", g_pla.msk , " >"[(g_pla.tic&3) == 1]); */
  /*     for (k=0; k<4; ++k) { */
  /*       rkchn_t * const C = &g_pla.chn[k]; */
  /*       if (C->trg) */
  /*         printf(" %02X", C->curIns->num); */
  /*       else */
  /*         printf(" --"); */
  /*       printf(" %02X %04X |", C->endVol, C->endPer); */
  /*     } */
  /*     printf("\n"); */
  /*     fflush(stdout); */
  /*   } */
  /* } */

  /* if (1) { */
  /*   const unsigned int seconds = g_pla.tic/g_pla.frq; */
  /*   printf("Length: %u tics (%u'%02u\")\n", */
  /*        g_pla.tic, seconds/60u, seconds % 60u ); */
  /* } */

  /* print_stats(&g_pla); */
  /* } */


clean_exit:
  free(mix);
  free(opt_output);
  if (aodev)
    ao_close(aodev);
  if (aoini)
    ao_shutdown();

  return ecode;
}

static long mystrtoul(char **s, const int base)
{
  long v; char * errp;

  errno = 0;
  if (!isdigit((int)**s))
    return -1;
  v = strtoul(*s,&errp,base);
  if (errno)
    return -1;
  *s = errp;
  return v;
}

/**
 * Parse -m/--mute -i/--ignore option argument. Either a string :=
 * [A-D]\+ or an integer {0..15}
 */
static int uint_mute(char * arg, char * name)
{
  int c = tolower(*arg), mute;
  if (c >= 'a' && c <= 'd') {
    char *s = arg;
    mute = 0;
    do {
      mute |= 1 << (c-'a');
    } while (c = tolower(*++s), (c >= 'a' && c <= 'd') );
    if (c) {
      emsg("invalid channels -- %s=%s\n",name,arg);
      mute = -1;
    }
  } else {
    char * errp = arg;
    mute = mystrtoul(&errp, 0);
    emsg("invalid channels -- %s=%s\n",name,arg);
  }
  return mute;
}
