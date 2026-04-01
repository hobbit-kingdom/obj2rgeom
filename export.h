#ifndef __EXPORT_H__
#define __EXPORT_H__

#ifdef __cplusplus
#define C_API extern "C"
#else
#define C_API
#endif

struct st_RawVertex
{
	int point;
	float normal[3];
	float uv[4][4];
	float color_rgba[4];
};

struct st_RawTriangle
{
	struct st_RawVertex v[3];
	const void *surface_tag;
};

struct st_RawModel
{
	int points_count;
	float (*points)[3];
	
	int tris_count;
	struct st_RawTriangle *tris;
};

struct st_PackedModel
{
	int verts_count;
	struct st_RawVertex *verts;
	
	int indices_count;
	int *indices;
};

int merge_rawmodel(struct st_RawModel *dest, struct st_RawModel *src);

struct st_RGeomPart
{
	int material_id;
	struct st_PackedModel geom;
};

struct st_RGeomMtlList
{
	int count;
	struct st_RGeomMtl {
		char *name;
		char *texture;
		float sm_angle;
		float sm_angle_cos;
	} *materials;
};

struct RCollision;

#define COLL_OK 0
#define COLL_OUTOFMEM 1
#define COLL_TOOMANYPOINTS 2

C_API int create_collision(struct RCollision *c, struct st_RawModel *model);
C_API void free_collision(struct RCollision *c);

C_API int export_rgeom(const char *filename, struct st_RGeomMtlList *mlist, unsigned nparts, struct st_RGeomPart *parts, struct st_RawModel *rawmodel, struct RCollision *collision);

#endif

