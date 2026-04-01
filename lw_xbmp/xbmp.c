#include <stdio.h>
#include <stdlib.h>

#include "xbmp.h"

int XBmp_Load(struct XBmp *xbmp, const char *filename)
{
	FILE *fp;
	
	fp = fopen(filename, "rb");
	if(!fp)
		return 1;
		
	fread(&xbmp->image_data_size, 1, sizeof(int), fp);
	fread(&xbmp->unknown1, 1, sizeof(int), fp);
	fread(&xbmp->width, 1, sizeof(int), fp);
	fread(&xbmp->height, 1, sizeof(int), fp);
	fread(&xbmp->width2, 1, sizeof(int), fp);
	fread(&xbmp->unknown2, 1, sizeof(int), fp);
	fread(&xbmp->mipmaps, 1, sizeof(int), fp);
	fread(&xbmp->format, 1, sizeof(int), fp);
		
	xbmp->image_data = malloc(xbmp->image_data_size);
	if(!xbmp->image_data) {
		fclose(fp);
		return 2;
	}
	
	fread(xbmp->image_data, 1, xbmp->image_data_size, fp);
	fclose(fp);
		
	return 0;
}

int XBmp_Save(struct XBmp *xbmp, const char *filename)
{
	FILE *fp;
	
	fp = fopen(filename, "wb");
	if(!fp)
		return 1;
		
	fwrite(&xbmp->image_data_size, 1, sizeof(int), fp);
	fwrite(&xbmp->unknown1, 1, sizeof(int), fp);
	fwrite(&xbmp->width, 1, sizeof(int), fp);
	fwrite(&xbmp->height, 1, sizeof(int), fp);
	fwrite(&xbmp->width2, 1, sizeof(int), fp);
	fwrite(&xbmp->unknown2, 1, sizeof(int), fp);
	fwrite(&xbmp->mipmaps, 1, sizeof(int), fp);
	fwrite(&xbmp->format, 1, sizeof(int), fp);
		
	if(fwrite(xbmp->image_data, 1, xbmp->image_data_size, fp) != xbmp->image_data_size) {
		fclose(fp);
		return 2;
	}
	
	fclose(fp);	
	return 0;
}

void XBmp_Unload(struct XBmp *xbmp)
{
	free(xbmp->image_data);
	xbmp->image_data = NULL;
}


