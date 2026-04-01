#include <string.h>
#include <stdlib.h>
#include <lwsdk\lwmodule.h>
#include <lwsdk\lwimageio.h>
#include "xbmp.h"
#include "dll.h"

#include <windows.h>
#include "dxt_decompress.h"

const char *get_ext(const char *path)
{
	const char *sep1, *sep2, *dot;
	
	sep1 = strrchr(path, '/');
	sep2 = strrchr(path, '\\');
	dot = strrchr(path, '.');
	
	if(dot > sep1 && dot > sep2) 
		return dot;
		
	return NULL;
}

int bgrrgb(int clr)
{
	int c1 = clr & 0xFF;
	int c2 = (clr >> 16) & 0xFF;
	return (clr & 0xFF00FF00) | (c1 << 16) | (c2);
}

void bgrrgb_image(int *data, int w, int h)
{
	int x, y;
	
	for(y = 0; y < h; y++)
		for(x = 0; x < w; x++)
			data[y*w+x] = bgrrgb(data[y*w+x]);
}

XCALL_(static int) XBMPLoad(long version, GlobalFunc *global, LWImageLoaderLocal *local, void *serverdata)
{
	struct XBmp xbmp;
	const char *ext;
	int i, result;
	LWImageProtocolID ip;
	
	if(version != LWIMAGELOADER_VERSION)
		return AFUNC_BADVERSION;
		
	ext = get_ext(local->filename);
	if(!ext || 0 != stricmp(ext, ".xbmp")) {
		local->result = IPSTAT_NOREC;
		return AFUNC_OK;
	}
	
	result = XBmp_Load(&xbmp, local->filename);
	if(result != 0) {
		switch(result) {
			case 1:  local->result = IPSTAT_BADFILE; break;
			default: local->result = IPSTAT_NOREC; break;
		}
		return AFUNC_OK;
	}
	
	switch(xbmp.format) {
		case XBMP_FORMAT_RGBX8888:
		case XBMP_FORMAT_RGBA8888: {
			ip = local->begin(local->priv_data, LWIMTYP_RGBA32);
			
			LWIP_SETSIZE(ip, xbmp.width, xbmp.height);
			LWIP_ASPECT(ip, 1.f);
			
			if(xbmp.mipmaps == 0) {
				bgrrgb_image(xbmp.image_data, xbmp.width, xbmp.height);
				
				for(i = 0; i < xbmp.height; i++)
					LWIP_SENDLINE(ip, i, xbmp.image_data + (i*xbmp.width*4));
			} else {
				struct XBmpMipmap *mip = xbmp.image_data;
				
				bgrrgb_image(xbmp.image_data + mip->offset, xbmp.width, xbmp.height);
				
				for(i = 0; i < xbmp.height; i++)
					LWIP_SENDLINE(ip, i, xbmp.image_data + mip->offset + (i*xbmp.width*4));				
			}
			
			local->result = LWIP_DONE(ip, IPSTAT_OK);
			local->done(local->priv_data, ip);
		} break;
		
		case XBMP_FORMAT_DXT1: {
			DWORD *data = malloc(xbmp.width * xbmp.height * 4);
			if(!data) {
				XBmp_Unload(&xbmp);
				local->result = IPSTAT_FAILED;
				return AFUNC_OK;
			}
			
			if(xbmp.mipmaps == 0) {
				if(xbmp.image_data_size < image_size_dxt1(xbmp.width, xbmp.height)) {
					free(data);
					XBmp_Unload(&xbmp);
					local->result = IPSTAT_FAILED;
					return AFUNC_OK;
				}
				
				decompress_dxt1(xbmp.image_data, data, xbmp.width, xbmp.height);
			} else {
				struct XBmpMipmap *mip = xbmp.image_data;
				
				if(xbmp.image_data_size-mip->offset < image_size_dxt1(xbmp.width, xbmp.height)) {
					free(data);
					XBmp_Unload(&xbmp);
					local->result = IPSTAT_FAILED;
					return AFUNC_OK;
				}
				
				decompress_dxt1(xbmp.image_data + mip->offset, data, xbmp.width, xbmp.height);				
			}
			
			bgrrgb_image(data, xbmp.width, xbmp.height);
			
			ip = local->begin(local->priv_data, LWIMTYP_RGBA32);

			LWIP_SETSIZE(ip, xbmp.width, xbmp.height);
			LWIP_ASPECT(ip, 1.f);

			for(i = 0; i < xbmp.height; i++)
				LWIP_SENDLINE(ip, i, data + (i*xbmp.width));
				
			local->result = LWIP_DONE(ip, IPSTAT_OK);
			local->done(local->priv_data, ip);
			
			free(data);
		} break;
		
		case XBMP_FORMAT_DXT3: {
			DWORD *data = malloc(xbmp.width * xbmp.height * 4);
			if(!data) {
				XBmp_Unload(&xbmp);
				local->result = IPSTAT_FAILED;
				return AFUNC_OK;
			}
			
			if(xbmp.mipmaps == 0) {
				if(xbmp.image_data_size < image_size_dxt3(xbmp.width, xbmp.height)) {
					free(data);
					XBmp_Unload(&xbmp);
					local->result = IPSTAT_FAILED;
					return AFUNC_OK;
				}
			
				decompress_dxt3(xbmp.image_data, data, xbmp.width, xbmp.height);
			} else {
				struct XBmpMipmap *mip = xbmp.image_data;
				
				if(xbmp.image_data_size-mip->offset < image_size_dxt3(xbmp.width, xbmp.height)) {
					free(data);
					XBmp_Unload(&xbmp);
					local->result = IPSTAT_FAILED;
					return AFUNC_OK;
				}
				
				decompress_dxt3(xbmp.image_data + mip->offset, data, xbmp.width, xbmp.height);				
			}
			
			bgrrgb_image(data, xbmp.width, xbmp.height);
			
			ip = local->begin(local->priv_data, LWIMTYP_RGBA32);

			LWIP_SETSIZE(ip, xbmp.width, xbmp.height);
			LWIP_ASPECT(ip, 1.f);

			for(i = 0; i < xbmp.height; i++)
				LWIP_SENDLINE(ip, i, data + (i*xbmp.width));
				
			local->result = LWIP_DONE(ip, IPSTAT_OK);
			local->done(local->priv_data, ip);
			
			free(data);
		} break;
		
		default:
			local->result = IPSTAT_NOREC;
	}
	
	XBmp_Unload(&xbmp);
	
	return AFUNC_OK;
}

void *Startup(void)
{
	return (void*)4;
}

void Shutdown(void *moddata)
{
}

ServerRecord _server_desc[] = {
	{ LWIMAGELOADER_CLASS, "Hobbit_XBMP", XBMPLoad },
	{ NULL }
};

DLLIMPORT ModuleDescriptor _mod_descrip = {
	MOD_SYSSYNC,
	MOD_SYSVER,
	MOD_MACHINE,
	Startup,
	Shutdown,
	_server_desc
};

