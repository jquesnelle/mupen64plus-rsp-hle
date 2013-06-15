/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - mp3.c                                           *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2012 Bobby Smiles                                       *
 *   Copyright (C) 2009 Richard Goedeken                                   *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "hle.h"
#include "m64p_types.h"

#include "mp3.h"

static void apply_gain(u16 mem, unsigned count, s16 gain);
static s32 dot(const s16 *x, const s16 *y, unsigned count);
static void dewindowing(u32 t4, u32 t6, u32 outPtr);
static void InnerLoop(u32 inPtr, u32 outPtr, u32 t4, u32 t5, u32 t6);

// FIXME: use DMEM instead
static u8 mp3data[0x1000];

static const u16 DEWINDOW_LUT[0x420] =
{
    0x0000, 0xfff3, 0x005d, 0xff38, 0x037a, 0xf736, 0x0b37, 0xc00e,
    0x7fff, 0x3ff2, 0x0b37, 0x08ca, 0x037a, 0x00c8, 0x005d, 0x000d,
    0x0000, 0xfff3, 0x005d, 0xff38, 0x037a, 0xf736, 0x0b37, 0xc00e,
    0x7fff, 0x3ff2, 0x0b37, 0x08ca, 0x037a, 0x00c8, 0x005d, 0x000d,
    0x0000, 0xfff2, 0x005f, 0xff1d, 0x0369, 0xf697, 0x0a2a, 0xbce7,
    0x7feb, 0x3ccb, 0x0c2b, 0x082b, 0x0385, 0x00af, 0x005b, 0x000b,
    0x0000, 0xfff2, 0x005f, 0xff1d, 0x0369, 0xf697, 0x0a2a, 0xbce7,
    0x7feb, 0x3ccb, 0x0c2b, 0x082b, 0x0385, 0x00af, 0x005b, 0x000b,
    0x0000, 0xfff1, 0x0061, 0xff02, 0x0354, 0xf5f9, 0x0905, 0xb9c4,
    0x7fb0, 0x39a4, 0x0d08, 0x078c, 0x038c, 0x0098, 0x0058, 0x000a,
    0x0000, 0xfff1, 0x0061, 0xff02, 0x0354, 0xf5f9, 0x0905, 0xb9c4,
    0x7fb0, 0x39a4, 0x0d08, 0x078c, 0x038c, 0x0098, 0x0058, 0x000a,
    0x0000, 0xffef, 0x0062, 0xfee6, 0x033b, 0xf55c, 0x07c8, 0xb6a4,
    0x7f4d, 0x367e, 0x0dce, 0x06ee, 0x038f, 0x0080, 0x0056, 0x0009,
    0x0000, 0xffef, 0x0062, 0xfee6, 0x033b, 0xf55c, 0x07c8, 0xb6a4,
    0x7f4d, 0x367e, 0x0dce, 0x06ee, 0x038f, 0x0080, 0x0056, 0x0009,
    0x0000, 0xffee, 0x0063, 0xfeca, 0x031c, 0xf4c3, 0x0671, 0xb38c,
    0x7ec2, 0x335d, 0x0e7c, 0x0652, 0x038e, 0x006b, 0x0053, 0x0008,
    0x0000, 0xffee, 0x0063, 0xfeca, 0x031c, 0xf4c3, 0x0671, 0xb38c,
    0x7ec2, 0x335d, 0x0e7c, 0x0652, 0x038e, 0x006b, 0x0053, 0x0008,
    0x0000, 0xffec, 0x0064, 0xfeac, 0x02f7, 0xf42c, 0x0502, 0xb07c,
    0x7e12, 0x3041, 0x0f14, 0x05b7, 0x038a, 0x0056, 0x0050, 0x0007,
    0x0000, 0xffec, 0x0064, 0xfeac, 0x02f7, 0xf42c, 0x0502, 0xb07c,
    0x7e12, 0x3041, 0x0f14, 0x05b7, 0x038a, 0x0056, 0x0050, 0x0007,
    0x0000, 0xffeb, 0x0064, 0xfe8e, 0x02ce, 0xf399, 0x037a, 0xad75,
    0x7d3a, 0x2d2c, 0x0f97, 0x0520, 0x0382, 0x0043, 0x004d, 0x0007,
    0x0000, 0xffeb, 0x0064, 0xfe8e, 0x02ce, 0xf399, 0x037a, 0xad75,
    0x7d3a, 0x2d2c, 0x0f97, 0x0520, 0x0382, 0x0043, 0x004d, 0x0007,
    0xffff, 0xffe9, 0x0063, 0xfe6f, 0x029e, 0xf30b, 0x01d8, 0xaa7b,
    0x7c3d, 0x2a1f, 0x1004, 0x048b, 0x0377, 0x0030, 0x004a, 0x0006,
    0xffff, 0xffe9, 0x0063, 0xfe6f, 0x029e, 0xf30b, 0x01d8, 0xaa7b,
    0x7c3d, 0x2a1f, 0x1004, 0x048b, 0x0377, 0x0030, 0x004a, 0x0006,
    0xffff, 0xffe7, 0x0062, 0xfe4f, 0x0269, 0xf282, 0x001f, 0xa78d,
    0x7b1a, 0x271c, 0x105d, 0x03f9, 0x036a, 0x001f, 0x0046, 0x0006,
    0xffff, 0xffe7, 0x0062, 0xfe4f, 0x0269, 0xf282, 0x001f, 0xa78d,
    0x7b1a, 0x271c, 0x105d, 0x03f9, 0x036a, 0x001f, 0x0046, 0x0006,
    0xffff, 0xffe4, 0x0061, 0xfe2f, 0x022f, 0xf1ff, 0xfe4c, 0xa4af,
    0x79d3, 0x2425, 0x10a2, 0x036c, 0x0359, 0x0010, 0x0043, 0x0005,
    0xffff, 0xffe4, 0x0061, 0xfe2f, 0x022f, 0xf1ff, 0xfe4c, 0xa4af,
    0x79d3, 0x2425, 0x10a2, 0x036c, 0x0359, 0x0010, 0x0043, 0x0005,
    0xffff, 0xffe2, 0x005e, 0xfe10, 0x01ee, 0xf184, 0xfc61, 0xa1e1,
    0x7869, 0x2139, 0x10d3, 0x02e3, 0x0346, 0x0001, 0x0040, 0x0004,
    0xffff, 0xffe2, 0x005e, 0xfe10, 0x01ee, 0xf184, 0xfc61, 0xa1e1,
    0x7869, 0x2139, 0x10d3, 0x02e3, 0x0346, 0x0001, 0x0040, 0x0004,
    0xffff, 0xffe0, 0x005b, 0xfdf0, 0x01a8, 0xf111, 0xfa5f, 0x9f27,
    0x76db, 0x1e5c, 0x10f2, 0x025e, 0x0331, 0xfff3, 0x003d, 0x0004,
    0xffff, 0xffe0, 0x005b, 0xfdf0, 0x01a8, 0xf111, 0xfa5f, 0x9f27,
    0x76db, 0x1e5c, 0x10f2, 0x025e, 0x0331, 0xfff3, 0x003d, 0x0004,
    0xffff, 0xffde, 0x0057, 0xfdd0, 0x015b, 0xf0a7, 0xf845, 0x9c80,
    0x752c, 0x1b8e, 0x1100, 0x01de, 0x0319, 0xffe7, 0x003a, 0x0003,
    0xffff, 0xffde, 0x0057, 0xfdd0, 0x015b, 0xf0a7, 0xf845, 0x9c80,
    0x752c, 0x1b8e, 0x1100, 0x01de, 0x0319, 0xffe7, 0x003a, 0x0003,
    0xfffe, 0xffdb, 0x0053, 0xfdb0, 0x0108, 0xf046, 0xf613, 0x99ee,
    0x735c, 0x18d1, 0x10fd, 0x0163, 0x0300, 0xffdc, 0x0037, 0x0003,
    0xfffe, 0xffdb, 0x0053, 0xfdb0, 0x0108, 0xf046, 0xf613, 0x99ee,
    0x735c, 0x18d1, 0x10fd, 0x0163, 0x0300, 0xffdc, 0x0037, 0x0003,
    0xfffe, 0xffd8, 0x004d, 0xfd90, 0x00b0, 0xeff0, 0xf3cc, 0x9775,
    0x716c, 0x1624, 0x10ea, 0x00ee, 0x02e5, 0xffd2, 0x0033, 0x0003,
    0xfffe, 0xffd8, 0x004d, 0xfd90, 0x00b0, 0xeff0, 0xf3cc, 0x9775,
    0x716c, 0x1624, 0x10ea, 0x00ee, 0x02e5, 0xffd2, 0x0033, 0x0003,
    0xfffe, 0xffd6, 0x0047, 0xfd72, 0x0051, 0xefa6, 0xf16f, 0x9514,
    0x6f5e, 0x138a, 0x10c8, 0x007e, 0x02ca, 0xffc9, 0x0030, 0x0003,
    0xfffe, 0xffd6, 0x0047, 0xfd72, 0x0051, 0xefa6, 0xf16f, 0x9514,
    0x6f5e, 0x138a, 0x10c8, 0x007e, 0x02ca, 0xffc9, 0x0030, 0x0003,
    0xfffe, 0xffd3, 0x0040, 0xfd54, 0xffec, 0xef68, 0xeefc, 0x92cd,
    0x6d33, 0x1104, 0x1098, 0x0014, 0x02ac, 0xffc0, 0x002d, 0x0002,
    0xfffe, 0xffd3, 0x0040, 0xfd54, 0xffec, 0xef68, 0xeefc, 0x92cd,
    0x6d33, 0x1104, 0x1098, 0x0014, 0x02ac, 0xffc0, 0x002d, 0x0002,
    0x0030, 0xffc9, 0x02ca, 0x007e, 0x10c8, 0x138a, 0x6f5e, 0x9514,
    0xf16f, 0xefa6, 0x0051, 0xfd72, 0x0047, 0xffd6, 0xfffe, 0x0003,
    0x0030, 0xffc9, 0x02ca, 0x007e, 0x10c8, 0x138a, 0x6f5e, 0x9514,
    0xf16f, 0xefa6, 0x0051, 0xfd72, 0x0047, 0xffd6, 0xfffe, 0x0003,
    0x0033, 0xffd2, 0x02e5, 0x00ee, 0x10ea, 0x1624, 0x716c, 0x9775,
    0xf3cc, 0xeff0, 0x00b0, 0xfd90, 0x004d, 0xffd8, 0xfffe, 0x0003,
    0x0033, 0xffd2, 0x02e5, 0x00ee, 0x10ea, 0x1624, 0x716c, 0x9775,
    0xf3cc, 0xeff0, 0x00b0, 0xfd90, 0x004d, 0xffd8, 0xfffe, 0x0003,
    0x0037, 0xffdc, 0x0300, 0x0163, 0x10fd, 0x18d1, 0x735c, 0x99ee,
    0xf613, 0xf046, 0x0108, 0xfdb0, 0x0053, 0xffdb, 0xfffe, 0x0003,
    0x0037, 0xffdc, 0x0300, 0x0163, 0x10fd, 0x18d1, 0x735c, 0x99ee,
    0xf613, 0xf046, 0x0108, 0xfdb0, 0x0053, 0xffdb, 0xfffe, 0x0003,
    0x003a, 0xffe7, 0x0319, 0x01de, 0x1100, 0x1b8e, 0x752c, 0x9c80,
    0xf845, 0xf0a7, 0x015b, 0xfdd0, 0x0057, 0xffde, 0xffff, 0x0003,
    0x003a, 0xffe7, 0x0319, 0x01de, 0x1100, 0x1b8e, 0x752c, 0x9c80,
    0xf845, 0xf0a7, 0x015b, 0xfdd0, 0x0057, 0xffde, 0xffff, 0x0004,
    0x003d, 0xfff3, 0x0331, 0x025e, 0x10f2, 0x1e5c, 0x76db, 0x9f27,
    0xfa5f, 0xf111, 0x01a8, 0xfdf0, 0x005b, 0xffe0, 0xffff, 0x0004,
    0x003d, 0xfff3, 0x0331, 0x025e, 0x10f2, 0x1e5c, 0x76db, 0x9f27,
    0xfa5f, 0xf111, 0x01a8, 0xfdf0, 0x005b, 0xffe0, 0xffff, 0x0004,
    0x0040, 0x0001, 0x0346, 0x02e3, 0x10d3, 0x2139, 0x7869, 0xa1e1,
    0xfc61, 0xf184, 0x01ee, 0xfe10, 0x005e, 0xffe2, 0xffff, 0x0004,
    0x0040, 0x0001, 0x0346, 0x02e3, 0x10d3, 0x2139, 0x7869, 0xa1e1,
    0xfc61, 0xf184, 0x01ee, 0xfe10, 0x005e, 0xffe2, 0xffff, 0x0005,
    0x0043, 0x0010, 0x0359, 0x036c, 0x10a2, 0x2425, 0x79d3, 0xa4af,
    0xfe4c, 0xf1ff, 0x022f, 0xfe2f, 0x0061, 0xffe4, 0xffff, 0x0005,
    0x0043, 0x0010, 0x0359, 0x036c, 0x10a2, 0x2425, 0x79d3, 0xa4af,
    0xfe4c, 0xf1ff, 0x022f, 0xfe2f, 0x0061, 0xffe4, 0xffff, 0x0006,
    0x0046, 0x001f, 0x036a, 0x03f9, 0x105d, 0x271c, 0x7b1a, 0xa78d,
    0x001f, 0xf282, 0x0269, 0xfe4f, 0x0062, 0xffe7, 0xffff, 0x0006,
    0x0046, 0x001f, 0x036a, 0x03f9, 0x105d, 0x271c, 0x7b1a, 0xa78d,
    0x001f, 0xf282, 0x0269, 0xfe4f, 0x0062, 0xffe7, 0xffff, 0x0006,
    0x004a, 0x0030, 0x0377, 0x048b, 0x1004, 0x2a1f, 0x7c3d, 0xaa7b,
    0x01d8, 0xf30b, 0x029e, 0xfe6f, 0x0063, 0xffe9, 0xffff, 0x0006,
    0x004a, 0x0030, 0x0377, 0x048b, 0x1004, 0x2a1f, 0x7c3d, 0xaa7b,
    0x01d8, 0xf30b, 0x029e, 0xfe6f, 0x0063, 0xffe9, 0xffff, 0x0007,
    0x004d, 0x0043, 0x0382, 0x0520, 0x0f97, 0x2d2c, 0x7d3a, 0xad75,
    0x037a, 0xf399, 0x02ce, 0xfe8e, 0x0064, 0xffeb, 0x0000, 0x0007,
    0x004d, 0x0043, 0x0382, 0x0520, 0x0f97, 0x2d2c, 0x7d3a, 0xad75,
    0x037a, 0xf399, 0x02ce, 0xfe8e, 0x0064, 0xffeb, 0x0000, 0x0007,
    0x0050, 0x0056, 0x038a, 0x05b7, 0x0f14, 0x3041, 0x7e12, 0xb07c,
    0x0502, 0xf42c, 0x02f7, 0xfeac, 0x0064, 0xffec, 0x0000, 0x0007,
    0x0050, 0x0056, 0x038a, 0x05b7, 0x0f14, 0x3041, 0x7e12, 0xb07c,
    0x0502, 0xf42c, 0x02f7, 0xfeac, 0x0064, 0xffec, 0x0000, 0x0008,
    0x0053, 0x006b, 0x038e, 0x0652, 0x0e7c, 0x335d, 0x7ec2, 0xb38c,
    0x0671, 0xf4c3, 0x031c, 0xfeca, 0x0063, 0xffee, 0x0000, 0x0008,
    0x0053, 0x006b, 0x038e, 0x0652, 0x0e7c, 0x335d, 0x7ec2, 0xb38c,
    0x0671, 0xf4c3, 0x031c, 0xfeca, 0x0063, 0xffee, 0x0000, 0x0009,
    0x0056, 0x0080, 0x038f, 0x06ee, 0x0dce, 0x367e, 0x7f4d, 0xb6a4,
    0x07c8, 0xf55c, 0x033b, 0xfee6, 0x0062, 0xffef, 0x0000, 0x0009,
    0x0056, 0x0080, 0x038f, 0x06ee, 0x0dce, 0x367e, 0x7f4d, 0xb6a4,
    0x07c8, 0xf55c, 0x033b, 0xfee6, 0x0062, 0xffef, 0x0000, 0x000a,
    0x0058, 0x0098, 0x038c, 0x078c, 0x0d08, 0x39a4, 0x7fb0, 0xb9c4,
    0x0905, 0xf5f9, 0x0354, 0xff02, 0x0061, 0xfff1, 0x0000, 0x000a,
    0x0058, 0x0098, 0x038c, 0x078c, 0x0d08, 0x39a4, 0x7fb0, 0xb9c4,
    0x0905, 0xf5f9, 0x0354, 0xff02, 0x0061, 0xfff1, 0x0000, 0x000b,
    0x005b, 0x00af, 0x0385, 0x082b, 0x0c2b, 0x3ccb, 0x7feb, 0xbce7,
    0x0a2a, 0xf697, 0x0369, 0xff1d, 0x005f, 0xfff2, 0x0000, 0x000b,
    0x005b, 0x00af, 0x0385, 0x082b, 0x0c2b, 0x3ccb, 0x7feb, 0xbce7,
    0x0a2a, 0xf697, 0x0369, 0xff1d, 0x005f, 0xfff2, 0x0000, 0x000d,
    0x005d, 0x00c8, 0x037a, 0x08ca, 0x0b37, 0x3ff2, 0x7fff, 0xc00e,
    0x0b37, 0xf736, 0x037a, 0xff38, 0x005d, 0xfff3, 0x0000, 0x000d,
    0x005d, 0x00c8, 0x037a, 0x08ca, 0x0b37, 0x3ff2, 0x7fff, 0xc00e,
    0x0b37, 0xf736, 0x037a, 0xff38, 0x005d, 0xfff3, 0x0000, 0x0000
};

