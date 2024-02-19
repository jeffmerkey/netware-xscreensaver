/***************************************************************************
*
*   Copyright(c) Jeff V. Merkey 1997-2024.  All rights reserved.
*
*   Portions adapted from xscreensaver loadsnake program is
*   portions Copyright (c) 2007-2011 Cosimo Streppone <cosimo@cpan.org>
*
*   Licensed under the MIT/X License
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to 
*   deal in the Software without restriction, including without limitation 
*   the rights to use, copy, modify, merge, publish, distribute, sublicense, 
*   and/or sell copies of the Software, and to permit persons to whom the 
*   Software is furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included 
*   in all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
*   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
*   DEALINGS IN THE SOFTWARE.
*
*   NetWare SMP style worm XScreensaver for Linux and android 
*
**************************************************************************/

#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <pty.h>
#include <sys/resource.h>

#include "screenhack.h"
#include "colors.h"
#include "textclient.h"
#include "utf8wc.h"

#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
# include "xdbe.h"
#endif /* HAVE_DOUBLE_BUFFER_EXTENSION */

#define WORM_MIN_LEN    4
#define WORM_MAX_LEN   36
#define WORM_TAIL_LEN   3
#define MAX_WORMS      64

// adjust worm total length based on screen area size.  smaller
// displays have shorter worms.  the current logic adjusts for
// worm speedup based on the current max worm length.  Here we
// check a min and max screen area for worm length expansion.

#define AREA_BASE_LEN   (WORM_MAX_LEN / 2)
#define AREA_MINLINES   19
#define AREA_MINCOLS    80
#define AREA_MIN        (AREA_MINLINES * AREA_MINCOLS)
#define AREA_MAXLINES   64
#define AREA_MAXCOLS    160
#define AREA_MAX        (AREA_MAXLINES * AREA_MAXCOLS)
#define AREA            ((st->COLS * st->LINES) < (AREA_MINLINES * AREA_MINCOLS) \
			 ? (AREA_MINLINES * AREA_MINCOLS) : (st->COLS * st->LINES))
#define AREA_DIVISOR    ((AREA_MAX - AREA_MIN) / AREA_BASE_LEN)
#define AREA_EXT_LEN    (((AREA - (AREA_MIN)) / (AREA_DIVISOR)) > \
			AREA_BASE_LEN ? AREA_BASE_LEN : \
			((AREA - (AREA_MIN)) / (AREA_DIVISOR)))

#define MAX_LOADAVG   100
#define MAX_MICROSEC  100000
#define MIN_MICROSEC  10000
#define COLORSETS     16
#define VERBOSE       0

// worm state structure
typedef struct _WORM
{
    int cpu;
    int count;
    int limit;
    int x[WORM_MAX_LEN];
    int y[WORM_MAX_LEN];
    int x_prev[WORM_MAX_LEN];
    int y_prev[WORM_MAX_LEN];
    int length;
    int length_prev;
    int direction;
    int runlength;
    int windowlength;
    int color;
} WORM;

// Netware SMP Xscreesnaver state array
typedef struct _STATE
{
    Display *dpy;
    Window window;
    Visual *visual;
    Screen *screen;
    int delay, wormsize, ncolors, dbuf;
    int cpus;
    XWindowAttributes xgwa;
    GC gc; 
    XColor **colors;
    Pixmap b, ba, bb;
#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
    XdbeBackBuffer backb;
#endif 
    int worm_max_length;
    int COLS;
    int LINES;
    int HEIGHT;
    int WIDTH;
    int cpunum;
    int speedup;
    int divisor;
    int prio;
    int mono;
    int sw, sh;
    unsigned long long usr[MAX_WORMS];
    unsigned long long sys[MAX_WORMS];
    unsigned long long nice[MAX_WORMS];
    unsigned long long idle[MAX_WORMS];
    unsigned long long io[MAX_WORMS];
    unsigned long long irq[MAX_WORMS];
    unsigned long long sirq[MAX_WORMS];
    unsigned long long steal[MAX_WORMS];
    unsigned long long guest[MAX_WORMS];
    unsigned long long guest_nice[MAX_WORMS];
    WORM *worms;
} STATE;

