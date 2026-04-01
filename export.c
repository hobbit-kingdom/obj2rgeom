/* Replace "dll.h" with the name of your header */
#include "dll.h"
#include <stdio.h>
#include <windows.h>
#include <time.h>

#include "utils.h"
#include "vector.h"
#include "crc32.h"
#include "export.h"
#include "rgeom.h" /* RCollision */
#include "geombuf.h"

#include <lwsdk\lwmodule.h>
#include <lwsdk\lwcmdseq.h>
#include <lwsdk\lwmodeler.h>
#include <lwsdk\lwhost.h>
#include <lwsdk\lwsurf.h>
#include <lwsdk\lwtxtr.h>
#include <lwsdk\lwimage.h>

void *Startup(void)
{
	return (void*)3;
}

void Shutdown(void *moddata)
{
}

int scan_polygons_cb(void *_it, LWPolID p)
{
	LWPolID **it = _it;
	
	**it = p;
	(*it)++;
	
	return 0;
}

int scan_points_cb(void *_it, LWPntID p)
{
	LWPntID **it = _it;
	
	**it = p;
	(*it)++;
	
	return 0;
}

int indexof(LWPntID array[], int array_size, LWPntID element)
{
	int i;
	
	for(i = 0; i < array_size; i++)
		if(array[i] == element)
			return i;
			
	return -1;
}

int check_for_uvs(LWMessageFuncs *msgfunc, LWMeshInfoID layer)
{
	void *vm;
	int dim;
	int have_uvs;
		
	vm = layer->pntVLookup(layer, LWVMAP_TXUV, "Texture");
	if(vm) {
		have_uvs = 1;
		
		dim = layer->pntVSelect(layer, vm);
		if(dim != 2)
			msgfunc->warning("VMAP dim != 2", NULL);
	} else {
		have_uvs = 0;
		msgfunc->warning("Can't find UV's", NULL);
	}
	
	return have_uvs;
}

/* optimised */
int calculate_normals_1(struct st_RawModel *dest, struct st_RGeomMtlList *mlist)
{	
	struct st_VertFace {
		int point;
		int tri;
	};

	int verttri_sort_cb(const void *_a, const void *_b) {
		const struct st_VertFace *a = _a;
		const struct st_VertFace *b = _b;
		
		if(a->point > b->point)
			return 1;
		if(a->point < b->point)
			return -1;
		return 0;
	}	
	
	struct st_VertFace *vertfaces;
	int vertfaces_cnt;
	
	int i, j, k;
	float (*tri_normals)[3];
	
	tri_normals = malloc(sizeof(float) * 3 * dest->tris_count);
	
	for(i = 0; i < dest->tris_count; i++) {
		vtrinormal(
			tri_normals[i],
			dest->points[dest->tris[i].v[0].point],
			dest->points[dest->tris[i].v[1].point],
			dest->points[dest->tris[i].v[2].point]
		);
		
		vcopy(dest->tris[i].v[0].normal, tri_normals[i]);
		vcopy(dest->tris[i].v[1].normal, tri_normals[i]);
		vcopy(dest->tris[i].v[2].normal, tri_normals[i]);
	}
	
	vertfaces_cnt = dest->tris_count * 3;
	vertfaces = malloc(sizeof(struct st_VertFace) * vertfaces_cnt);
	for(i = 0; i < dest->tris_count; i++) {
		for(j = 0; j < 3; j++) {
			vertfaces[i*3+j].point = dest->tris[i].v[j].point;
			vertfaces[i*3+j].tri = i;
		}
	}
	qsort(vertfaces, vertfaces_cnt, sizeof(struct st_VertFace), verttri_sort_cb);

	for(i = 0; i < dest->tris_count; i++) {
		int mtl_id = (int)dest->tris[i].surface_tag;
		const struct st_RGeomMtl *mtl = &mlist->materials[mtl_id];
		
		if(mtl->sm_angle <= 0.f)
			continue;

		for(k = 0; k < 3; k++) {
			struct st_VertFace key, *result; 
			key.point = dest->tris[i].v[k].point;
			
			result = bsearch(
				&key, 
				vertfaces, 
				vertfaces_cnt, 
				sizeof(struct st_VertFace), 
				verttri_sort_cb
			);
			
			/* find lower bound */
			while((result-1) >= vertfaces && (result-1)->point == key.point)
				result--;
				
			/* sum up all normals of trianges this point belongs to */
			while(result->point == key.point && result != vertfaces+vertfaces_cnt) {
				if(result->tri == i) {
					result++;
					continue;
				}
				
				if(vdot3(tri_normals[i], tri_normals[result->tri]) >= mtl->sm_angle_cos) {
					vadd(dest->tris[i].v[k].normal, tri_normals[result->tri]);
				}
				
				result++;
			}
		}
		
		vnormalize(dest->tris[i].v[0].normal);
		vnormalize(dest->tris[i].v[1].normal);
		vnormalize(dest->tris[i].v[2].normal);
	}
	
	free(vertfaces);
	free(tri_normals);
	
	return 0;
}

