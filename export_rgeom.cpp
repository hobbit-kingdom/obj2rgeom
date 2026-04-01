#include <stdio.h>
#include <stdlib.h>

#include "NvTriStrip.h"
#include "export.h"
#include "rgeom.h"
#include "vector.h"
#include "default_hmtl.h"

#include <windows.h> /* only for BOOL >_> */
#include "utils.h"

// meters to centimeters scale
const float M2CM = 1.f;

void RGeom_SaveToFILE(struct RGeom *rgeom, FILE *f)
{
	int i;
	
	fwrite(&rgeom->version, 1, sizeof(rgeom->version), f);
	fwrite(&rgeom->unknown1, 1, sizeof(rgeom->unknown1), f);
	fwrite(&rgeom->bbox, 1, sizeof(rgeom->bbox), f);
	
	fwrite(&rgeom->names_count, 1, sizeof(rgeom->names_count), f);
	fwrite(&rgeom->visuals_count, 1, sizeof(rgeom->visuals_count), f);
	fwrite(&rgeom->vertexbuffers_count, 1, sizeof(rgeom->vertexbuffers_count), f);
	fwrite(&rgeom->indexbuffers_count, 1, sizeof(rgeom->indexbuffers_count), f);
	fwrite(&rgeom->materials_count, 1, sizeof(rgeom->materials_count), f);
	fwrite(&rgeom->unknownthingy2_count, 1, sizeof(rgeom->unknownthingy2_count), f);
	fwrite(&rgeom->unknownthingy3_count, 1, sizeof(rgeom->unknownthingy3_count), f);
	fwrite(&rgeom->unknownthingy4_count, 1, sizeof(rgeom->unknownthingy4_count), f);
	
	/* write names */
	fwrite(rgeom->names, 1, sizeof(struct RGeom_Name) * rgeom->names_count, f);
	
	/* write visuals */
	for(i = 0; i < rgeom->visuals_count; i++) {
		struct RGeom_Visual *v = &rgeom->visuals[i];
		
		fwrite(&v->num_triangles, 1, sizeof(v->num_triangles), f);
		fwrite(&v->material_id, 1, sizeof(v->material_id), f);
		fwrite(&v->ib_count, 1, sizeof(v->ib_count), f);
		fwrite(&v->ib_id, 1, sizeof(v->ib_id), f);
		fwrite(&v->unknown5_count, 1, sizeof(v->unknown5_count), f);
		if(v->unknown5_count) {
			fwrite(v->unknown5, 1, v->unknown5_count * sizeof(short), f);
		}
		fwrite(&v->skin_indices_size, 1, sizeof(v->skin_indices_size), f);
		if(v->skin_indices_size) {
			fwrite(v->skin_indices, 1, v->skin_indices_size, f);
		}
		fwrite(&v->skin_vb_count, 1, sizeof(v->skin_vb_count), f);
		fwrite(&v->skin_vb_id, 1, sizeof(v->skin_vb_id), f);
	}
	
	/* write vertex buffers */
	for(i = 0; i < rgeom->vertexbuffers_count; i++) {
		struct RGeom_VertexBuffer *vb = &rgeom->vertexbuffers[i];
		
		fwrite(&vb->vertex_format, 1, sizeof(vb->vertex_format), f);
		fwrite(&vb->vertex_stride, 1, sizeof(vb->vertex_stride), f);
		fwrite(&vb->unknown2, 1, sizeof(vb->unknown2), f);
		fwrite(&vb->unknown3, 1, sizeof(vb->unknown3), f);
		fwrite(&vb->data_size, 1, sizeof(vb->data_size), f);
		fwrite(&vb->unknown4, 1, sizeof(vb->unknown4), f);
		if(vb->data_size) {
			fwrite(vb->data, 1, vb->data_size, f);
		}
	}
	
	/* write index buffers */
	for(i = 0; i < rgeom->indexbuffers_count; i++) {
		struct RGeom_IndexBuffer *ib = &rgeom->indexbuffers[i];
		
		fwrite(&ib->unknown1, 1, sizeof(ib->unknown1), f);
		fwrite(&ib->vertex_count, 1, sizeof(ib->vertex_count), f);
		fwrite(&ib->start_index, 1, sizeof(ib->start_index), f);
		fwrite(&ib->unknown3, 1, sizeof(ib->unknown3), f);
		fwrite(&ib->unknown4, 1, sizeof(ib->unknown4), f);
		fwrite(&ib->index_count, 1, sizeof(ib->index_count), f);
		fwrite(&ib->data_size, 1, sizeof(ib->data_size), f);
		fwrite(&ib->index_stride, 1, sizeof(ib->index_stride), f);
		fwrite(&ib->vb_id, 1, sizeof(ib->vb_id), f);
		fwrite(&ib->unknown5, 1, sizeof(ib->unknown5), f);
		fwrite(&ib->unknown6, 1, sizeof(ib->unknown6), f);
		if(ib->data_size) {
			fwrite(ib->data, 1, ib->data_size, f);
		}			
	}
	
	/* write materials */
	for(i = 0; i < rgeom->materials_count; i++) {
		struct RGeom_Material *m = &rgeom->materials[i];
		
		fwrite(&m->sign1, 1, sizeof(m->sign1), f);
		fwrite(&m->data, 1, sizeof(m->data), f);
		fwrite(&m->sign2, 1, sizeof(m->sign2), f);
		
		if(m->data.data1_count) {
			fwrite(m->data1, 1, m->data.data1_count * sizeof(int), f);
		}
		
		if(m->data.texture_count) {
			fwrite(m->texture_names, 1, m->data.texture_count * 256 * sizeof(char), f);
		}
	}
	
	/* */
	if(rgeom->unknownthingy2_count) {
		fwrite(rgeom->unknownthingy2, 1, rgeom->unknownthingy2_count, f);
	}
	
	/* */
	if(rgeom->unknownthingy3_count) {
		fwrite(rgeom->unknownthingy3, 1, rgeom->unknownthingy3_count * sizeof(RGeom_UnknownThingy3), f);
	}
	
	/* */
	if(rgeom->unknownthingy4_count) {
		fwrite(rgeom->unknownthingy3, 1, rgeom->unknownthingy4_count * sizeof(RGeom_UnknownThingy4), f);
	}
}