static XColor *make_colorset(STATE *st, const int *rgb, const int *rgb_low) 
{
    int h1, h2 = 0;
    double s1, v1, s2, v2 = 0;
    XColor *colset = (XColor *)calloc(st->ncolors, sizeof(XColor));

    // ramp the colors based on rgb_low array.  If rgb_low not specified,
    // then default to the color black as the ramp color to scale to.
    if (rgb_low) 
       rgb_to_hsv(rgb_low[0], rgb_low[1], rgb_low[2], &h1, &s1, &v1);
    else
       rgb_to_hsv(0, 0, 0, &h1, &s1, &v1);

    // Finish: foreground, head 
    rgb_to_hsv(rgb[0], rgb[1], rgb[2], &h2, &s2, &v2);

    make_color_ramp(
        st->screen, st->visual, st->xgwa.colormap,
        h1, s1, v1,
        h2, s2, v2,
        colset, &st->ncolors,
        True, True, False
    );
    return colset;
}

static XColor **init_colorsets(STATE *st)
{
    int n;

    // monochrome mode rgb bitmap (white)
    int rgbmono[3]= {
       0xFFFF, 0xFFFF, 0xFFFF
    };

    int rgb[COLORSETS][3] = {
	{ 0xFFFF, 0,      0      }, // red 
        { 0,      0,      0xFFFF }, // blue 
        { 0,      0xFFFF, 0      }, // green 
        { 0,      0xFFFF, 0xFFFF }, // cyan 

	{ 0xFFFF, 0xFFFF, 0      }, // yellow 
        { 0xFFFF, 0xFFFF, 0xFFFF }, // britewhite
        { 0xFFFF, 0,      0xFFFF }, // magenta
        { 0x7FFF, 0x7FFF, 0xFFFF }, // purple

	{ 0xFFFF, 0x80FF, 0      }, // orange 
        { 0x0,    0x99FF, 0x4CFF }, // olive 
        { 0x99FF, 0,      0      }, // burgundy 
        { 0xFFFF, 0x99FF, 0x99FF }, // salmon 

	{ 0xCCFF, 0xFFFF, 0x99FF }, // yellowgreen
	{ 0x99FF, 0xCCFF, 0xFFFF }, // ltblue
        { 0xFFFF, 0x99FF, 0xFFFF }, // ltmagenta
        { 0xB8FF, 0x73FF, 0x33FF }, // copper
    };

    // ramp scaling to black for certain colors 
    // will induce too much green or yellow into 
    // the output color ramp array for certain colors.  
    // Set a more reasonable target since the 
    // color ramp libs with xscreensaver utils 
    // don't always handle the color black very 
    // well when scaling and add too much green 
    // or yellow to the ramp colorsets. 

    int rgb_low[COLORSETS][3] = {
	{ 0x1,    0,      0      }, // red target
        { 0,      0,      0x1    }, // blue target
        { 0,      0x1,    0      }, // green target
        { 0,      0x1,    0x1    }, // cyan target

	{ 0x1,    0x1,    0      }, // yellow target
        { 0,      0,      0      }, // britewhite targer
        { 0x1,    0,      0x1    }, // magenta target
        { 0,      0,      0x7    }, // purple target

	{ 0x2,    0x1,    0      }, // orange target
        { 0,      0x9,    0x4    }, // olive target
        { 0x1,    0,      0      }, // burgundy target
        { 0x2,    0x1,    0x1    }, // salmon target

	{ 0x1,    0x1,    0      }, // yellowgreen target
	{ 0x1,    0x2,    0x2    }, // ltblue target
        { 0x2,    0x1,    0x2    }, // ltmagenta target
        { 0,      0,      0      }, // copper target
    };

    XColor** colset = (XColor **)calloc(COLORSETS, sizeof(XColor *));

    if (st->mono) {
       for (n = 0; n < COLORSETS; n++)
           colset[n] = make_colorset(st, rgbmono, NULL);
    }
    else {
       for (n = 0; n < COLORSETS; n++)
           colset[n] = make_colorset(st, rgb[n], rgb_low[n]);
    }
    return colset;
}