/* linear */
int calculate_normals_2(struct st_RawModel *dest, struct st_RGeomMtlList *mlist)
{
	int i, j, k;
	float (*tri_normals)[3];
	
	tri_normals = malloc(sizeof(float) * 3 * dest->tris_count);
	
	for(i = 0; i < dest->tris_count; i++) {
		vtrinormal(
			tri_normals[i],
			dest->points[dest->tris[i].v[0].point],
			dest->points[dest->tris[i].v[1].point],
			dest->points[dest->tris[i].v[2].point]
		);
		
		vcopy(dest->tris[i].v[0].normal, tri_normals[i]);
		vcopy(dest->tris[i].v[1].normal, tri_normals[i]);
		vcopy(dest->tris[i].v[2].normal, tri_normals[i]);
	}
	
	for(i = 0; i < dest->tris_count; i++) {
		int mtl_id = (int)dest->tris[i].surface_tag;
		const struct st_RGeomMtl *mtl = &mlist->materials[mtl_id];
		
		if(mtl->sm_angle <= 0.f)
			continue;
		
		for(j = 0; j < dest->tris_count; j++) {
			if(j == i) 
				continue;
				
			for(k = 0; k < 3; k++) {		
				if(
					dest->tris[i].v[k].point == dest->tris[j].v[0].point ||
					dest->tris[i].v[k].point == dest->tris[j].v[1].point ||
					dest->tris[i].v[k].point == dest->tris[j].v[2].point
				) {
					if(vdot3(tri_normals[i], tri_normals[j]) >= mtl->sm_angle_cos) {
						vadd(dest->tris[i].v[k].normal, tri_normals[j]);
					}
				}
			}
		}
		
		vnormalize(dest->tris[i].v[0].normal);
		vnormalize(dest->tris[i].v[1].normal);
		vnormalize(dest->tris[i].v[2].normal);
	}
	
	free(tri_normals);
	
	return 0;
}

int calculate_normals(struct st_RawModel *dest, struct st_RGeomMtlList *mlist)
{
	return calculate_normals_2(dest, mlist);
}

struct st_HashVertex {
	unsigned hash;
	unsigned index;
};

int hash_sort_cb(const void *a, const void *b)
{
	struct st_HashVertex *v1, *v2;
	
	v1 = (struct st_HashVertex *)a;
	v2 = (struct st_HashVertex *)b;
	
	if(v1->hash < v2->hash)
		return -1;
	if(v1->hash > v2->hash)
		return 1;
		
	return 0;
}

int hash_search_cb(const void *a, const void *b)
{
	unsigned h1, h2;
	
	h1 = *(unsigned *)a;
	h2 = *(unsigned *)b;
	
	if(h1 < h2)
		return -1;
	if(h1 > h2)
		return 1;
		
	return 0;
}

/* optimised (?) */
void make_indexed_1(struct st_RawModel *model, struct st_PackedModel *result)
{
	unsigned *hash_linear;
	struct st_HashVertex *hash_sorted, *hash_packed;
	
	struct st_RawVertex *packed_verts;
	int *packed_indices;
	
	int i, j;
	int npackedverts;
	
	/* calculate hash for each vertex */
	hash_linear = malloc(model->tris_count * 3 * sizeof(unsigned));
	hash_sorted = malloc(model->tris_count * 3 * sizeof(struct st_HashVertex));
	
	for(i = 0; i < model->tris_count; i++) {
		for(j = 0; j < 3; j++) {
			hash_linear[i*3+j] = hash_sorted[i*3+j].hash = 
				crc32(&model->tris[i].v[j], sizeof(struct st_RawVertex));
				
			hash_sorted[i*3+j].index = i*3+j;
		}
	}
	
	/* remove duplicate hashes */
	qsort(hash_sorted, model->tris_count*3, sizeof(struct st_HashVertex), hash_sort_cb);
	
	hash_packed = malloc(model->tris_count*3*sizeof(struct st_HashVertex));
	
	hash_packed[0] = hash_sorted[0];
	npackedverts = 1;
		
	for(i = 1; i < model->tris_count*3; i++) {
		if(hash_sorted[i].hash != hash_sorted[i-1].hash) {
			hash_packed[npackedverts++] = hash_sorted[i];
		}
	}
	
	free(hash_sorted);
	
	/* create packed vertex & index buffers */
	packed_verts = malloc(npackedverts * sizeof(struct st_RawVertex));
	packed_indices = malloc(model->tris_count * 3 * sizeof(int));
	
	for(i = 0; i < npackedverts; i++) {
		int index = hash_packed[i].index;			
		memcpy(&packed_verts[i], &model->tris[index/3].v[index%3], sizeof(struct st_RawVertex));	
	}
	
	for(i = 0; i < model->tris_count*3; i++) {
	
		struct st_HashVertex *p = bsearch(
			&hash_linear[i], 
			hash_packed, npackedverts, 
			sizeof(struct st_HashVertex), hash_search_cb);
		
		/*
		struct st_HashVertex *p = hash_packed;
		while(p->hash != hash_linear[i])
			p++;
		*/
			
		int index = p-hash_packed;
	
		if(!p)
			packed_indices[i] = 0xFFFFFFFF;
		else
			packed_indices[i] = index;
	}
	
	/* free resources and exit */
	free(hash_linear);
	free(hash_packed);
	
	result->verts_count = npackedverts;
	result->verts = packed_verts;
	result->indices_count = model->tris_count*3;
	result->indices = packed_indices;
}

