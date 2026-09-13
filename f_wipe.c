//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// DESCRIPTION:
//      FizzleFade screen transition.
//

#include <string.h>

#include "z_zone.h"
#include "i_video.h"
#include "v_video.h"
#include "m_random.h"

#include "doomtype.h"

#include "f_wipe.h"

static boolean    go = 0;

static byte*    wipe_scr_start;
static byte*    wipe_scr_end;
static byte*    wipe_scr;

static unsigned int rndbits_y;
static unsigned int rndmask;
static unsigned int lastrndval;
static int frame_count = 0;
static int pixels_drawn_total = 0;

// XOR masks for the pseudo-random number sequence starting with n=17 bits
static const uint32_t rndmasks[] = {
    0x00012000, // 17
    0x00020400, // 18
    0x00040023, // 19
    0x00090000, // 20
    0x00140000, // 21
    0x00300000, // 22
    0x00420000, // 23
    0x00e10000, // 24
    0x01200000, // 25
};

static int log2_ceil(uint32_t x)
{
    int n = 0;
    uint32_t v = 1;
    while (v < x)
    {
        n++;
        v <<= 1;
    }
    return n;
}

int
wipe_initFizzle
( int     width,
  int     height,
  int     ticks )
{
    int rndbits_x = log2_ceil(width);
    rndbits_y = log2_ceil(height);

    int rndbits = rndbits_x + rndbits_y;
    if (rndbits < 17) rndbits = 17;
    else if (rndbits > 25) rndbits = 25;

    rndmask = rndmasks[rndbits - 17];
    memcpy(wipe_scr, wipe_scr_start, width * height);
    
    frame_count = 0;
    pixels_drawn_total = 0;
    lastrndval = 0;
    return 0;
}

int
wipe_doFizzle
( int     width,
  int     height,
  int     ticks )
{
    if (ticks <= 0)
    {
        return 0;
    }

    frame_count += ticks;
    int total_pixels = width * height;

    int target_total = (frame_count >= 38) ? total_pixels : (total_pixels * frame_count) / 38;
    int pixels_to_draw_now = target_total - pixels_drawn_total;

    unsigned int rndval = lastrndval;

    for (int p = 0; p < pixels_to_draw_now; p++)
    {
        unsigned int x = rndval >> rndbits_y;
        unsigned int y = rndval & ((1 << rndbits_y) - 1);

        rndval = (rndval >> 1) ^ (rndval & 1 ? 0 : rndmask);

        if (x < width && y < height)
        {
            wipe_scr[y * width + x] = wipe_scr_end[y * width + x];
        }
        else
        {
            if (rndval == 0) break;
            p--;
            continue;
        }

        if (rndval == 0)
        {
            lastrndval = rndval;
            return 1; // Done early if sequence completes
        }
    }

    lastrndval = rndval;
    pixels_drawn_total = target_total;

    return (frame_count >= 38 || pixels_drawn_total >= total_pixels);
}

int
wipe_exitFizzle
( int     width,
  int     height,
  int     ticks )
{
    if (wipe_scr_start)
    {
        Z_Free(wipe_scr_start);
        wipe_scr_start = NULL;
    }
    if (wipe_scr_end)
    {
        Z_Free(wipe_scr_end);
        wipe_scr_end = NULL;
    }
    return 0;
}

int
wipe_StartScreen
( int     x,
  int     y,
  int     width,
  int     height )
{
    wipe_scr_start = Z_Malloc(width * height, PU_STATIC, NULL);
    I_ReadScreen(wipe_scr_start);
    return 0;
}

int
wipe_EndScreen
( int     x,
  int     y,
  int     width,
  int     height )
{
    wipe_scr_end = Z_Malloc(width * height, PU_STATIC, NULL);
    I_ReadScreen(wipe_scr_end);
    V_DrawBlock(x, y, width, height, wipe_scr_start);
    return 0;
}

int
wipe_ScreenWipe
( int     wipeno,
  int     x,
  int     y,
  int     width,
  int     height,
  int     ticks )
{
    int rc;
    static int (*wipes[])(int, int, int) =
    {
        wipe_initFizzle, wipe_doFizzle, wipe_exitFizzle,
        wipe_initFizzle, wipe_doFizzle, wipe_exitFizzle
    };

    // initial stuff
    if (!go)
    {
        go = 1;
        wipe_scr = I_VideoBuffer;
        (*wipes[0])(width, height, ticks);
    }

    // do a piece of wipe-in
    V_MarkRect(0, 0, width, height);
    rc = (*wipes[1])(width, height, ticks);

    // final stuff
    if (rc)
    {
        go = 0;
        (*wipes[2])(width, height, ticks);
    }

    return !go;
}
