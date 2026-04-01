#ifndef __UTILS_H__
#define __UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

BOOL SelectFileBin(char *filename, size_t buflen);
BOOL SelectFileRGeom(char *filename, size_t buflen);

char *extract_path(char *dest, const char *src);
char *extract_filename(char *dest, const char *src);
const char *get_ext(const char *path);

#ifdef __cplusplus
}
#endif

#endif