void RCollision_SaveToFILE(struct RCollision *c, FILE *f)
{
	fwrite(&c->version, 1, sizeof(c->version), f);
	fwrite(&c->unknown1, 1, sizeof(c->unknown1), f);
	fwrite(&c->unknown2, 1, sizeof(c->unknown2), f);
	
	fwrite(c->bbox, 1, sizeof(c->bbox), f);
	
	fwrite(c->points_add, 1, sizeof(c->points_add), f);
	fwrite(c->points_scale, 1, sizeof(c->points_scale), f);
	fwrite(c->bbox_size, 1, sizeof(c->bbox_size), f);
	
	fwrite(&c->unknown3, 1, sizeof(c->unknown3), f);
	fwrite(&c->unknown4, 1, sizeof(c->unknown4), f);
	fwrite(&c->unknown5, 1, sizeof(c->unknown5), f);
	fwrite(&c->unknown6, 1, sizeof(c->unknown6), f);
	
	fwrite(&c->tris_count, 1, sizeof(c->tris_count), f);
	fwrite(&c->unknown7_count, 1, sizeof(c->unknown7_count), f);
	fwrite(&c->points_count, 1, sizeof(c->points_count), f);
	fwrite(&c->data_size, 1, sizeof(c->data_size), f);
	
	fwrite(c->tris, 1, c->tris_count * ((c->points_count > 256) ? sizeof(short)*3 : sizeof(char)*3), f);
	fwrite(c->points, 1, c->points_count * sizeof(short) * 3, f);
	
	fwrite(&c->unknown7_count2, 1, sizeof(c->unknown7_count2), f);
	fwrite(c->unknown7, 1, c->unknown7_count * sizeof(short), f);
}

int add_point(float (*array)[3], int *count, float point[3])
{
	int i;
	for(i = 0; i < *count; i++)
		if(array[i][0] == point[0] && 
		   array[i][1] == point[1] && 
		   array[i][2] == point[2])
			return i;
			
	vcopy(array[i], point);
	(*count)++;
	return i;
}