static void worm_write(STATE *st, int c, long row, long col, WORM *s, int clear)
{
    int which;
    XColor *snake_colset;

    if (col >= st->COLS)
       return;
    if (row >= st->LINES)
       return;

    if (clear) {
       // clear previous block
       XSetForeground(st->dpy, st->gc, st->colors[0][0].pixel);
    }
    else {
       snake_colset = st->colors[s->color];
       switch (c)
       {
          // head block
          case 0:
	  default:
             which = st->ncolors >> 1;
     	     break;
          // dark shade block
          case 1:
             st->mono 
             ? (which = (st->ncolors >> 1) + 2) 
	     : (which = (st->ncolors >> 1) + 2);
             break;
          // medium shade block
          case 2:
             st->mono 
             ? (which = (st->ncolors >> 1) + 3) 
	     : (which = (st->ncolors >> 1) + 3);
             break;
          // light shade block
          case 3:
             st->mono 
	     ? (which = (st->ncolors >> 1) + 4) 
	     : (which = (st->ncolors >> 1) + 4);
             break;
       }
       XSetForeground (st->dpy, st->gc, snake_colset[which].pixel);
    }
    XFillRectangle(st->dpy, st->b, st->gc, col * st->WIDTH, row * st->HEIGHT, 
		   st->WIDTH, st->HEIGHT);

#if VERBOSE
    printf("Xfill col: %ld row: %ld  COLS: %d LINES: %d\n", col, row, st->COLS, st->LINES);
#endif
    return;
}

static int get_processors(void)
{
    int cpus = 0;
    cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpus < 1)
        cpus = 1;
    return (cpus);
}

static int get_cpu_load(STATE *st, int cpu)
{
    static char line[1024], *s;
    unsigned long long p_usr = 0, p_nice = 0, p_sys  = 0, p_idle = 0;
    unsigned long long load = 0, idle = 0, util = 0, len;
    unsigned long long p_io = 0, p_irq = 0, p_sirq = 0;
    unsigned long long p_steal = 0, p_guest = 0, p_guest_nice = 0;
    FILE *f;
    char src[1024] = "\0";

    if (cpu > st->cpus)
        return 0;

    if (cpu > MAX_WORMS)
        return 0;

    // convert cpu num to ascii text number
    // and null terminate
    snprintf(src, 100, "cpu%d", cpu);
    len = strlen(src);

    if (NULL != (f = fopen("/proc/stat", "r")))
    {
        while (!feof(f) && !load)
	{
            s = fgets(line, sizeof(line) - 1, f);
            if (s && !strncasecmp(src, line, len))
	    {
		p_usr  = st->usr[cpu];
                p_nice = st->nice[cpu];
                p_sys  = st->sys [cpu];
                p_idle = st->idle[cpu];
                p_io = st->io[cpu];
                p_irq = st->irq[cpu];
                p_sirq = st->sirq[cpu];
                p_steal = st->steal[cpu];
                p_guest = st->guest[cpu];
                p_guest_nice = st->guest_nice[cpu];

                if (sscanf(&line[len + 1], "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
			   &(st->usr[cpu]), &(st->nice[cpu]),
			   &(st->sys[cpu]), &(st->idle[cpu]),
			   &(st->io[cpu]), &(st->irq[cpu]),
			   &(st->sirq[cpu]), &(st->steal[cpu]),
                           &(st->guest[cpu]), &(st->guest_nice[cpu])) == 10)
		{
		    // calculate total cycles
		    unsigned long long user, nice;
	            unsigned long long idletime;
	            unsigned long long deltaidle;
	            unsigned long long systime;
	            unsigned long long virtalltime;
	            unsigned long long totaltime;
	            unsigned long long deltatime;

		    // Guest time is already accounted in usertime
		    user = st->usr[cpu] - st->guest[cpu];     
		    nice = st->nice[cpu] - st->guest_nice[cpu];
		    // io is added in the total idle time
                    idletime = st->idle[cpu] + st->io[cpu];
                    systime = st->sys[cpu] + st->irq[cpu] + st->sirq[cpu];
                    virtalltime = st->guest[cpu] + st->guest_nice[cpu];
                    totaltime = user + nice + systime + idletime + 
                                st->steal[cpu] + virtalltime;

		    // Guest time is already accounted in usertime
		    user = p_usr - p_guest;     
		    nice = p_nice - p_guest_nice;
		    // io is added in the total idle time
                    deltaidle = p_idle + p_io;
                    systime = p_sys + p_irq + p_sirq;
                    virtalltime = p_guest + p_guest_nice;
                    deltatime = user + nice + systime + deltaidle + 
                                  p_steal + virtalltime;

                    load = totaltime - deltatime;
                    idle = idletime - deltaidle;
		    // prevent divide by zero if result is 0
		    if (!load) {
                        load = 1;
                        idle = 1;
                    }

		    // subtract idle cycles from load and mulitply * 100
		    // to express as percentage
		    util = (load - idle) * 100 / load;
                    idle = (idle * 100) / load;
		    break;
                }
                else
		{
	            fclose(f);
                    return 0;
		}
            }
        }
	fclose(f);
    }

    len = util * util * st->worm_max_length / 10000.0;
    if (len < WORM_MIN_LEN)
        len = WORM_MIN_LEN;
#if VERBOSE
    printf("Load on cpu %d is %lld%% length %lld idle %lld%%\n", cpu, util, len, idle);
#endif
    return (len);
}