/* linear */
void make_indexed_2(struct st_RawModel *model, struct st_PackedModel *result)
{
	struct st_RawVertex *linear_verts;
	struct st_RawVertex *packed_verts;
	int *packed_indices;
	
	int i, j;
	int npackedverts;
	
	linear_verts = malloc(model->tris_count*3*sizeof(struct st_RawVertex));
	for(i = 0; i < model->tris_count; i++) {
		for(j = 0; j < 3; j++) {
			linear_verts[i*3+j] = model->tris[i].v[j];
		}
	}
	
	packed_verts = malloc(model->tris_count*3*sizeof(struct st_RawVertex));
	packed_indices = malloc(model->tris_count*3*sizeof(int));
	npackedverts = 0;
	
	for(i = 0; i < model->tris_count*3; i++) {
		for(j = 0; j < npackedverts; j++) {
			if(memcmp(&packed_verts[j], &linear_verts[i], sizeof(struct st_RawVertex)) == 0) {
				packed_indices[i] = j;
				goto continue_i;
			}
		}
		
		packed_verts[npackedverts] = linear_verts[i];
		packed_indices[i] = npackedverts;
		npackedverts++;
		
		continue_i:
		npackedverts = npackedverts;
	}
	
	/* free resources and exit */
	free(linear_verts);
	
	result->verts_count = npackedverts;
	result->verts = packed_verts;
	result->indices_count = model->tris_count*3;
	result->indices = packed_indices;
}

void make_indexed(struct st_RawModel *model, struct st_PackedModel *result)
{
	make_indexed_2(model, result);
}

