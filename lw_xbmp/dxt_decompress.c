#include <windows.h>
#include "dxt_decompress.h"

void decompress_dxt1(__int64 *in, DWORD *out, int w, int h)
{
	int block_w = w/4;
	int block_h = h/4;
	
	int x, y, i, j;
	
	for(y = 0; y < block_h; y++) {
		for(x = 0; x < block_w; x++) {
			__int64 block = in[y*block_w+x];
			unsigned short c0 = block & 0xFFFF;
			unsigned short c1 = (block >> 16) & 0xFFFF;
			unsigned long index = block >> 32;
			
			int clr[4];
			int r0, g0, b0;
			int r1, g1, b1;
			
			b0 = (c0 & 0x1F) << 3;
			g0 = ((c0 >> 5) & 0x3F) << 2;
			r0 = (c0 >> 11) << 3;
				
			b1 = (c1 & 0x1F) << 3;
			g1 = ((c1 >> 5) & 0x3F) << 2;
			r1 = (c1 >> 11) << 3;
			
			clr[0]  = b0;
			clr[0] |= g0 << 8;
			clr[0] |= r0 << 16;
			
			clr[1]  = b1;
			clr[1] |= g1 << 8;
			clr[1] |= r1 << 16;
			
			if(c0 > c1) {
				clr[2]  = (b0+b0+b1)/3;
				clr[2] |= ((g0+g0+g1)/3)<<8;
				clr[2] |= ((r0+r0+r1)/3)<<16;
				
				clr[3]  = (b0+b1+b1)/3;
				clr[3] |= ((g0+g1+g1)/3)<<8;
				clr[3] |= ((r0+r1+r1)/3)<<16;
			} else {
				clr[2]  = (b0+b1)/2;
				clr[2] |= ((g0+g1)/2)<<8;
				clr[2] |= ((r0+r1)/2)<<16;
				
				clr[3] = 0;			
			}
			
			for(i = 0; i < 4; i++) {
				for(j = 0; j < 4; j++) {
					out[(y*4+i)*w+(x*4+j)] = clr[index&3];
					index >>= 2;
				}
			}
		}
	}
}

DWORD image_size_dxt1(int w, int h)
{
	return max(w/4, 1) * max(h/4, 1) * 8;
}

void decompress_dxt3(__int64 *in, DWORD *out, int w, int h)
{
	int block_w = w/4;
	int block_h = h/4;
	
	int x, y, i, j;
	
	for(y = 0; y < block_h; y++) {
		for(x = 0; x < block_w; x++) {
			__int64 alpha = in[(y*block_w+x)*2];
			__int64 block = in[(y*block_w+x)*2+1];
			unsigned short c0 = block & 0xFFFF;
			unsigned short c1 = (block >> 16) & 0xFFFF;
			unsigned long index = block >> 32;
			
			int clr[4];
			int r0, g0, b0;
			int r1, g1, b1;
			
			b0 = (c0 & 0x1F) << 3;
			g0 = ((c0 >> 5) & 0x3F) << 2;
			r0 = (c0 >> 11) << 3;
				
			b1 = (c1 & 0x1F) << 3;
			g1 = ((c1 >> 5) & 0x3F) << 2;
			r1 = (c1 >> 11) << 3;
			
			clr[0]  = b0;
			clr[0] |= g0 << 8;
			clr[0] |= r0 << 16;
			
			clr[1]  = b1;
			clr[1] |= g1 << 8;
			clr[1] |= r1 << 16;
			
			if(c0 > c1) {
				clr[2]  = (b0+b0+b1)/3;
				clr[2] |= ((g0+g0+g1)/3)<<8;
				clr[2] |= ((r0+r0+r1)/3)<<16;
				
				clr[3]  = (b0+b1+b1)/3;
				clr[3] |= ((g0+g1+g1)/3)<<8;
				clr[3] |= ((r0+r1+r1)/3)<<16;
			} else {
				clr[2]  = (b0+b1)/2;
				clr[2] |= ((g0+g1)/2)<<8;
				clr[2] |= ((r0+r1)/2)<<16;
				
				clr[3] = 0;			
			}
			
			for(i = 0; i < 4; i++) {
				for(j = 0; j < 4; j++) {
					out[(y*4+i)*w+(x*4+j)] = clr[index&3] | ((alpha & 0xF) << 28);
					
					index >>= 2;
					alpha >>= 4;
				}
			}
		}
	}
}

DWORD image_size_dxt3(int w, int h)
{
	return max(w/4, 1) * max(h/4, 1) * 16;
}
