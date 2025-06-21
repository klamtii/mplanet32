/*
 * Copyright (C) 2010 Cameron Zemek ( grom@zeminvaders.net)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdint.h>
#include "hqx.h"

/*uint32_t   RGBtoYUV[16777216];*/
uint32_t BtoYUV[256];
uint32_t GtoYUV[256];
uint32_t RtoYUV[256];
uint32_t   YUV1, YUV2;

HQX_API void HQX_CALLCONV hqxInit(void)
{
    /* Initalize RGB to YUV lookup table */
   uint32_t c, r, g, b, y, u, v;
/*    for (c = 0; c < 16777216; c++) {
        r = (c & 0xFF0000) >> 16;
        g = (c & 0x00FF00) >> 8;
        b = c & 0x0000FF;
        y = (299*r + 587*g + 114*b)/1000;
        u = (-169*r - 331*g + 500*b)/1000 + 128;
        v = (500*r - 419*g - 81*b)/1000 + 128;
        RGBtoYUV[c] = (y << 16) + (u << 8) + v;
    }*/

#define TRUNC_TO_8BIT(a,b,c) {(a)&=0xFF;(b)&=0xFF;(c)&=0xFF;}

 for(c=0;c<256;c++)
 {
  y = (114*c)/1000;
  u = (500*c)/1000;
  v = (-81*c)/1000+64;
  TRUNC_TO_8BIT(y,u,v);
  BtoYUV[c] = (y << 16) + (u << 8) + v;                   
  y = (587*c)/1000;
  u = (-331*c)/1000 + 64;
  v = (-419*c)/1000 + 64;
  TRUNC_TO_8BIT(y,u,v);
  GtoYUV[c] = (y << 16) + (u << 8) + v;                   
  y = (299*c)/1000;
  u = (-169*c)/1000 + 64;
  v = (500*c)/1000;
  TRUNC_TO_8BIT(y,u,v);
  RtoYUV[c] = (y << 16) + (u << 8) + v;                   
 }
}
