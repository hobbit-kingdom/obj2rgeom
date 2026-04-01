#include <windows.h>
#include <string.h>
#include "utils.h"

BOOL SelectFileBin(char *filename, size_t buflen)
{
	OPENFILENAME ofn = { sizeof(OPENFILENAME) };
	
	ofn.lpstrFilter = "Model Binary File (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
	ofn.lpstrDefExt = "bin";
	ofn.Flags = OFN_NOCHANGEDIR;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = buflen;
	
	return GetSaveFileName(&ofn);
}

BOOL SelectFileRGeom(char *filename, size_t buflen)
{
	OPENFILENAME ofn = { sizeof(OPENFILENAME) };
	
	ofn.lpstrFilter = "RGeom File (*.rgeom)\0*.rgeom\0All Files (*.*)\0*.*\0";
	ofn.lpstrDefExt = "rgeom";
	ofn.Flags = OFN_NOCHANGEDIR;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = buflen;
	
	return GetSaveFileName(&ofn);
}

char *extract_path(char *dest, const char *src)
{
	const char *sep1, *sep2;
	size_t len;
	
	sep1 = strrchr(src, '/');
	sep2 = strrchr(src, '\\');
	
	if(sep1 > sep2) {
		len = sep1 - src + 1;
		strncpy(dest, src, len);
		dest[len] = '\0';
	} else if(sep2 > sep1) {
		len = sep2 - src + 1;
		strncpy(dest, src, len);
		dest[len] = '\0';
	} else {
		dest[0] = '\0';
	}
	
	return dest;
}

char *extract_filename(char *dest, const char *src)
{
	const char *sep1, *sep2;
	
	sep1 = strrchr(src, '/');
	sep2 = strrchr(src, '\\');
	
	if(sep1 > sep2) {
		strcpy(dest, sep1+1);
	} else if(sep2 > sep1) {
		strcpy(dest, sep2+1);
	} else {
		strcpy(dest, src);
	}
	
	return dest;
}

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
