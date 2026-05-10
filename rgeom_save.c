#include <stdio.h>
#include "endianess.h"
#include "rgeom.h"

static inline void write_int(FILE *fp, int val, int big_endian)
{
	if(big_endian) val = BEtoNl(val);
	fwrite(&val, 1, sizeof(val), fp);
}

static inline void write_longlong(FILE *fp, long long val, int big_endian)
{
	if(big_endian) val = BEtoNll(val);
	fwrite(&val, 1, sizeof(val), fp);
}

static inline void write_short(FILE *fp, short val, int big_endian)
{
	if(big_endian) val = BEtoNs(val);
	fwrite(&val, 1, sizeof(val), fp);
}

static inline void write_float(FILE *fp, float val, int big_endian)
{
	write_int(fp, *(int*)&val, big_endian);
}

static void save_material_data(struct RGeom_MaterialData *m, FILE *fp, int big_endian)
{
	const int be = big_endian;
	int i;
	
	fwrite(m->material_name, 1, sizeof(m->material_name), fp);
	write_int(fp, m->unknown1, be);
	write_int(fp, m->unknown2, be);
	write_int(fp, m->unknown3, be);
	write_int(fp, m->unknown4, be);
	write_int(fp, m->unknown5, be);
	write_int(fp, m->unknown6, be);
	write_int(fp, m->unknown7, be);
	
	for(i = 0; i < 10; i++) {
		write_float(fp, m->constants[i].flt_unknown1, be);
		write_float(fp, m->constants[i].flt_unknown2, be);
		write_float(fp, m->constants[i].flt_unknown3, be);
		write_float(fp, m->constants[i].flt_unknown4, be);
		
		write_int(fp, m->constants[i].unknown1, be);
		
		write_short(fp, m->constants[i].unknown2[0], be);
		write_short(fp, m->constants[i].unknown2[1], be);
		write_short(fp, m->constants[i].unknown3[0], be);
		write_short(fp, m->constants[i].unknown3[1], be);
	}
	
	write_int(fp, m->ntexused, be);
	
	/* texture records */
	/* these are a bit weird */
	for(i = 0; i < 16; i++) {
		write_int(fp, m->tex[i].tc_index, 0);    /* write as is */
		write_int(fp, m->tex[i].wrap_u, be);     /* endianess matter, I GUESS */
		write_int(fp, m->tex[i].wrap_v, be);     /* endianess matter, I GUESS */
		write_int(fp, m->tex[i].texfilter, be);  /* endianess matter */
		
		/* garbage string 1 */
		fwrite(m->tex[i].unknown3, 1, sizeof(m->tex[i].unknown3), fp);
		
		write_short(fp, m->tex[i].uvframestart, be); /* ??? */
		write_short(fp, m->tex[i].uvframeend,   be); /* endianess matter */
		
		write_short(fp, m->tex[i].unknown4[0], be);  /* ??? */
		write_short(fp, m->tex[i].unknown4[1], be);  /* ??? */
		
		write_int(fp, m->tex[i].animparams, 0);      /* write as is */
		write_int(fp, m->tex[i].unk_maxint1, 0);     /* endianess matter */
		
		write_short(fp, m->tex[i].unknown5[0], be);  /* ??? */
		write_short(fp, m->tex[i].unknown5[1], be);  /* ??? */
		write_short(fp, m->tex[i].unknown6[0], be);  /* ??? */
		write_short(fp, m->tex[i].unknown6[1], be);  /* ??? */

		/* garbage string 2 */
		fwrite(m->tex[i].unknown7, 1, sizeof(m->tex[i].unknown7), fp);		
	}
	
	/* Garbage area1 */
	fwrite(m->unknown8, 1, sizeof(m->unknown8), fp);
	
	/* Garbage pointer to UV animation, initialized later by the game */
	write_int(fp, m->data1_ptr, be);
	
	/* Size of UV animation, valid */
	write_int(fp, m->data1_count, be);
	
	/* ?? */
	write_int(fp, m->unknown17, be);
	
	/* Count of texture names, valid */
	write_int(fp, m->texture_count, be);
	
	/* Garbage area2 */
	fwrite(m->unknown9, 1, sizeof(m->unknown9), fp);			
}

void save_material(struct RGeom_Material *m, FILE *fp, int big_endian)
{
	const int be = big_endian;
	int tex_cnt;
	
	write_int(fp, m->sign1, be);
	save_material_data(&m->data, fp, big_endian);
	write_int(fp, m->sign2, be);
	
	if(m->data.data1_count) {
		fwrite(m->data1, 1, m->data.data1_count * sizeof(int), fp);
	}
	
	tex_cnt = m->data.texture_count;
	if(tex_cnt) {
		fwrite(m->texture_names, 1, tex_cnt * 256 * sizeof(char), fp);
	}
}

