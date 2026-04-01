#include <stdlib.h>
#include <string.h>
#include "geombuf.h"

#define VERTS_STEP 256
#define TRIS_STEP 256
#define TRI_SIZE (sizeof(int)*3)

void GeomBuf_Init(struct st_GeomBuf *buf, unsigned vert_size, unsigned max_verts)
{
	buf->vert_size = vert_size;
	buf->verts_max_cnt = max_verts;
	buf->verts = NULL;
	buf->verts_count = 0;
	buf->verts_allocated = 0;
	buf->tris = NULL;
	buf->tris_count = 0;
	buf->tris_allocated = 0;
}

void GeomBuf_Destroy(struct st_GeomBuf *buf)
{
	free(buf->verts);
	free(buf->tris);
}

int GeomBuf_FindVert(struct st_GeomBuf *buf, void *vert)
{
	int i;
	for(i = 0; i < buf->verts_count; i++) {
		void *v = (buf->verts+buf->vert_size*i);
		if(memcmp(v, vert, buf->vert_size) == 0)
			return i;
	}
	return -1;
}

int GeomBuf_AddVert(struct st_GeomBuf *buf, void *vert)
{
	void *v;
	
	if(buf->verts_allocated == buf->verts_count) {
		void *p = realloc(
			buf->verts,
			(buf->verts_allocated+VERTS_STEP)*buf->vert_size
		);
		
		if(!p)
			return -1;
			
		buf->verts = p;
		buf->verts_allocated += VERTS_STEP;
	}
	
	v = (buf->verts+buf->vert_size*buf->verts_count);
	memcpy(v, vert, buf->vert_size);
	return buf->verts_count++;
}

int GeomBuf_AddTri(struct st_GeomBuf *buf, void *v1, void *v2, void *v3)
{
	int i1 = GeomBuf_FindVert(buf, v1);
	int i2 = GeomBuf_FindVert(buf, v2);
	int i3 = GeomBuf_FindVert(buf, v3);
	
	int alloc = (i1 == -1) + (i2 == -1) + (i3 == -1);
	
	if((buf->verts_count+alloc) > buf->verts_max_cnt)
		return ERROR_BUFFERISFULL;
		
	if(i1 == -1) i1 = GeomBuf_AddVert(buf, v1);
	if(i2 == -1) i2 = GeomBuf_AddVert(buf, v2);
	if(i3 == -1) i3 = GeomBuf_AddVert(buf, v3);
	
	if(i1 == -1 || i2 == -1 || i3 == -1)
		return ERROR_NOMEM;
		
	if(buf->tris_allocated == buf->tris_count) {
		void *p = realloc(
			buf->tris,
			(buf->tris_allocated+TRIS_STEP)*TRI_SIZE
		);
		
		if(!p)
			return ERROR_NOMEM;
			
		buf->tris = p;
		buf->tris_allocated += TRIS_STEP;		
	}
	
	buf->tris[buf->tris_count*3+0] = i1;
	buf->tris[buf->tris_count*3+1] = i2;
	buf->tris[buf->tris_count*3+2] = i3;
	return buf->tris_count++;
}