int extract_LWLayer(struct st_RawModel *dest, LWMeshInfoID layer)
{
	size_t lw_points_count;
	LWPntID *lw_points;
	
	size_t lw_polygons_count;
	LWPolID *lw_polygons;
	
	LWPntID *it_p;
	LWPolID *it_f;
	
	void *vm_uv[4];
	void *vm_color;
	
	size_t tris_count;
	int i;
	
	/* lookup vertex maps */
	vm_uv[0] = layer->pntVLookup(layer, LWVMAP_TXUV, "Texture");
	vm_uv[1] = layer->pntVLookup(layer, LWVMAP_TXUV, "Texture 2");
	vm_uv[2] = layer->pntVLookup(layer, LWVMAP_TXUV, "Texture 3");
	vm_uv[3] = layer->pntVLookup(layer, LWVMAP_TXUV, "Texture 4");
	vm_color = layer->pntVLookup(layer, LWVMAP_RGBA, "Vertex Color");
	if(!vm_color)
		vm_color = layer->pntVLookup(layer, LWVMAP_RGB, "Vertex Color");
	
	/* retrieve point IDs and data */
	lw_points_count = layer->numPoints(layer);
	lw_points = malloc(sizeof(LWPntID) * lw_points_count);
	
	it_p = lw_points;
	layer->scanPoints(layer, scan_points_cb, &it_p);
	
	dest->points_count = lw_points_count;
	dest->points = malloc(sizeof(float) * 3 * lw_points_count);
	
	for(i = 0; i < lw_points_count; i++)
		layer->pntBasePos(layer, lw_points[i], dest->points[i]);
		
	/* retrieve polygon IDs */
	lw_polygons_count = layer->numPolygons(layer);
	lw_polygons = malloc(sizeof(LWPolID) * lw_polygons_count);
	
	it_f = lw_polygons;
	layer->scanPolys(layer, scan_polygons_cb, &it_f);
	
	/* count how many triangles we need */
	tris_count = 0;
	for(i = 0; i < lw_polygons_count; i++) {
		LWPolID p = lw_polygons[i];
		if(layer->polType(layer, p) == LWPOLTYPE_FACE && layer->polSize(layer, p) >= 3)
			tris_count += (layer->polSize(layer, p) - 2);
	}
	
	/* retrieve polygon data */
	dest->tris_count = 0;
	dest->tris = calloc(tris_count, sizeof(struct st_RawTriangle));
	
	for(i = 0; i < lw_polygons_count; i++) {
		LWPolID polygon = lw_polygons[i];
		int j;
		const char *surface_tag;
		
		if(layer->polType(layer, polygon) != LWPOLTYPE_FACE || layer->polSize(layer, polygon) < 3)
			continue;
			
		/* get associated surface */
		surface_tag = layer->polTag(layer, polygon, LWPTAG_SURF);

		j = layer->polSize(layer, polygon);
		
		/* алгоритм триангул€ции подсмотрен в библиотечке TinyGL */
		while(j >= 3) {
			LWPntID lw_p1, lw_p2, lw_p3;
			struct st_RawTriangle *t = &dest->tris[dest->tris_count++];
			int k, dim;
			
			j--;
			lw_p1 = layer->polVertex(layer, polygon, j);
			lw_p2 = layer->polVertex(layer, polygon, 0);
			lw_p3 = layer->polVertex(layer, polygon, j-1);
			
			t->v[0].point = indexof(lw_points, lw_points_count, lw_p1);
			t->v[1].point = indexof(lw_points, lw_points_count, lw_p2);
			t->v[2].point = indexof(lw_points, lw_points_count, lw_p3);
			
			for(k = 0; k < 4; k++) {
				if(!vm_uv[k])
					continue;
					
				dim = layer->pntVSelect(layer, vm_uv[k]);
				
				if(!layer->pntVPGet(layer, lw_p1, polygon, t->v[0].uv[k]))
					layer->pntVGet(layer, lw_p1, t->v[0].uv[k]);
	
				if(!layer->pntVPGet(layer, lw_p2, polygon, t->v[1].uv[k]))
					layer->pntVGet(layer, lw_p2, t->v[1].uv[k]);
				
				if(!layer->pntVPGet(layer, lw_p3, polygon, t->v[2].uv[k]))
					layer->pntVGet(layer, lw_p3, t->v[2].uv[k]);
					
				/* flip UVs */
				t->v[0].uv[k][1] = 1.f - t->v[0].uv[k][1];
				t->v[1].uv[k][1] = 1.f - t->v[1].uv[k][1];
				t->v[2].uv[k][1] = 1.f - t->v[2].uv[k][1];
			}
			
			if(vm_color) {
				dim = layer->pntVSelect(layer, vm_color);
				
				if(!layer->pntVPGet(layer, lw_p1, polygon, t->v[0].color_rgba))
					layer->pntVGet(layer, lw_p1, t->v[0].color_rgba);
	
				if(!layer->pntVPGet(layer, lw_p2, polygon, t->v[1].color_rgba))
					layer->pntVGet(layer, lw_p2, t->v[1].color_rgba);
				
				if(!layer->pntVPGet(layer, lw_p3, polygon, t->v[2].color_rgba))
					layer->pntVGet(layer, lw_p3, t->v[2].color_rgba);
					
				if(dim == 3) {
					t->v[0].color_rgba[3] = 1.f;
					t->v[1].color_rgba[3] = 1.f;
					t->v[2].color_rgba[3] = 1.f;	
				}
			} else {
				vset4(t->v[0].color_rgba, 1.f, 1.f, 1.f, 1.f);
				vset4(t->v[1].color_rgba, 1.f, 1.f, 1.f, 1.f);
				vset4(t->v[2].color_rgba, 1.f, 1.f, 1.f, 1.f);
			}
			
			t->surface_tag = surface_tag;
		}
	}
	
	/* free resources */
	free(lw_points);
	free(lw_polygons);
	
	return 0;
}