C_API int create_collision(struct RCollision *c, struct st_RawModel *rawmodel)
{
	int i;
	
	float (*packed_points)[3];
	int points_count;
	
	int (*packed_tris)[3];
	int tris_count;
	
	/* remove unused and duplicate points */
	packed_points = (float(*)[3])malloc(sizeof(float)*3*rawmodel->points_count);
	packed_tris = (int(*)[3])malloc(sizeof(int)*3*rawmodel->tris_count);
	points_count = 0;
	tris_count = rawmodel->tris_count;
	
	if(!packed_points || !packed_tris) {
		free(packed_points);
		free(packed_tris);
		return COLL_OUTOFMEM;
	}	
	
	for(i = 0; i < rawmodel->tris_count; i++) {
		int p1 = rawmodel->tris[i].v[0].point;
		int p2 = rawmodel->tris[i].v[1].point;
		int p3 = rawmodel->tris[i].v[2].point;
		packed_tris[i][0] = add_point(packed_points, &points_count, rawmodel->points[p1]);
		packed_tris[i][1] = add_point(packed_points, &points_count, rawmodel->points[p2]);
		packed_tris[i][2] = add_point(packed_points, &points_count, rawmodel->points[p3]);
	}
	
	if(points_count > 65535) {
		free(packed_points);
		free(packed_tris);
		return COLL_TOOMANYPOINTS;
	}
	
	/* create collision */
	c->version = RCOLLISION_VERSION;
	c->unknown1 = 1;
	c->unknown2 = 0;
	
	vcopy(&c->bbox[0], packed_points[0]);
	vcopy(&c->bbox[3], packed_points[0]);
	for(i = 1; i < points_count; i++) {
		vmin(&c->bbox[0], packed_points[i]);
		vmax(&c->bbox[3], packed_points[i]);
	}
	
	for(i = 0; i < 6; i++)
		c->bbox[i] *= M2CM;
	
	vcopy(c->points_add, &c->bbox[0]);
	c->points_scale[0] = (1.f/65535.f) * (c->bbox[3] - c->bbox[0]);
	c->points_scale[1] = (1.f/65535.f) * (c->bbox[4] - c->bbox[1]);
	c->points_scale[2] = (1.f/65535.f) * (c->bbox[5] - c->bbox[2]);
	c->bbox_size[0] = (c->bbox[3] - c->bbox[0]);
	c->bbox_size[1] = (c->bbox[4] - c->bbox[1]);
	c->bbox_size[2] = (c->bbox[5] - c->bbox[2]);
	
	// these are different for PlaySurface's
	c->unknown3 = 1;
	c->unknown4 = 1;
	c->unknown5 = 1;
	c->unknown6 = 1;
	
	c->tris_count = tris_count;
	c->unknown7_count = tris_count;
	c->points_count = points_count;
	c->data_size = 
		c->points_count * sizeof(short) * 3 +
		c->tris_count * ((c->points_count > 256) ? sizeof(short)*3 : sizeof(char)*3) +
		c->unknown7_count * sizeof(short) +
		sizeof(int); // unknown7_count2
		
	if(c->points_count > 256) {
		unsigned short *tris = (unsigned short *)malloc(3 * sizeof(short) * c->tris_count);
		
		if(!tris) {
			free(packed_points);
			free(packed_tris);
			return COLL_OUTOFMEM;
		}
		
		for(i = 0; i < tris_count; i++) {
			tris[i*3+0] = packed_tris[i][0];
			tris[i*3+1] = packed_tris[i][1];
			tris[i*3+2] = packed_tris[i][2];
		}
		
		c->tris = tris;
	} else {
		unsigned char *tris = (unsigned char *)malloc(3 * sizeof(char) * c->tris_count);
		
		if(!tris) {
			free(packed_points);
			free(packed_tris);
			return COLL_OUTOFMEM;
		}		
		
		for(i = 0; i < tris_count; i++) {
			tris[i*3+0] = packed_tris[i][0];
			tris[i*3+1] = packed_tris[i][1];
			tris[i*3+2] = packed_tris[i][2];
		}
		
		c->tris = tris;
	}
	free(packed_tris);
	
	c->points = (unsigned short *)malloc(3 * sizeof(short) * c->points_count);
	if(!c->points) {
		free(c->tris);
		free(packed_points);
		return COLL_OUTOFMEM;
	}
	
	for(i = 0; i < c->points_count; i++) {
		float p[3];
		vcopy(p, packed_points[i]);
		
		c->points[i*3+0] = ((p[0]*M2CM - c->points_add[0]) / c->bbox_size[0]) * 65535.f;
		c->points[i*3+1] = ((p[1]*M2CM - c->points_add[1]) / c->bbox_size[1]) * 65535.f;
		c->points[i*3+2] = ((p[2]*M2CM - c->points_add[2]) / c->bbox_size[2]) * 65535.f;
	}
	free(packed_points);
	
	c->unknown7_count2 = c->unknown7_count;
	c->unknown7 = (unsigned short*)malloc(c->tris_count * sizeof(short));
	
	if(!c->unknown7) {
		free(c->tris);
		free(c->points);
		return COLL_OUTOFMEM;
	}
	
	for(i = 0; i < c->tris_count; i++) {
		c->unknown7[i] = c->tris_count - i - 1;
	}
	
	return COLL_OK;
}