static int get_system_load(void)
{
    char load[100], *s;
    float l1 = 0;
    int l2 = 0;
    FILE *f;

    f = fopen("/proc/loadavg", "r");
    if (f != NULL)
    {
        s = fgets(load, 98, f);
        if (s) {
           sscanf(load, "%f", &l1);
#if VERBOSE
           printf("Load from /proc/loadavg is %f\n", l1);
#endif
	}
        fclose(f);
    }
    else
        l1 = 0;

    // convert from float to integer
    l1 *= 1000.0;
    l2 = (int) l1;
    l2 /= 1000;

    return l2;
}

static void move_worm(STATE *st, WORM *s)
{
    int n = 0, dir = 0;
    int x = 0, y = 0;

    /* worm head position */
    x = s->x[0];
    y = s->y[0];

    /* and direction */
    dir = s->direction;

    /* 0=up, 2=right, 4=down, 6=left */
    switch(dir)
    {
        case 0: y++;      break;
        case 1: y++; x++; break;
        case 2:      x += 2; break;
        case 3: y--; x++; break;
        case 4: y--;      break;
        case 5: y--; x--; break;
        case 6:      x -= 2; break;
        case 7: y++; x--; break;
    }

    /* Check bounds and change direction */
    if (x < 0 && (dir >= 5 && dir <= 7)) {
        x = 1;
        dir -= 4;
    }
    else if (y < 0 && (dir >= 3 && dir <= 5)) {
        y = 1;
        dir -= 4;
    }
    else if (x >= (st->COLS - 2) && (dir >= 1 && dir <= 3)) {
        x = st->COLS - 2;
        dir += 4;
    }
    else if (y >= st->LINES && (dir == 7 || dir == 0 || dir == 1)) {
        y = st->LINES - 1;
        dir += 4;
    }
    else if (s->runlength == 0) {
        int rnd;

	rnd = random() % 128;
	if(rnd > 90)
            dir += 2;
	else if(rnd == 1)
            dir++;
        else if(rnd == 2)
            dir--;
        // set this to the current worm length
	s->runlength = s->length;
    }
    else {
        int rnd;

	s->runlength--;
	rnd = random() % 128;
	if(rnd == 1)
            dir++;
        else if(rnd == 2)
            dir--;
    }

    if (dir < 0)
        dir = -dir;
    dir = dir % 8;

    s->direction = dir;

    /* Copy x,y coords in "tail" positions */
    for(n = s->length - 1; n > 0; n--) {
	s->x[n] = s->x[n-1];
        s->y[n] = s->y[n-1];
    }

    /* New head position */
    s->x[0] = x;
    s->y[0] = y;

}

static int grow_worm(STATE *st, WORM *s)
{
    int newlen = get_cpu_load(st, s->cpu);
    int len = s->length;

#if VERBOSE
    printf("grow: cpu %d len %d newlen %d\n", s->cpu, len, newlen);
#endif
    if (newlen > len) {
        int x, y;

        x = s->x[len - 1];
        y = s->y[len - 1];

        switch(s->direction) {
            case 0: y--;      break;
            case 1: y--; x--; break;
            case 2:      x -= 2; break;
            case 3: y++; x--; break;
            case 4: y++;      break;
            case 5: y++; x++; break;
            case 6:      x += 2; break;
            case 7: y--; x++; break;
        }
        len++;

        if (len >= st->worm_max_length)
            len = st->worm_max_length - 1;

        s->x[len] = x;
        s->y[len] = y;
    }
    else if (newlen < len) {
        len--;
        if (len < WORM_MIN_LEN)
            len = WORM_MIN_LEN;
        s->x[len + 1] = 0;
        s->y[len + 1] = 0;
    }
    s->length = len;
    return (len);
}