static s16 clamp_s16(s32 x)
{
    if (x > 32767) { x = 32767; } else if (x < -32768) { x = -32768; }
    return x;
}

static s32 dmul_round(s16 x, s16 y)
{
    return ((s32)x * (s32)y + 0x4000) >> 15;
}

static void smul(s16 *x, s16 gain)
{
    *x = clamp_s16((s32)(*x) * (s32)gain);
}

static s16* sample_at(u16 mem)
{
    assert((mem & 0x1) == 0);
    assert((mem & ~0xfff) == 0);

    return (s16*)(mp3data+(mem^S16));
}

static void swap(u32 *a, u32 *b)
{
    u32 tmp = *b;
    *b = *a;
    *a = tmp;
}

/* global function */
void mp3_decode(u32 address, unsigned char index)
{
    // Initialization Code
    u32 readPtr; // s5
    u32 writePtr; // s6
    //u32 Count = 0x0480; // s4
    u32 inPtr, outPtr;
    int cnt, cnt2;

    u32 t6 = 0x08A0; // I think these are temporary storage buffers
    u32 t5 = 0x0AC0;
    u32 t4 = index;

    writePtr = address;
    readPtr  = writePtr;
    memcpy (mp3data+0xCE8, rsp.RDRAM+readPtr, 8); // Just do that for efficiency... may remove and use directly later anyway
    readPtr += 8; // This must be a header byte or whatnot

    for (cnt = 0; cnt < 0x480; cnt += 0x180)
    {
        memcpy (mp3data+0xCF0, rsp.RDRAM+readPtr, 0x180); // DMA: 0xCF0 <- RDRAM[s5] : 0x180
        inPtr  = 0xCF0; // s7
        outPtr = 0xE70; // s3
// --------------- Inner Loop Start --------------------
        for (cnt2 = 0; cnt2 < 0x180; cnt2 += 0x40)
        {
            t6 &= 0xFFE0;
            t5 &= 0xFFE0;
            t6 |= (t4 << 1);
            t5 |= (t4 << 1);
            InnerLoop(inPtr, outPtr, t4, t5, t6);
            t4 = (t4 - 1) & 0x0f;
            swap(&t5, &t6);
            outPtr += 0x40;
            inPtr += 0x40;
        }
// --------------- Inner Loop End --------------------
        memcpy (rsp.RDRAM+writePtr, mp3data+0xe70, 0x180);
        writePtr += 0x180;
        readPtr  += 0x180;
    }
}