/* remap surface_tag's from lightwave TAGs into pointer to my surface list */
int extract_LWSurfaces(struct st_RGeomMtlList *mlist, struct st_RawModel *model, const char *objname, GlobalFunc *global)
{
	LWMessageFuncs *msgfunc;
	LWSurfaceFuncs *surff;
	LWTextureFuncs *texf;
	LWImageList *imglist;
	
	LWSurfaceID *s_ids;	
	int i, j;
	
	msgfunc = global(LWMESSAGEFUNCS_GLOBAL, GFUSE_TRANSIENT);
	surff = global(LWSURFACEFUNCS_GLOBAL, GFUSE_TRANSIENT);
	texf = global(LWTEXTUREFUNCS_GLOBAL, GFUSE_TRANSIENT);
	imglist = global(LWIMAGELIST_GLOBAL, GFUSE_TRANSIENT);
	
	for(i = 0; i < model->tris_count; i++) {
		/* search for previosly extracted surface */
		for(j = 0; j < mlist->count; j++) {
			if(strcmp(mlist->materials[j].name, model->tris[i].surface_tag) == 0) {
				model->tris[i].surface_tag = j;
				break;
			}
		}
		
		/* add, if not found */
		if(j == mlist->count) {
			int m = mlist->count;
			void *p = realloc(mlist->materials, sizeof(*mlist->materials) * (mlist->count+1));
			
			if(!p) {
				return ERROR_NOMEM;
			}
			
			mlist->materials = p;
			mlist->count++;
			
			/* retrieve surface parameters */
			s_ids = surff->byName(model->tris[i].surface_tag, objname);
			if(s_ids && s_ids[0]) {
				LWSurfaceID s_id = s_ids[0];
				const char *surfname = surff->name(s_id);
				const char *surftexture = NULL;
				double *surfsmooth_flt = surff->getFlt(s_id, SURF_SMAN); /* radians */
				LWTextureID tex_id = surff->getTex(s_id, SURF_COLR);
				
				if(tex_id) {
					LWTLayerID layer = texf->firstLayer(tex_id);
					if(layer && texf->layerType(layer) == TLT_IMAGE) {
						LWImageID image_id = 0;
						texf->getParam(layer, TXTAG_IMAGE, &image_id);
						if(image_id) {
							surftexture = imglist->filename(image_id, 0);
						}
					}
				}
				
				mlist->materials[m].name = strdup(surfname);
				mlist->materials[m].sm_angle = surfsmooth_flt[0];
				mlist->materials[m].sm_angle_cos = cos(mlist->materials[m].sm_angle);
				if(surftexture)
					mlist->materials[m].texture = strdup(surftexture);
				else
					mlist->materials[m].texture = NULL;
				
				//{char str[128];
				//sprintf(str, "Surface: %s\nImage: %s\n", surfname, surftexture);
				//msgfunc->info(str, NULL);}
			} else {
				char str[768];
				
				sprintf(str, "Cannot find surface '%s' of object '%s'\n", 
					model->tris[i].surface_tag, objname);
				msgfunc->error(str, NULL);
				
				mlist->materials[m].name = strdup(model->tris[i].surface_tag);
				mlist->materials[m].sm_angle = -1.f;
				mlist->materials[m].sm_angle_cos = 0.f;	
				mlist->materials[m].texture = NULL;
			}
			
			model->tris[i].surface_tag = m;
		}
	}
	
	return 0;
}