static void clear_worm(STATE *st, WORM *s)
{
    int n;

    for (n = s->length_prev - 1; n >= 0; n--) {
       worm_write(st, ' ', s->y_prev[n], s->x_prev[n], s, 1);
       worm_write(st, ' ', s->y_prev[n], s->x_prev[n] + 1, s, 1);
    }
}

/*
For drawing the worm the following integral equation maps the
worm chars to create the effect of the worm moving and expanding.
The logic is non-intuitive but it is described below.  There are
four worm drawing characters in total.  The mapping is defined as:

current char position = n
div = length / 4
mod = length % 4
c = n < (div + 1) * mod ? n / (div + 1) : (n - mod) / div

the above routine produces the following output:

LENGTH    DIV/MOD     WINDOW                CHARACTER MAP
----------------------------------------------------------
length 04 div 1 mod 0 (div + 1) * mod = 00  0 1 2 3
length 05 div 1 mod 1 (div + 1) * mod = 02  0 0 1 2 3
length 06 div 1 mod 2 (div + 1) * mod = 04  0 0 1 1 2 3
length 07 div 1 mod 3 (div + 1) * mod = 06  0 0 1 1 2 2 3
length 08 div 2 mod 0 (div + 1) * mod = 00  0 0 1 1 2 2 3 3
length 09 div 2 mod 1 (div + 1) * mod = 03  0 0 0 1 1 2 2 3 3
length 10 div 2 mod 2 (div + 1) * mod = 06  0 0 0 1 1 1 2 2 3 3
length 11 div 2 mod 3 (div + 1) * mod = 09  0 0 0 1 1 1 2 2 2 3 3
length 12 div 3 mod 0 (div + 1) * mod = 00  0 0 0 1 1 1 2 2 2 3 3 3
length 13 div 3 mod 1 (div + 1) * mod = 04  0 0 0 0 1 1 1 2 2 2 3 3 3
length 14 div 3 mod 2 (div + 1) * mod = 08  0 0 0 0 1 1 1 1 2 2 2 3 3 3
length 15 div 3 mod 3 (div + 1) * mod = 12  0 0 0 0 1 1 1 1 2 2 2 2 3 3 3
length 16 div 4 mod 0 (div + 1) * mod = 00  0 0 0 0 1 1 1 1 2 2 2 2 3 3 3 3
length 17 div 4 mod 1 (div + 1) * mod = 05  0 0 0 0 0 1 1 1 1 2 2 2 2 3 3 3 3
length 18 div 4 mod 2 (div + 1) * mod = 10  0 0 0 0 0 1 1 1 1 1 2 2 2 2 3 3 3 3
length 19 div 4 mod 3 (div + 1) * mod = 15  0 0 0 0 0 1 1 1 1 1 2 2 2 2 2 3 3 3 3
length 20 div 5 mod 0 (div + 1) * mod = 00  0 0 0 0 0 1 1 1 1 1 2 2 2 2 2 3 3 3 3 3
length 21 div 5 mod 1 (div + 1) * mod = 06  0 0 0 0 0 0 1 1 1 1 1 2 2 2 2 2 3 3 3 3 3
length 22 div 5 mod 2 (div + 1) * mod = 12  0 0 0 0 0 0 1 1 1 1 1 1 2 2 2 2 2 3 3 3 3 3
length 23 div 5 mod 3 (div + 1) * mod = 18  0 0 0 0 0 0 1 1 1 1 1 1 2 2 2 2 2 2 3 3 3 3 3
length 24 div 6 mod 0 (div + 1) * mod = 00  0 0 0 0 0 0 1 1 1 1 1 1 2 2 2 2 2 2 3 3 3 3 3 3
length 25 div 6 mod 1 (div + 1) * mod = 07  0 0 0 0 0 0 0 1 1 1 1 1 1 2 2 2 2 2 2 3 3 3 3 3 3
length 26 div 6 mod 2 (div + 1) * mod = 14  0 0 0 0 0 0 0 1 1 1 1 1 1 1 2 2 2 2 2 2 3 3 3 3 3 3
length 27 div 6 mod 3 (div + 1) * mod = 21  0 0 0 0 0 0 0 1 1 1 1 1 1 1 2 2 2 2 2 2 2 3 3 3 3 3 3
length 28 div 7 mod 0 (div + 1) * mod = 00  0 0 0 0 0 0 0 1 1 1 1 1 1 1 2 2 2 2 2 2 2 3 3 3 3 3 3 3
length 29 div 7 mod 1 (div + 1) * mod = 08  0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 2 2 2 2 2 2 2 3 3 3 3 3 3 3
length 30 div 7 mod 2 (div + 1) * mod = 16  0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 3 3 3 3 3 3 3
length 31 div 7 mod 3 (div + 1) * mod = 24  0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3
length 32 div 8 mod 0 (div + 1) * mod = 00  0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3
length 33 div 8 mod 1 (div + 1) * mod = 09  0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3
length 34 div 8 mod 2 (div + 1) * mod = 18  0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3
length 35 div 8 mod 3 (div + 1) * mod = 27  0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3
length 36 div 9 mod 0 (div + 1) * mod = 00  0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3 3
*/