/* local functions */
static void MP3AB0(s32 *v)
{
    // Part 2 - 100% Accurate
    const u16 LUT2[8] = { 0xFEC4, 0xF4FA, 0xC5E4, 0xE1C4, 
                          0x1916, 0x4A50, 0xA268, 0x78AE };
    const u16 LUT3[4] = { 0xFB14, 0xD4DC, 0x31F2, 0x8E3A };
    int i;

    for (i = 0; i < 8; i++)
    {
        v[16+i] = v[0+i] + v[8+i];
        v[24+i] = ((v[0+i] - v[8+i]) * LUT2[i]) >> 0x10;
    }

    // Part 3: 4-wide butterflies

    for (i=0; i < 4; i++)
    {
        v[0+i]  = v[16+i] + v[20+i];
        v[4+i]  = ((v[16+i] - v[20+i]) * LUT3[i]) >> 0x10;

        v[8+i]  = v[24+i] + v[28+i];
        v[12+i] = ((v[24+i] - v[28+i]) * LUT3[i]) >> 0x10;
    }
                
    // Part 4: 2-wide butterflies - 100% Accurate

    for (i = 0; i < 16; i+=4)
    {
        v[16+i] = v[0+i] + v[2+i];
        v[18+i] = ((v[0+i] - v[2+i]) * 0xEC84) >> 0x10;

        v[17+i] = v[1+i] + v[3+i];
        v[19+i] = ((v[1+i] - v[3+i]) * 0x61F8) >> 0x10;
    }
}