#if 0
int test_ExportModel(long version, GlobalFunc *global, LWModCommand *local, void *serverdata)
{
	LWStateQueryFuncs *statefunc;
	LWObjectFuncs *objfunc;
	LWMessageFuncs *msgfunc;
	
	const char *objname;
	int objid;
	
	int have_uvs;
	
	char filename[256] = "";
	FILE *out;
	
	int i, j;
	
	struct st_RawModel rawmodel;
	struct st_PackedModel pmodel;

	if(version != LWMODCOMMAND_VERSION)
		return AFUNC_BADVERSION;
		
	statefunc = global(LWSTATEQUERYFUNCS_GLOBAL, GFUSE_TRANSIENT);
	objfunc = global(LWOBJECTFUNCS_GLOBAL, GFUSE_TRANSIENT);
	msgfunc = global(LWMESSAGEFUNCS_GLOBAL, GFUSE_TRANSIENT);
	
	/* retrieve name of current object */
	objname = statefunc->object();
	
	if(objname == NULL) {
		msgfunc->error("objname == NULL", NULL);
		return AFUNC_OK;
	}
		
	for(i = 0; i < objfunc->numObjects(); i++) {
		const char *filename = objfunc->filename(i);
		
		if(filename == NULL)
			filename = objfunc->refName(i);
			
		if(filename == NULL)
			msgfunc->error("filename == NULL", NULL);
		
		if(filename && strcmp(objname, filename) == 0) {
			objid = i;
			break;
		}
	}
	
	/* select filename for export */
	if(!SelectFileBin(filename, sizeof(filename)))
		return AFUNC_OK;
	
	/* do export itself */
	if(objfunc->layerExists(objid, 0)) {
		LWMeshInfoID layer;
		
		layer = objfunc->layerMesh(objid, 0);
		
		/* check texcoords presence */	
		have_uvs = check_for_uvs(msgfunc, layer);
		
		/* extract the model */
		extract_LWLayer(&rawmodel, layer);
		
		/* calculate vertex normals */
		calculate_normals(&rawmodel, M_PI/4);
		
#if 0
		/* write all that shit into the file */
		out = fopen(filename, "wb");
		if(out) {
			i = rawmodel.tris_count*3;
			fwrite(&i, 1, sizeof(i), out);
			
			for(i = 0; i < rawmodel.tris_count; i++) {	
				for(j = 0; j < 3; j++) {
					fwrite(rawmodel.points[rawmodel.tris[i].v[j].point], 1, 12, out);
					fwrite(rawmodel.tris[i].v[j].normal, 1, 12, out);
					fwrite(rawmodel.tris[i].v[j].uv[0], 1, 8, out);
				}
			}
			
			fclose(out);
		} else {
			msgfunc->error("Can't open file for writing", NULL);
		}
#elif 1
		make_indexed(&rawmodel, &pmodel);

		/* write all that shit into the file */
		out = fopen(filename, "wb");
		if(out) {
			i = pmodel.verts_count;
			fwrite(&i, 1, sizeof(i), out);
			
			for(i = 0; i < pmodel.verts_count; i++) {	
				fwrite(rawmodel.points[pmodel.verts[i].point], 1, 12, out);
				fwrite(pmodel.verts[i].normal, 1, 12, out);
				fwrite(pmodel.verts[i].uv[0], 1, 8, out);
			}
			
			i = pmodel.indices_count;
			fwrite(&i, 1, sizeof(i), out);
			
			for(i = 0; i < pmodel.indices_count; i++) {
				unsigned short sj = pmodel.indices[i];
				fwrite(&sj, 1, sizeof(sj), out);
			}
			
			fclose(out);
		} else {
			msgfunc->error("Can't open file for writing", NULL);
		}
		
		free(pmodel.verts);
		free(pmodel.indices);	
#else
		/* write all that shit into the file */
		out = fopen(filename, "wb");
		if(out) {
			i = rawmodel.points_count;
			fwrite(&i, 1, sizeof(i), out);
			
			fwrite(rawmodel.points, 1, sizeof(float)*3*rawmodel.points_count, out);
			
			i = rawmodel.tris_count;
			fwrite(&i, 1, sizeof(i), out);
			fwrite(rawmodel.tris, 1, sizeof(struct st_RawTriangle)*rawmodel.tris_count, out);
			
			fclose(out);
		} else {
			msgfunc->error("Can't open file for writing", NULL);
		}	
#endif
		
		/* free resources*/		
		free(rawmodel.points);
		free(rawmodel.tris);
		
		if(layer->destroy)
			layer->destroy(layer);
	}
		
	return AFUNC_OK;
}
#endif

int sort_by_surface_cb(const void *a, const void *b)
{
	struct st_RawTriangle *t1 = (struct st_RawTriangle *)a;
	struct st_RawTriangle *t2 = (struct st_RawTriangle *)b;
	
	if(t1->surface_tag > t2->surface_tag)
		return 1;
	if(t1->surface_tag < t2->surface_tag)
		return -1;
		
	return 0;	
}

#include "rgeom_options.h"

