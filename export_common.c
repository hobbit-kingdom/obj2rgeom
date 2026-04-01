#include <stdlib.h>
#include <string.h>
#include "export.h"

int merge_rawmodel(struct st_RawModel *dest, struct st_RawModel *src)
{
	int base_vertex;
	void *p;
	int i;
	
	base_vertex = dest->points_count;
	p = realloc(dest->points, (dest->points_count+src->points_count)*sizeof(float)*3);
	if(p) {
		dest->points = p;
		memcpy(&dest->points[base_vertex], src->points, src->points_count*sizeof(float)*3);
		dest->points_count += src->points_count;
	} else {
		return 1;
	}
	
	p = realloc(dest->tris, (dest->tris_count+src->tris_count)*sizeof(struct st_RawTriangle));
	if(p) {
		dest->tris = p;
		memcpy(&dest->tris[dest->tris_count], src->tris, src->tris_count*sizeof(struct st_RawTriangle));
		
		for(i = dest->tris_count; i < (dest->tris_count+src->tris_count); i++) {
			dest->tris[i].v[0].point += base_vertex;
			dest->tris[i].v[1].point += base_vertex;
			dest->tris[i].v[2].point += base_vertex;
		}
		
		dest->tris_count += src->tris_count;
	} else {
		return 1;
	}
	
	return 0;
}
