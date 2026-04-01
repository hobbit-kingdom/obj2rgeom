#ifndef __XBMP_H__
#define __XBMP_H__

/* not sure how these two are different indeed */
/* TODO rename to BGRA (due to byte order in file) ? */
#define XBMP_FORMAT_RGBX8888   0x00000003
#define XBMP_FORMAT_RGBA8888   0x00000004

#define XBMP_FORMAT_ARGB4444   0x00000007
#define XBMP_FORMAT_ARGB1555   0x0000000A
#define XBMP_FORMAT_RGB555     0x0000000B
#define XBMP_FORMAT_RGB565     0x0000000C

#define XBMP_FORMAT_DXT1       0x00000049
#define XBMP_FORMAT_LUMINANCE8 0x0000004B /* or ALPHA8 ? Nevertheless, it's just a 8-bit b/w image */
#define XBMP_FORMAT_DXT3       0x0000004E

struct XBmp
{
	int image_data_size;
	int unknown1; // = 0
	int width;
	int height;
	int width2;   // or pitch I guess
	int unknown2; // = 3;
	int mipmaps;  // count of mip levels minus 1, i.e. if file contains three images (64x64, 32x32, 16x16), this value will be 2
	int format;   

	char *image_data; // if mipmaps != 0 this is pointer to array of XBmpMipmap, otherwise pointer to image pixels
};

struct XBmpMipmap
{
	int   offset;
	short width;
	short height;	
};

int XBmp_Load(struct XBmp *xbmp, const char *filename);
int XBmp_Save(struct XBmp *xbmp, const char *filename);
void XBmp_Unload(struct XBmp *xbmp);

#endif