static void load_v(s32 *v, u16 inPtr)
{
    v[0] = *sample_at(inPtr+0x00); v[31] = *sample_at(inPtr+0x3E); v[0] += v[31];
    v[1] = *sample_at(inPtr+0x02); v[30] = *sample_at(inPtr+0x3C); v[1] += v[30];
    v[2] = *sample_at(inPtr+0x06); v[28] = *sample_at(inPtr+0x38); v[2] += v[28];
    v[3] = *sample_at(inPtr+0x04); v[29] = *sample_at(inPtr+0x3A); v[3] += v[29];

    v[4] = *sample_at(inPtr+0x0E); v[24] = *sample_at(inPtr+0x30); v[4] += v[24];
    v[5] = *sample_at(inPtr+0x0C); v[25] = *sample_at(inPtr+0x32); v[5] += v[25];
    v[6] = *sample_at(inPtr+0x08); v[27] = *sample_at(inPtr+0x36); v[6] += v[27];
    v[7] = *sample_at(inPtr+0x0A); v[26] = *sample_at(inPtr+0x34); v[7] += v[26];

    v[8] = *sample_at(inPtr+0x1E); v[16] = *sample_at(inPtr+0x20); v[8] += v[16];
    v[9] = *sample_at(inPtr+0x1C); v[17] = *sample_at(inPtr+0x22); v[9] += v[17];
    v[10]= *sample_at(inPtr+0x18); v[19] = *sample_at(inPtr+0x26); v[10]+= v[19];
    v[11]= *sample_at(inPtr+0x1A); v[18] = *sample_at(inPtr+0x24); v[11]+= v[18];

    v[12]= *sample_at(inPtr+0x10); v[23] = *sample_at(inPtr+0x2E); v[12]+= v[23];
    v[13]= *sample_at(inPtr+0x12); v[22] = *sample_at(inPtr+0x2C); v[13]+= v[22];
    v[14]= *sample_at(inPtr+0x16); v[20] = *sample_at(inPtr+0x28); v[14]+= v[20];
    v[15]= *sample_at(inPtr+0x14); v[21] = *sample_at(inPtr+0x2A); v[15]+= v[21];

}