void RGeom_SaveToFILE(struct RGeom *rgeom, FILE *f, int big_endian)
{
	int i, j;
	const int be = big_endian;
	
	write_int(f, rgeom->version, be);
	write_int(f, rgeom->kind, be);
	for(i = 0; i < 6; i++)
		write_float(f, rgeom->bbox[i], be);
		
	write_int(f, rgeom->names_count, be);
	write_int(f, rgeom->visuals_count, be);
	write_int(f, rgeom->vertexbuffers_count, be);
	write_int(f, rgeom->indexbuffers_count, be);
	write_int(f, rgeom->materials_count, be);
	write_int(f, rgeom->unknownthingy2_count, be);
	write_int(f, rgeom->joints_count, be);
	write_int(f, rgeom->remap_count, be);
	
	/* write names */
	for(i = 0; i < rgeom->names_count; i++) {
		fwrite(rgeom->names[i].name, 1, sizeof(rgeom->names[i].name), f);
		write_int(f, rgeom->names[i].count, be);
		write_int(f, rgeom->names[i].offset, be);
	}
	
	/* write visuals */
	for(i = 0; i < rgeom->visuals_count; i++) {
		struct RGeom_Visual *v = &rgeom->visuals[i];
		
		write_int(f, v->num_triangles, be);
		write_int(f, v->material_id, be);
		write_int(f, v->ib_count, be);
		write_int(f, v->ib_id, be);	
		
		write_int(f, v->unknown5_count, be);	
		for(j = 0; j < 	v->unknown5_count; j++) {
			write_short(f, v->unknown5[j], be);
		}
		
		write_int(f, v->skin_indices_size, be);	
		if(v->skin_indices_size) {
			fwrite(v->skin_indices, 1, v->skin_indices_size, f);
		}
		
		write_int(f, v->skin_vb_count, be);
		write_int(f, v->skin_vb_id, be);
	}
	
	/* write vertex buffers */
	for(i = 0; i < rgeom->vertexbuffers_count; i++) {
		struct RGeom_VertexBuffer *vb = &rgeom->vertexbuffers[i];
		
		write_longlong(f, vb->vertex_format, be);
		write_int(f, vb->vertex_stride, be);
		write_int(f, vb->unknown2, be);
		write_int(f, vb->unknown3, be);
		write_int(f, vb->data_size, be);
		write_int(f, vb->unknown4, be);
		if(vb->data_size) {
			fwrite(vb->data, 1, vb->data_size, f);
		}
	}
	
	/* write index buffers */
	for(i = 0; i < rgeom->indexbuffers_count; i++) {
		struct RGeom_IndexBuffer *ib = &rgeom->indexbuffers[i];
		
		write_int(f, ib->unknown1, be);
		write_int(f, ib->vertex_count, be);
		write_int(f, ib->start_index, be);
		write_int(f, ib->unknown3, be);
		write_int(f, ib->unknown4, be);
		write_int(f, ib->index_count, be);
		write_int(f, ib->data_size, be);
		write_int(f, ib->index_stride, be);
		write_int(f, ib->vb_id, be);
		write_int(f, ib->remap_count, be);
		write_int(f, ib->remap_start, be);
		if(ib->data_size) {
			fwrite(ib->data, 1, ib->data_size, f);
		}			
	}
	
	/* write materials */
	for(i = 0; i < rgeom->materials_count; i++) {
		struct RGeom_Material *m = &rgeom->materials[i];
		save_material(m, f, be);
	}
	
	/* */
	if(rgeom->unknownthingy2_count) {
		fwrite(rgeom->unknownthingy2, 1, rgeom->unknownthingy2_count, f);
	}
	
	/* write joints */
	for(i = 0; i < rgeom->joints_count; i++) {
		write_int(f, rgeom->joints[i].parent_id, be);
		write_int(f, rgeom->joints[i].num_childs, be);
		
		write_float(f, rgeom->joints[i].s[0], be);
		write_float(f, rgeom->joints[i].s[1], be);
		write_float(f, rgeom->joints[i].s[2], be);
		
		write_float(f, rgeom->joints[i].q[0], be);
		write_float(f, rgeom->joints[i].q[1], be);
		write_float(f, rgeom->joints[i].q[2], be);
		write_float(f, rgeom->joints[i].q[3], be);
		
		write_float(f, rgeom->joints[i].t[0], be);
		write_float(f, rgeom->joints[i].t[1], be);
		write_float(f, rgeom->joints[i].t[2], be);
	}
	
	/* write remaps */
	for(i = 0; i < rgeom->remap_count; i++) {
		write_short(f, rgeom->remaps[i].from, be);
		write_short(f, rgeom->remaps[i].to, be);
	}
}

void RCollision_SaveToFILE(struct RCollision *c, FILE *f, int big_endian)
{
	int i;
	int be = big_endian;
	
	write_int(f, c->version, be);
	write_int(f, c->unknown1, be);
	write_int(f, c->unknown2, be);
	
	for(i = 0; i < 6; i++)
		write_float(f, c->bbox[i], be);
	for(i = 0; i < 3; i++)
		write_float(f, c->points_add[i], be);
	for(i = 0; i < 3; i++)
		write_float(f, c->points_scale[i], be);
	for(i = 0; i < 3; i++)
		write_float(f, c->bbox_size[i], be);
	
	write_int(f, c->unknown3, be);
	write_int(f, c->unknown4, be);
	write_int(f, c->unknown5, be);
	write_int(f, c->unknown6, be);
	
	write_int(f, c->tris_count, be);
	write_int(f, c->unknown7_count, be);
	write_int(f, c->points_count, be);
	write_int(f, c->data_size, be);
	
	if(c->points_count > 256) {
		if(be) {
			short *pTris = (short*)c->tris;
			for(i = 0; i < c->tris_count*3; i++)
				write_short(f, pTris[i], be);
		} else {
			fwrite(c->tris, 1, c->tris_count*sizeof(short)*3, f);
		}
	} else {
		fwrite(c->tris, 1, c->tris_count*sizeof(char)*3, f);
	}
	
	if(be) {
		short *pPoints = (short*)c->points;
		for(i = 0; i < c->points_count*3; i++)
			write_short(f, pPoints[i], be);
	} else {
		fwrite(c->points, 1, c->points_count * sizeof(short) * 3, f);
	}
	
	write_int(f, c->unknown7_count2, be);
	if(be) {
		short *pUnk7 = (short*)c->unknown7;
		for(i = 0; i < c->unknown7_count; i++)
			write_short(f, pUnk7[i], be);
	} else {
		fwrite(c->unknown7, 1, c->unknown7_count * sizeof(short), f);
	}
}