static void draw_worm(STATE *st, WORM *s)
{
    int n, div, mod, c;

    // get character interval and draw worm it is
    // assumed that the minimum worm length is 4
    div = s->length / 4;
    mod = s->length % 4;
    for (n = s->length - 1; n >= 0 && div; n--) {
	   c = n < (div + 1) * mod ? n / (div + 1) : (n - mod) / div;
	   worm_write(st, c % 4, s->y[n], s->x[n], s, 0);
	   worm_write(st, c % 4, s->y[n], s->x[n] + 1, s, 0);
#if VERBOSE
       printf("cpu %d x[n] = %d y[n] = %d n = %d\n",
	      s->cpu, s->x[n], s->y[n], n);
#endif
    }
#if VERBOSE
    printf("\n");
#endif
}

static void save_worm(WORM *s)
{
    int n;

    // save last worm position and coordinates
    // for clearing later
    for (n = s->length - 1; n >= 0; n--) {
        s->x_prev[n] = s->x[n];
        s->y_prev[n] = s->y[n];
    }
    s->length_prev = s->length;
}

static unsigned long run_worms(STATE *st)
{
    float range, increment;
    int n;

    // reset columns and lines in case the screen was resized
    st->worm_max_length = AREA_BASE_LEN + AREA_EXT_LEN;
    if (st->worm_max_length > WORM_MAX_LEN)
       st->worm_max_length = WORM_MAX_LEN;

    for (n = 0; n < st->cpus; n++) {
        WORM *s = (WORM *) &st->worms[n];

       if (++s->count >= s->limit) {
           s->count = 0;
           grow_worm(st, s);
           move_worm(st, s);
           //clear_worm(st, s);
           s->limit = 4 - (s->length / (st->worm_max_length / 4));
#if VERBOSE
           printf("length %d limit %d\n", s->length, s->limit);
#endif
	}
        
        clear_worm(st, s);
        save_worm(s);

        // update all worms even those sleeping to
	// maintain worm overwrite stacking order
	// when one worm overwrites another during
	// display
        draw_worm(st, s);
    }

    // decrease base wait time if system load increases
    // range is 0-100 load average before reaching
    // minimum st->delay wait time
    n = get_system_load();

    range = MAX_MICROSEC - MIN_MICROSEC;
    increment = range / MAX_LOADAVG;
    st->delay = MAX_MICROSEC - (n * increment);
    if (st->delay < MIN_MICROSEC)
        st->delay = MIN_MICROSEC;
    st->delay /= st->divisor;
#if VERBOSE
    printf("delay %d load(n) = %d\n", st->delay, n);
#endif
    return st->delay;
}