static void InnerLoop(u32 inPtr, u32 outPtr, u32 t4, u32 t5, u32 t6)
{
    s32 v[32];
    int i;


    load_v(v, inPtr);

    MP3AB0(v);

    u32 t0 = t6 + 0x100;
    u32 t1 = t6 + 0x200;
    u32 t2 = t5 + 0x100;
    u32 t3 = t5 + 0x200;
    
    //
    v[11] = ((v[16] - v[17]) * 0xB504) >> 0x10;
    v[16] = -(v[16] + v[17]);
    *(s16 *)(mp3data+((t6 + 0x0))) = (s16)v[11];
    *(s16 *)(mp3data+((t5 + 0x0))) = (s16)(-v[11]);
    *(s16 *)(mp3data+((t3 + 0x0))) = (s16)v[16];

    v[2] = -(v[18] + v[19]);
    v[3] = (((v[18] - v[19]) * 0x16A09) >> 0x10) + v[2];
    *(s16 *)(mp3data+((t2 + 0x0))) = (s16)v[2];
    *(s16 *)(mp3data+((t0 + 0x0))) = (s16)v[3];

    v[4] = -v[20] -v[21];
    v[5] = (((v[20] - v[21]) * 0x16A09) >> 0x10) - v[4];
    *(s16 *)(mp3data+((t3 - 0x80))) = (s16)v[4];
    
    v[6] = (v[22] + v[23]) << 1;
    v[7] = (((v[22] - v[23]) * 0x2D413) >> 0x10) - v[5];
    *(s16 *)(mp3data+((t1 - 0x80))) = (s16)v[7];

    v[5] = v[5] - v[6];
    *(s16 *)(mp3data+((t0 - 0x80))) = (s16)v[5];

    v[4] = -(v[4] + v[6]);
    *(s16 *)(mp3data+((t2 - 0x80))) = (s16)v[4];



    v[8] = v[24] + v[25];
    v[9] = ((v[24] - v[25]) * 0x16A09) >> 0x10;
    v[2] = v[8] + v[9];

    v[10] = (v[26] + v[27]) << 1;
    v[11] = ((v[26] - v[27]) * 0x2D413) >> 0x10;
    v[12] = (v[28] + v[29]) << 1;
    v[13] = ((v[28] - v[29]) * 0x2D413) >> 0x10;

    v[3] = v[8] + v[10];

    v[14] = v[3] - ((v[30] + v[31]) << 2);

    v[13] = (v[13] - v[2]) + v[12];
    v[15] = (((v[30] - v[31]) * 0x5A827) >> 0x10) - (v[11] + v[2]);
    v[17] = v[13] - v[10];
    v[9] = v[9] + v[14];
    v[11] = v[11] - v[13];
    v[12] = v[8] - v[12];
    v[8] = -v[8];
    v[10] = -v[10] -v[12];
    
    *(s16 *)(mp3data+((t6 + 0x40))) = (s16)v[9];
    *(s16 *)(mp3data+((t0 - 0x40))) = (s16)v[17];
    *(s16 *)(mp3data+((t0 + 0x40))) = (s16)v[11];
    *(s16 *)(mp3data+((t1 - 0x40))) = (s16)v[15];
    *(s16 *)(mp3data+((t2 + 0x40))) = (s16)v[12];
    *(s16 *)(mp3data+((t3 - 0x40))) = (s16)v[8];
    *(s16 *)(mp3data+((t5 + 0x40))) = (s16)v[14];
    *(s16 *)(mp3data+((t2 - 0x40))) = (s16)v[10];

    load_v(v, inPtr);

    //0, 1, 3, 2, 7, 6, 4, 5, 7, 6, 4, 5, 0, 1, 3, 2
    const u16 LUT6[16] = { 0xFFB2, 0xFD3A, 0xF10A, 0xF854,
                           0xBDAE, 0xCDA0, 0xE76C, 0xDB94,
                           0x1920, 0x4B20, 0xAC7C, 0x7C68,
                           0xABEC, 0x9880, 0xDAE8, 0x839C };
    for (i = 0; i < 16; i++)
    {
        v[0+i] = (v[0+i] * LUT6[i]) >> 0x10;
    }
    v[0] = v[0] + v[0]; v[1] = v[1] + v[1];
    v[2] = v[2] + v[2]; v[3] = v[3] + v[3]; v[4] = v[4] + v[4];
    v[5] = v[5] + v[5]; v[6] = v[6] + v[6]; v[7] = v[7] + v[7];
    v[12] = v[12] + v[12]; v[13] = v[13] + v[13]; v[15] = v[15] + v[15];
    
    MP3AB0(v);

    v[0] = ( v[17] + v[16] ) >> 1;
    v[1] = ((v[17] * (int)((short)0xA57E * 2)) + (v[16] * 0xB504)) >> 0x10;
    v[2] = -v[18] -v[19];
    v[3] = ((v[18] - v[19]) * 0x16A09) >> 0x10;
    v[4] = v[20] + v[21] + v[0];
    v[5] = (((v[20] - v[21]) * 0x16A09) >> 0x10) + v[1];
    v[6] = (((v[22] + v[23]) << 1) + v[0]) - v[2];
    v[7] = (((v[22] - v[23]) * 0x2D413) >> 0x10) + v[0] + v[1] + v[3];
    *(s16 *)(mp3data+((t3 - 0x20))) = (s16)-v[0];
    v[8] = v[24] + v[25];
    v[9] = ((v[24] - v[25]) * 0x16A09) >> 0x10;
    v[10] = ((v[26] + v[27]) << 1) + v[8];
    v[11] = (((v[26] - v[27]) * 0x2D413) >> 0x10) + v[8] + v[9];
    v[12] = v[4] - ((v[28] + v[29]) << 1);
    *(s16 *)(mp3data+((t2 + 0x20))) = (s16)v[12];
    v[13] = (((v[28] - v[29]) * 0x2D413) >> 0x10) - v[12] - v[5];
    v[14] = v[30] + v[31];
    v[14] = v[14] + v[14];
    v[14] = v[14] + v[14];
    v[14] = v[6] - v[14];
    v[15] = (((v[30] - v[31]) * 0x5A827) >> 0x10) - v[7];
    *(s16 *)(mp3data+((t5 + 0x20))) = (s16)v[14];
    v[14] = v[14] + v[1];
    *(s16 *)(mp3data+((t6 + 0x20))) = (s16)v[14];
    *(s16 *)(mp3data+((t1 - 0x20))) = (s16)v[15];
    v[9] = v[9] + v[10];
    v[1] = v[1] + v[6];
    v[6] = v[10] - v[6];
    *(s16 *)(mp3data+((t5 + 0x60))) = (s16)v[6];
    v[1] = v[9] - v[1];
    *(s16 *)(mp3data+((t6 + 0x60))) = (s16)v[1];
    v[10] = v[4] - (v[10] + v[2]);
    *(s16 *)(mp3data+((t2 - 0x60))) = (s16)v[10];
    v[12] = v[2] - v[12];
    *(s16 *)(mp3data+((t2 - 0x20))) = (s16)v[12];
    v[5] = v[4] + v[5];
    v[4] = v[8] - v[4];
    *(s16 *)(mp3data+((t2 + 0x60))) = (s16)v[4];
    v[0] = v[0] - v[8];
    *(s16 *)(mp3data+((t3 - 0x60))) = (s16)v[0];
    v[7] = v[7] - v[11];
    *(s16 *)(mp3data+((t1 - 0x60))) = (s16)v[7];
    v[11] = v[11] - v[3] - v[5];
    *(s16 *)(mp3data+((t0 + 0x60))) = (s16)v[11];
    v[3] = v[3] - v[13];
    *(s16 *)(mp3data+((t0 + 0x20))) = (s16)v[3];
    v[13] = v[13] + v[2];
    *(s16 *)(mp3data+((t0 - 0x20))) = (s16)v[13];
    v[2] = (v[5] - v[2]) - v[9];
    *(s16 *)(mp3data+((t0 - 0x60))) = (s16)v[2];

    dewindowing(t4, t6, outPtr);
}