C_API void free_collision(struct RCollision *c)
{
	free(c->points);
	free(c->tris);
	free(c->unknown7);
	
	c->points = NULL;
	c->tris = NULL;
	c->unknown7 = NULL;
}

struct MYVERTEX
{
	float x, y, z;
	float normal[3];
	float u, v;
};

struct MYVERTEX2
{
	float x, y, z;
	float normal[3];
	float u, v;
	float u2, v2;
};

C_API int export_rgeom(const char *filename, struct st_RGeomMtlList *mlist, unsigned nparts, struct st_RGeomPart *parts, struct st_RawModel *rawmodel, struct RCollision *collision)
{
	struct RGeom rgeom;
	struct RGeom_Name rname;
	struct RGeom_UnknownThingy3 unk3;
	
	short temp1[1] = { 0 };
	
	int i, j;
	
	int use_strips = 1;
	
	rgeom.version = RGEOM_VERSION;
	rgeom.unknown1 = 0;
	rgeom.names_count = 1;
	rgeom.visuals_count = nparts;
	rgeom.vertexbuffers_count = nparts*2;
	rgeom.indexbuffers_count = nparts;
	rgeom.materials_count = mlist->count;
	rgeom.unknownthingy2_count = 0;
	rgeom.unknownthingy3_count = 1;
	rgeom.unknownthingy4_count = 0;
	
	vcopy(&rgeom.bbox[0], rawmodel->points[0]);
	vcopy(&rgeom.bbox[3], rawmodel->points[0]);
	for(i = 1; i < rawmodel->points_count; i++) {
		vmin(&rgeom.bbox[0], rawmodel->points[i]);
		vmax(&rgeom.bbox[3], rawmodel->points[i]);
	}
	
	for(i = 0; i < 6; i++)
		rgeom.bbox[i] *= M2CM;
	
	/* names */
	rgeom.names = &rname;
	strcpy(rname.name, "Object");
	rname.count = nparts;
	rname.offset = 0;
	
	/* visuals */
	rgeom.visuals = new RGeom_Visual[nparts]; //malloc(sizeof(struct RGeom_Visual) * nparts);
	for(i = 0; i < nparts; i++) {
		struct RGeom_Visual *v = &rgeom.visuals[i];

		v->num_triangles = parts[i].geom.indices_count/3;
		v->material_id = parts[i].material_id;
		v->ib_count = 1;
		v->ib_id = i;
		v->unknown5_count = 1;
		v->unknown5 = temp1;
		v->skin_indices_size = 0;
		v->skin_indices = NULL;
		v->skin_vb_count = 0;
		v->skin_vb_id = 0;		
	}
	
	/* vertex buffers */
	rgeom.vertexbuffers = new RGeom_VertexBuffer[nparts*2]; //malloc(sizeof(struct RGeom_VertexBuffer) * nparts * 2);
	for(i = 0; i < nparts; i++) {
		struct RGeom_VertexBuffer *vb;
		unsigned *verts_colors;
		
		int two_textures = 0;
		
		/* vertices */
		vb = &rgeom.vertexbuffers[i*2+0];
		
		/*
			1 texture = 0x000000001B17001Bull;
			2 texture = 0x000000001B27001Bull;
		*/
		vb->vertex_format = two_textures ? 0x000000001B27001Bull
		                                 : 0x000000001B17001Bull;
		vb->vertex_stride = two_textures ? sizeof(struct MYVERTEX2)
		                                 : sizeof(struct MYVERTEX);
		vb->unknown2 = 1;
		vb->unknown3 = 0;
		vb->data_size = parts[i].geom.verts_count * vb->vertex_stride;
		vb->unknown4 = 0;
		
		if(two_textures) {
			struct MYVERTEX2 *verts = (struct MYVERTEX2 *)malloc(parts[i].geom.verts_count * vb->vertex_stride);
			for(j = 0; j < parts[i].geom.verts_count; j++) {
				vcopy(&verts[j].x, rawmodel->points[parts[i].geom.verts[j].point]);
				verts[j].x *= M2CM;
				verts[j].y *= M2CM;
				verts[j].z *= M2CM;
				vcopy(verts[j].normal, parts[i].geom.verts[j].normal);
				verts[j].u = parts[i].geom.verts[j].uv[0][0];
				verts[j].v = parts[i].geom.verts[j].uv[0][1];
				verts[j].u2 = parts[i].geom.verts[j].uv[1][0];
				verts[j].v2 = parts[i].geom.verts[j].uv[1][1];
			}		
			vb->data = (char*)verts;
		} else {
			struct MYVERTEX *verts = (struct MYVERTEX *)malloc(parts[i].geom.verts_count * vb->vertex_stride);
			for(j = 0; j < parts[i].geom.verts_count; j++) {
				vcopy(&verts[j].x, rawmodel->points[parts[i].geom.verts[j].point]);
				verts[j].x *= M2CM;
				verts[j].y *= M2CM;
				verts[j].z *= M2CM;
				vcopy(verts[j].normal, parts[i].geom.verts[j].normal);
				verts[j].u = parts[i].geom.verts[j].uv[0][0];
				verts[j].v = parts[i].geom.verts[j].uv[0][1];
			}			
			vb->data = (char*)verts;
		}
		
		/* colors/mask */
		vb = &rgeom.vertexbuffers[i*2+1];
		vb->vertex_format = 0x0000000000001C00ull;
		vb->vertex_stride = sizeof(unsigned); // DWORD
		vb->unknown2 = 1;
		vb->unknown3 = 0;
		vb->data_size = parts[i].geom.verts_count * vb->vertex_stride;
		vb->unknown4 = 0;
		
		verts_colors = (unsigned*)malloc(parts[i].geom.verts_count * vb->vertex_stride);
		for(j = 0; j < parts[i].geom.verts_count; j++) {
			int r = parts[i].geom.verts[j].color_rgba[0] * 255.f;
			int g = parts[i].geom.verts[j].color_rgba[1] * 255.f;
			int b = parts[i].geom.verts[j].color_rgba[2] * 255.f;
			int a = parts[i].geom.verts[j].color_rgba[3] * 255.f;
			
			if(r < 0) r = 0; if(r > 255) r = 255;
			if(g < 0) g = 0; if(g > 255) g = 255;
			if(b < 0) b = 0; if(b > 255) b = 255;
			if(a < 0) a = 0; if(a > 255) a = 255;
			
			verts_colors[j] = r | (g << 8) | (b << 16) | (a << 24);
		}
		
		vb->data = (char*)verts_colors;
	}
	
	/* index buffers */
	rgeom.indexbuffers = new RGeom_IndexBuffer[nparts]; //malloc(sizeof(struct RGeom_IndexBuffer) * nparts);
	for(i = 0; i < nparts; i++) {
		struct RGeom_IndexBuffer *ib = &rgeom.indexbuffers[i];
		
		unsigned short *tmp_indices;
		struct PrimitiveGroup *prim;
		unsigned short n_prim;
		
		tmp_indices = (unsigned short*)malloc(parts[i].geom.indices_count * sizeof(short));
		for(j = 0; j < parts[i].geom.indices_count; j++)
			tmp_indices[j] = parts[i].geom.indices[j];
			
		if(use_strips) {
			/* generate strips */
			SetStitchStrips(true);
			GenerateStrips(tmp_indices, parts[i].geom.indices_count, &prim, &n_prim);
			
			free(tmp_indices);
			
			/* I hate this unnecessary copying but.. bruh */
			ib->data = (char*)malloc(prim[0].numIndices*sizeof(short));
			memcpy(ib->data, prim[0].indices, prim[0].numIndices*sizeof(short));
		} else {
			ib->data = (char*)tmp_indices;
		}
		
		/**/
		ib->unknown1 = use_strips ? 1 : 3;
		ib->vertex_count = parts[i].geom.verts_count;
		ib->start_index = 0;
		ib->unknown3 = 0;
		ib->unknown4 = 16;
		ib->index_count = use_strips ? prim[0].numIndices : parts[i].geom.indices_count;
		ib->data_size = ib->index_count * sizeof(short);
		ib->index_stride = 2;
		ib->vb_id = i*2;
		ib->unknown5 = 0;
		ib->unknown6 = 0;
		
		/* free resources */
		if(use_strips) {
			delete [] prim;
		}
	}
	
	/* materials */
	rgeom.materials = new RGeom_Material[mlist->count];
	for(i = 0; i < mlist->count; i++) {
		struct RGeom_Material *material = &rgeom.materials[i];
		
		/*
		const char *default_tex_names[3] = {
			"Lvl_Htown_woodFront[D].xbmp",
			"LVL_HTOWN_STONEWALLB[D].XBMP",
			"LVL_HTOWN_WOOD_PLANK[D].XBMP"
		};
		*/
		
		const char *default_tex_names[3] = {
			"DEFAULTTEXTURE[D].XBMP",
			"DEFAULTTEXTURE[D].XBMP",
			"DEFAULTTEXTURE[D].XBMP"
		};
		
		FILE *f_mtl;
		
		f_mtl = fopen("C:\\Temp\\default.hmtl", "rb");
		if(f_mtl) {
			fread(&material->data, 1, sizeof(material->data), f_mtl);
			fclose(f_mtl);
		} else {
			memcpy(&material->data, default_hmtl, sizeof(material->data)); 
		}
		
		material->sign1 = 0x3EC;
		material->sign2 = 0x3EC;
		material->data1 = NULL;
		
		material->data.data1_count = 0;
		material->data.texture_count = 1;
		
		material->texture_names = (char(*)[256])calloc(material->data.texture_count, 256);
		
		if(mlist->materials[i].texture) {
			extract_filename(material->texture_names[0], mlist->materials[i].texture);
			
			char *ext = (char*)get_ext(material->texture_names[0]);
			if(ext) 
				strcpy(ext, ".xbmp");
			else    
				strcat(material->texture_names[0], ".xbmp");
		} else {
			strcpy(material->texture_names[0], default_tex_names[i%3]);
		}
			
		if(mlist->materials[i].name)
			strcpy(material->data.material_name, mlist->materials[i].name);
	}
	
	/* unknown thingy */
	rgeom.unknownthingy3 = &unk3;
	unk3.parent_id = 0xFFFFFFFF;
	unk3.num_childs = 0;
	vset3(unk3.s, 1.f, 1.f, 1.f);
	vset4(unk3.q, 0.f, 0.f, 0.f, -1.f);
	vset3(unk3.t, 0.f, 0.f, 0.f);
	
	FILE *f = fopen(filename, "wb");
	if(!f) {
		return 1;
	}
	
	RGeom_SaveToFILE(&rgeom, f);
	if(collision)
		RCollision_SaveToFILE(collision, f);
	
	fclose(f);
	
	/* free resources */
	for(i = 0; i < rgeom.vertexbuffers_count; i++) {
		free(rgeom.vertexbuffers[i].data);
	}
	for(i = 0; i < rgeom.indexbuffers_count; i++) {
		free(rgeom.indexbuffers[i].data);
	}
	for(i = 0; i < rgeom.materials_count; i++) {
		free(rgeom.materials[i].texture_names);
	}
	delete [] rgeom.visuals;
	delete [] rgeom.vertexbuffers;
	delete [] rgeom.indexbuffers;
	delete [] rgeom.materials;
	
	return 0;
}

