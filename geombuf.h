#ifndef __GEOMBUF_H__
#define __GEOMBUF_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ERROR_NOMEM          -1
#define ERROR_BUFFERISFULL   -2

struct st_GeomBuf
{
	unsigned vert_size;
	unsigned verts_max_cnt;
	
	char *verts;
	unsigned verts_count;
	unsigned verts_allocated;
	
	int *tris;
	unsigned tris_count;
	unsigned tris_allocated;
};

void GeomBuf_Init(struct st_GeomBuf *buf, unsigned vert_size, unsigned max_verts);
void GeomBuf_Destroy(struct st_GeomBuf *buf);
int GeomBuf_FindVert(struct st_GeomBuf *buf, void *vert);
int GeomBuf_AddVert(struct st_GeomBuf *buf, void *vert);
int GeomBuf_AddTri(struct st_GeomBuf *buf, void *v1, void *v2, void *v3);

#ifdef __cplusplus
}
#endif

#endif