static void dewindowing(u32 t4, u32 t6, u32 outPtr)
{
    u32 offset = 0x10-t4;
    u32 addptr = t6 & 0xFFE0;
    s32 v2=0, v4=0, v6=0, v8=0;

    int x, i;
    for (x = 0; x < 8; x++)
    {
        v2 = dot((s16*)(mp3data + addptr + 0x00), (s16*)(DEWINDOW_LUT + offset + 0x00), 16);
        v6 = dot((s16*)(mp3data + addptr + 0x20), (s16*)(DEWINDOW_LUT + offset + 0x20), 16);

        // clamp ?
        *sample_at(outPtr    ) = v2;
        *sample_at(outPtr + 2) = v6;
        
        outPtr+=4;
        addptr += 0x40;
        offset += 0x40;
    }

    offset = 0x10-t4 + 8*0x40;
    v2 = v4 = 0;
    for (i = 0; i < 4; i++)
    {
        v2 += dmul_round(*(s16*)(mp3data+addptr+0x00), DEWINDOW_LUT[offset+0x00]);
        v2 += dmul_round(*(s16*)(mp3data+addptr+0x10), DEWINDOW_LUT[offset+0x08]);
        addptr+=2; offset++;
        v4 += dmul_round(*(s16*)(mp3data+addptr+0x00), DEWINDOW_LUT[offset+0x00]);
        v4 += dmul_round(*(s16*)(mp3data+addptr+0x10), DEWINDOW_LUT[offset+0x08]);
        addptr+=2; offset++;
    }
    s32 mult6 = *(s32 *)(mp3data+0xCE8);
    s32 mult4 = *(s32 *)(mp3data+0xCEC);
    if (t4 & 0x1)
    {
        v2 = (v2 * *(u32 *)(mp3data+0xCE8)) >> 0x10;
        *sample_at(outPtr) = v2;
    }
    else
    {
        v4 = (v4 * *(u32 *)(mp3data+0xCE8)) >> 0x10;
        *sample_at(outPtr) = v4;
        mult4 = *(u32 *)(mp3data+0xCE8);
    }
    addptr -= 0x50;

    for (x = 0; x < 8; x++)
    {
        v2 = v4 = v6 = v8 = 0;

        offset = (0x22F-t4 + x*0x40);
    
        for (i = 0; i < 4; i++)
        {
            v2 += dmul_round(*(s16*)(mp3data+addptr+0x20), DEWINDOW_LUT[offset+0x00]);
            v2 -= dmul_round(*(s16*)(mp3data+addptr+0x22), DEWINDOW_LUT[offset+0x01]);
            v4 += dmul_round(*(s16*)(mp3data+addptr+0x30), DEWINDOW_LUT[offset+0x08]);
            v4 -= dmul_round(*(s16*)(mp3data+addptr+0x32), DEWINDOW_LUT[offset+0x09]);
            v6 += dmul_round(*(s16*)(mp3data+addptr+0x00), DEWINDOW_LUT[offset+0x20]);
            v6 -= dmul_round(*(s16*)(mp3data+addptr+0x02), DEWINDOW_LUT[offset+0x21]);
            v8 += dmul_round(*(s16*)(mp3data+addptr+0x10), DEWINDOW_LUT[offset+0x28]);
            v8 -= dmul_round(*(s16*)(mp3data+addptr+0x12), DEWINDOW_LUT[offset+0x29]);
            addptr+=4; offset+=2;
        }
        s32 v0  = v2 + v4;
        s32 v18 = v6 + v8;
        //Clamp(v0);
        //Clamp(v18);
        // clamp???
        *sample_at(outPtr + 2) = v0;
        *sample_at(outPtr + 4) = v18;
        
        outPtr+=4;
        addptr -= 0x50;
    }

    apply_gain(outPtr - 0x40, 16, (s16)(mult6 >> 16));
    apply_gain(outPtr - 0x1e, 16, (s16)(mult4 >> 16));
}

static void apply_gain(u16 mem, unsigned count, s16 gain)
{
    while (count != 0)
    {
        smul(sample_at(mem), gain);
        mem += 2;
        --count;
    }
}

static s32 dot(const s16 *x, const s16 *y, unsigned count)
{
    s32 accu = 0;

    while(count != 0)
    {
        accu += dmul_round(*(x++), *(y++));
        --count;
    }

    return accu;
}