static void *netwaresmp_init(Display *dpy, Window window)
{
    STATE *st;
    int n, i;

    st = (STATE *)calloc(1, sizeof(STATE));
    if (!st) {
       fprintf(stderr, "%s: allocation of STATE storage failed\n", progname);
       exit(1);
    }
    memset(st, 0, sizeof(STATE));
    
    XColor fg, bg;
    XGCValues gcv;
   
    st->dpy = dpy;
    st->window = window;

    st->cpunum = get_integer_resource (st->dpy, "cpus", "Integer");
    st->speedup = get_integer_resource (st->dpy, "speedup", "Integer");
    st->delay = get_integer_resource (st->dpy, "delay", "Integer");
    st->wormsize = get_integer_resource(st->dpy, "wormsize", "Integer");
    st->ncolors = 16;
    st->dbuf = get_boolean_resource(st->dpy, "doubleBuffer", "Boolean");
    st->mono = get_boolean_resource (st->dpy, "mono", "Boolean");

#ifdef HAVE_COCO
    st->dbuf = False;
#endif

    XGetWindowAttributes (st->dpy, st->window, &st->xgwa);
    st->visual = st->xgwa.visual;
    st->screen = st->xgwa.screen;
    fg.pixel = get_pixel_resource(st->dpy, st->xgwa.colormap, "foreground", "Foreground");
    bg.pixel = get_pixel_resource(st->dpy, st->xgwa.colormap, "background", "Background");
    XQueryColor (st->dpy, st->xgwa.colormap, &fg);
    XQueryColor (st->dpy, st->xgwa.colormap, &bg);
    gcv.foreground = fg.pixel;
    gcv.background = bg.pixel;
    st->gc = XCreateGC(st->dpy, st->window, GCForeground|GCBackground, &gcv);

    if (st->ncolors < 4) {
       free(st);
       fprintf (stderr, "%s: insufficient colors!\n", progname);
       exit (1);
    }

    if (st->wormsize < 10 || st->wormsize > 80)
    {
       fprintf(stderr, "%s: specified wormsize (%d) must be between 10 and 80, defaulting to 30\n", 
	       progname, st->wormsize);
       st->HEIGHT = 30;
       st->WIDTH = st->HEIGHT / 2;
    }
    else
    {
       st->HEIGHT = st->wormsize;
       st->WIDTH = st->wormsize / 2;
    }

    st->colors = init_colorsets(st);
    st->COLS = st->xgwa.width / st->WIDTH;
    st->LINES = st->xgwa.height / st->HEIGHT;

    st->cpus = get_processors();
    if (!st->cpus) 
    {
       free(st);
       fprintf(stderr, "%s: no processors detected\n", progname);
       exit(1);
    }

    if (st->cpunum > st->cpus)
       st->cpus = st->cpunum;

    if (st->cpus > MAX_WORMS)
       st->cpus = MAX_WORMS;

    if (st->speedup > 0)
       st->divisor = st->speedup;
    else
       st->divisor = 1;

#if VERBOSE
    printf("cols: %d lines: %d base: %d len: %d area: %d"
	   " max: %d min: %d adj: %d divisor: %d\n",
      st->COLS,st->LINES, AREA_BASE_LEN, AREA_EXT_LEN, AREA,
       AREA_MAX, AREA_MIN, (AREA) - (AREA_MIN), AREA_DIVISOR);
#endif

    st->worm_max_length = AREA_BASE_LEN + AREA_EXT_LEN;
    if (st->worm_max_length > WORM_MAX_LEN)
       st->worm_max_length = WORM_MAX_LEN;

    st->worms = (WORM *)calloc(st->cpus, sizeof(WORM));
    if (!st->worms) {
       fprintf(stderr, "%s: could not allocate processor state storage\n", progname);
       free(st);
       exit(1);
    }
    memset(st->worms, 0, st->cpus * sizeof(WORM));

    for (n = 0; n < st->cpus; n++) {
        WORM *s = (WORM *)&st->worms[n];

	s->cpu  = n;
        s->x[0] = random() % (st->COLS - 1);
	s->y[0] = random() % st->LINES;
	for (i=1; i < WORM_MAX_LEN; i++)
	{
           s->x[i] = s->x[0];
           s->y[i] = s->y[0];
	}
	s->direction = ((random() % 9) >> 1) << 1;
	s->length = WORM_MIN_LEN;
        s->runlength = WORM_MIN_LEN;
        s->color = n % COLORSETS;
#if VERBOSE
	printf("worm %d starting at %d,%d dir %d length %d\n",
	       s->cpu, s->x[0], s->y[0], s->direction, s->length);
#endif
    }

    if (st->dbuf) {
#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
        st->b = xdbe_get_backbuffer (st->dpy, st->window, XdbeUndefined);
        st->backb = st->b;
#endif  
      if (!st->b) {
            st->ba = XCreatePixmap (st->dpy, st->window, st->xgwa.width, st->xgwa.height, st->xgwa.depth);
            st->bb = XCreatePixmap (st->dpy, st->window, st->xgwa.width, st->xgwa.height, st->xgwa.depth);
            st->b = st->ba;
        }
    }
    else {
        st->b = st->window;
    }
    return st;
}
static unsigned long netwaresmp_draw(Display *dpy, Window window, void *closure)
{
    STATE *st = (STATE *) closure;

    // set nice value to highest priority since on a heavily loaded system
    // we want the worms to get maximum cycles so the display is not jerky 
    // or intermitently freezes 
    st->prio = getpriority(PRIO_PROCESS, 0);
    setpriority(PRIO_PROCESS, 0, -20);

    run_worms(st);

#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
    if (st->backb) {
        XdbeSwapInfo info[1];
        info[0].swap_window = st->window;
        info[0].swap_action = XdbeUndefined;
        XdbeSwapBuffers (st->dpy, info, 1);
    }
    else
#endif /* HAVE_DOUBLE_BUFFER_EXTENSION */
        if (st->dbuf) {
            XCopyArea (st->dpy, st->b, st->window, st->gc, 0, 0, 
                       st->xgwa.width, st->xgwa.height, 0, 0);
            st->b = (st->b == st->ba ? st->bb : st->ba);
        }

    setpriority(PRIO_PROCESS, 0, st->prio);
    return st->delay;
}