int test_ExportRGeom(long version, GlobalFunc *global, LWModCommand *local, void *serverdata)
{
	FILE *f_log = fopen("C:\\Users\\User\\Desktop\\lw_plugin.log", "a");
	clock_t t_start, t_end;
	
	LWStateQueryFuncs *statefunc;
	LWObjectFuncs *objfunc;
	LWMessageFuncs *msgfunc;

	
	/* options */
	int opt_collision = COLL_BYRENDERSURFACES;
	char collision_surface[256];
	
	char *objname;
	int objid;
	
	int have_uvs;
	
	char filename[256] = "";
	FILE *out;
	
	int i, j, start;
	int maxlayers;
	
	struct st_RawModel rawmodel; /* full model */
	struct st_RawModel layer_rawmodel; /* single layer */
	struct st_RawModel *collision_model = NULL;
	
	unsigned surf_parts;
	unsigned nparts;
	struct st_RGeomPart *parts;
	
	struct RCollision rcollision = { 0 };
	struct RCollision *rcollision_ptr = NULL;
	
	struct st_RGeomMtlList mlist = { 0 };
	
	if(f_log) fprintf(f_log, "----------------------------------------------------\n");

	if(version != LWMODCOMMAND_VERSION)
		return AFUNC_BADVERSION;
		
	statefunc = global(LWSTATEQUERYFUNCS_GLOBAL, GFUSE_TRANSIENT);
	objfunc = global(LWOBJECTFUNCS_GLOBAL, GFUSE_TRANSIENT);
	msgfunc = global(LWMESSAGEFUNCS_GLOBAL, GFUSE_TRANSIENT);
	
	/* retrieve name of current object */
	if(statefunc->object() == NULL) {
		msgfunc->error("statefunc->object() == NULL", NULL);
		return AFUNC_OK;
	}
	
	/* Strange but in LW2015 object name gets overriden later somehow, so I have to strdup it */
	objname = strdup(statefunc->object());
		
	for(i = 0; i < objfunc->numObjects(); i++) {
		const char *filename = objfunc->filename(i);
		
		if(filename == NULL)
			filename = objfunc->refName(i);
			
		if(filename == NULL)
			msgfunc->error("filename == NULL", NULL);
		
		if(filename && strcmp(objname, filename) == 0) {
			objid = i;
			break;
		}
	}
	
	/* run options dialog */
	if(!run_rgeom_export_options(global, &opt_collision, collision_surface)) {
		free(objname);
		return AFUNC_OK;
	}
	
	/* select filename for export */
	if(!SelectFileRGeom(filename, sizeof(filename))) {
		free(objname);
		return AFUNC_OK;
	}
	
	/* do export itself */
	rawmodel.points_count = 0;
	rawmodel.points = NULL;
	rawmodel.tris_count = 0;
	rawmodel.tris = NULL;
	
	maxlayers = objfunc->maxLayers(objid);
	for(i = 0; i < maxlayers; i++) {
		LWMeshInfoID layer;
		
		if(!objfunc->layerExists(objid, i)) 
			continue;
			
		layer = objfunc->layerMesh(objid, i);
		
		/* check texcoords presence */	
		have_uvs = check_for_uvs(msgfunc, layer);
		
		/* extract the model */
		extract_LWLayer(&layer_rawmodel, layer);
		if(extract_LWSurfaces(&mlist, &layer_rawmodel, objname, global) != 0)
			msgfunc->error("Out of memory! (extract_LWSurfaces)", NULL);
		
		/* calculate normals */
		/* LW allows smooth edges between different surfaces in single layer, so it should be done here */
		t_start = clock();
		calculate_normals(&layer_rawmodel, &mlist);
		t_end = clock();
		if(f_log) fprintf(f_log, "calculate_normals took %u clocks\n", t_end-t_start);
		
		/* concatenate */
		if(0 != merge_rawmodel(&rawmodel, &layer_rawmodel)) {
			msgfunc->error("Out of memory! (merge)", NULL);
		}
		
		/* free resources*/
		free(layer_rawmodel.points);
		free(layer_rawmodel.tris);
		
		if(layer->destroy)
			layer->destroy(layer);
	}
	
	/* split model into parts by associaced surfaces */
	nparts = 0;
	parts = NULL;
	
	qsort(rawmodel.tris, rawmodel.tris_count, sizeof(struct st_RawTriangle), sort_by_surface_cb);
	
	surf_parts = 1;
	for(i = 1; i < rawmodel.tris_count; i++) {
		if(rawmodel.tris[i].surface_tag != rawmodel.tris[i-1].surface_tag)
			surf_parts++;
	}
	
	start = 0;
	for(i = 0; i < surf_parts; i++) {
		struct st_RawModel model;
		struct st_RGeomMtl *mtl;
		
		if(start >= rawmodel.tris_count) {
			msgfunc->warning("lolwut? Something is messed up", NULL);
			break;
		}
		
		model.points_count = rawmodel.points_count;
		model.points = rawmodel.points;
		
		model.tris = rawmodel.tris + start;
		model.tris_count = 1;
		
		start ++;
		while(start < rawmodel.tris_count && rawmodel.tris[start].surface_tag == model.tris[0].surface_tag) {
			model.tris_count++;
			start++;
		}
		
		mtl = &mlist.materials[(int)(model.tris[0].surface_tag)];
		if(opt_collision == COLL_BYSURFACE && mtl->name && 0 == strcmp(mtl->name, collision_surface)) {
			i--;
			surf_parts--;
			
			collision_model = malloc(sizeof(struct st_RawModel));
			*collision_model = model;
		} else {
			struct st_GeomBuf geombuf;			
			t_start = clock();
			
			//make_indexed(&model, &parts[i].geom);
			
			GeomBuf_Init(&geombuf, sizeof(struct st_RawVertex), 13107); 
			for(j = 0; j < model.tris_count; j++) {
				int result = GeomBuf_AddTri(&geombuf,
					&model.tris[j].v[0],
					&model.tris[j].v[1],
					&model.tris[j].v[2]
				);
				
				if(result == ERROR_NOMEM) {
					msgfunc->error("Out of memory! (AddTri)", NULL);
				} else if(result == ERROR_BUFFERISFULL) {
					int mtl_id = (int)model.tris[0].surface_tag;
					void *p = realloc(parts, (nparts+1)*sizeof(struct st_RGeomPart));
					parts = p;
					
					parts[nparts].material_id = mtl_id;
					parts[nparts].geom.indices = geombuf.tris;
					parts[nparts].geom.indices_count = geombuf.tris_count*3;
					parts[nparts].geom.verts = (struct st_RawVertex*)geombuf.verts;
					parts[nparts].geom.verts_count = geombuf.verts_count;
					
					/* clear */
					GeomBuf_Init(&geombuf, sizeof(struct st_RawVertex), 13107); 
					
					nparts++;
				}
			}
			
			/* tail */
			if(geombuf.tris_count) {
				int mtl_id = (int)model.tris[0].surface_tag;
				void *p = realloc(parts, (nparts+1)*sizeof(struct st_RGeomPart));
				parts = p;
				
				parts[nparts].material_id = mtl_id;
				parts[nparts].geom.indices = geombuf.tris;
				parts[nparts].geom.indices_count = geombuf.tris_count*3;
				parts[nparts].geom.verts = (struct st_RawVertex*)geombuf.verts;
				parts[nparts].geom.verts_count = geombuf.verts_count;
				
				nparts++;
			}
			
			t_end = clock();
			if(f_log) fprintf(f_log, "make_indexed took %u clocks\n", t_end-t_start);
		}
	}
	
	/* write all that shit into the file */
	switch(opt_collision) {
		case COLL_NONE: 
			collision_model = NULL; 
			//MessageBox(NULL, "COLL_NONE", NULL, MB_OK);
		break;
		case COLL_BYRENDERSURFACES: 
			collision_model = &rawmodel;
			//MessageBox(NULL, "COLL_BYRENDERSURFACES", NULL, MB_OK);
		break;
		case COLL_BYSURFACE: 
			/* dont touch */ 
			//MessageBox(NULL, "COLL_BYSURFACE", NULL, MB_OK);
		break;
	}
	
	if(collision_model) {
		int ret;
		
		t_start = clock();
		ret = create_collision(&rcollision, collision_model);
		t_end = clock();
		
		if(f_log) fprintf(f_log, "create_collision took %u clocks\n", t_end-t_start);
		
		switch(ret) {
			case COLL_OK: 
				rcollision_ptr = &rcollision; 
			break;
			case COLL_OUTOFMEM: 
				msgfunc->error("Can't create collision: out of memory", NULL);
			break;
			case COLL_TOOMANYPOINTS: 
				msgfunc->error("Can't create collision: too many points (cannot be >65535)", NULL);
			break;
		}
	} else {
		if(f_log) fprintf(f_log, "NO COLLISION\n");
	}
	
	/* check for vertex buffer limits */
#if 0
	for(i = 0; i < nparts; i++) {
		if(parts[i].geom.verts_count > 65535) {
			char str[768];
			
			sprintf(str, "Surface '%s' has too many verts (%d), model will be corrupted\n", 
				parts[i].surface_name, parts[i].geom.verts_count);
			msgfunc->error(str, NULL);
		}
	}
#endif
	
	t_start = clock();
	export_rgeom(filename, &mlist, nparts, parts, &rawmodel, rcollision_ptr);
	t_end = clock();
	
	if(f_log) fprintf(f_log, "export_rgeom took %u clocks\n", t_end-t_start);
	
	free_collision(&rcollision);
	
	/* free resources*/
	if(opt_collision == COLL_BYSURFACE)
		free(collision_model);
		
	for(i = 0; i < mlist.count; i++) {
		free(mlist.materials[i].name);
		free(mlist.materials[i].texture);
	}
	free(mlist.materials);
	
	for(i = 0; i < nparts; i++) {
		free(parts[i].geom.verts);
		free(parts[i].geom.indices);
	}
	free(parts);
	
	free(rawmodel.points);
	free(rawmodel.tris);
	free(objname);
	
	if(f_log) fclose(f_log);
		
	return AFUNC_OK;
}

ServerRecord _server_desc[] = {
//	{ LWMODCOMMAND_CLASS, "Test_ExportModel", test_ExportModel },
	{ LWMODCOMMAND_CLASS, "Test_ExportRGeom", test_ExportRGeom },
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
