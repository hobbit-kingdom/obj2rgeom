#ifndef __DXT_DECOMPRESS_H__
#define __DXT_DECOMPRESS_H__

void decompress_dxt1(__int64 *in, DWORD *out, int w, int h);
DWORD image_size_dxt1(int w, int h);

void decompress_dxt3(__int64 *in, DWORD *out, int w, int h);
DWORD image_size_dxt3(int w, int h);

#endif