static void netwaresmp_reshape(Display *dpy, Window window, void *closure, 
                 unsigned int w, unsigned int h)
{
    STATE *st = (STATE *) closure; 

    XGetWindowAttributes (st->dpy, st->window, &st->xgwa);
    st->LINES = st->xgwa.height / st->HEIGHT;
    st->COLS = st->xgwa.width / st->WIDTH;

#if VERBOSE
    printf("cols: %d lines: %d base: %d len: %d area: %d"
	   " max: %d min: %d adj: %d divisor: %d\n",
       st->COLS, st->LINES, AREA_BASE_LEN, AREA_EXT_LEN, AREA,
       AREA_MAX, AREA_MIN, (AREA) - (AREA_MIN), AREA_DIVISOR);
#endif
}

static Bool netwaresmp_event(Display *dpy, Window window, void *closure, XEvent *event)
{
        return False;
}

static void netwaresmp_free(Display *dpy, Window window, void *closure)
{
        STATE *st = (STATE *) closure;
        XColor **colset = st->colors;
	int n = COLORSETS;

	while (n--)
                free(colset[n]);
	if (st->colors)
	   free(st->colors);
	if (st->worms)
  	   free(st->worms);
	free(st);
}

static const char *netwaresmp_defaults [] = {
        ".cpus: 1",
        ".speedup: 1",
        ".background: #000000",
        ".foreground: #FF0000",
        "*delay: 100000",
        "*wormsize: 30",
        "*doubleBuffer: False",
#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
        "*useDBE: True",
        "*useDBEClear: True",
#endif 
        0
};

static XrmOptionDescRec netwaresmp_options [] = {
        { "-cpus", ".cpus", XrmoptionSepArg, 0},
        { "-speedup", ".speedup", XrmoptionSepArg, 0},
        { "-fg", ".foreground", XrmoptionSepArg, 0},
        { "-bg", ".background", XrmoptionSepArg, 0},
        { "-delay",     ".delay", XrmoptionSepArg, 0 },
        { "-wormsize", ".wormsize", XrmoptionSepArg, 0 },
        { "-mono",        ".mono", XrmoptionNoArg, "True" },
        { "-db",        ".doubleBuffer", XrmoptionNoArg, "True" },
        { "-no-db",     ".doubleBuffer", XrmoptionNoArg, "False" },
        { 0, 0, 0, 0 }
};

XSCREENSAVER_MODULE ("NetwareSMP", netwaresmp)