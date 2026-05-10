#ifndef __RGEOM_H__
#define __RGEOM_H__

#include <stdio.h>

#define RGEOM_VERSION_OLD_7D0 0x000007D0L
#define RGEOM_VERSION 0x000007D1L

#define RGEOM_KIND_RIGID        0
#define RGEOM_KIND_SKINNED_PC   1
#define RGEOM_KIND_SKINNED_XBOX 8

#define RGEOM_MATERIAL_SIGN         0x3EC
#define RGEOM_MATERIAL_SIGN_OLD_3EA 0x3EA

struct RGeom_Name
{
	char name[16];
	int count;
	int offset;
};

struct RGeom_Visual
{
	int num_triangles;     // count of triangles without degenerate ones
	                       // i.e. as if model was converted to D3DPT_TRIANGLELIST
	int material_id;
	int ib_count;          // index buffer count (IF NOT GCN SKIN)
	int ib_id;             // index buffer id
	int unknown5_count;
	short *unknown5;
	int skin_indices_size;
	char *skin_indices;
	int skin_vb_count;
	int skin_vb_id; 
};

struct RGeom_VertexBuffer
{
	__int64 vertex_format;  // vertex format
	int vertex_stride;
	int unknown2;
	int unknown3;
	int data_size;
	int unknown4;
	
	char *data;
};

struct RGeom_IndexBuffer
{
	int unknown1;    // if equals to 3 primitive type is triangle list, otherwise (usually 1) it's triangle strip
	int vertex_count;
	int start_index;
	int unknown3;
	int unknown4;
	int index_count;
	int data_size;
	int index_stride; // not stride, but vertex buffer count ?
	int vb_id;        // vertex-buffer id
	int remap_count;
	int remap_start;
	
	char *data;	
};

struct RGeom_MaterialConstantRecord
{
	float flt_unknown1; // = 255.f
	float flt_unknown2; // = 255.f
	float flt_unknown3; // = 255.f
	float flt_unknown4; // = 0.f, not sure if it is float indeed
	int unknown1; // 0x0000ffff - frame count (in data1 buffer)
	              // 0xffff0000 - ?
	short unknown2[2]; // usually 0x0000FFFF
	short unknown3[2]; // looks like animparams
};

struct RGeom_MaterialTextureRecord // 120 (78h) bytes
{
	int tc_index;       // texcoord index of this texture
	int wrap_u;         // 0 = D3DTADDRESS_WRAP, 1 = D3DTADDRESS_CLAMP, 2 = D3DTADDRESS_MIRROR
	int wrap_v;
	int texfilter;      // 1 - D3DTEXF_LINEAR, 2 - D3DTEXF_POINT
	char unknown3[16];  // string? often contains trash
	short uvframestart;
	short uvframeend;   // UV frame count div 2, something related to animation
	short unknown4[2];
	int animparams;     // 0x000000FF - component count ?, 
	                    // 0x00FF0000 - speed (frames per second I guess)
	                    // 0x0000FFFF - ?
	int   unk_maxint1;  // usually 0x7FFFFFFF
	short unknown5[2];
	short unknown6[2];
	char  unknown7[64]; // string? usually contains trash
};

struct RGeom_MaterialData
{
	char material_name[256];
	int unknown1; // = -1
	int unknown2; // combine type:
	              // 1 = opaque, 1 texture
	              // 2 = transparent, 1 or 2 texture
	              // 3 = mix by mask, 3 texture
	              // 4 = mix by vertex colors, 2 texture
	              
	int unknown3; // 
	int unknown4; // usually equals to 2
	int unknown5; // additive blend for combine type 1
	int unknown6; // tint type, according to Area51 sources
	              // color tint works for combine type 1, but doesn't for 12...
	int unknown7; // bitmask
	              // 0x01 - disable culling ( two sided surface )
	              // 0x02 - enable alpha test for combine type 2, otherwise alpha blend is used
	              // 0x04 - ???
	              // 0x20 - ???

	struct RGeom_MaterialConstantRecord constants[10];
	// first constant is tint color
	// second constant is tint alpha
	
	// for combine type 9:
	// constant 3 controls procedural texcoord animation speed, only U channel working
	// for combine type 11:
	// constant 3 controls procedural texcoord animation speed, both U and V channels is working
	
	int ntexused; // count of texture records that are indeed used
	struct RGeom_MaterialTextureRecord tex[16];
	
	int unknown8[6];    // ?
	
	int data1_ptr;      // 32-Bit pointer
	int data1_count;
	int unknown17;
	int texture_count;
	
	int unknown9[9];    // ?
	
}; // 2564 bytes

struct RGeom_Material
{
	int sign1; // = 0x3EC;
	struct RGeom_MaterialData data;
	int sign2; // = 0x3EC;
	
	int *data1; // UV animation (floats)
	char (*texture_names)[256];
};

struct RGeom_Joint
{
	int parent_id;  /* -1 if root */
	int num_childs;
	float s[3];     /* scale */
	float q[4];     /* quaternion */
	float t[3];     /* translate */
};

struct RGeom_JointRemap
{
	short from;
	short to;
};

#define MODEL_KIND_RIGID    0
#define MODEL_KIND_NPC      1
#define MODEL_KIND_NPC_GCN  2
#define MODEL_KIND_PS2      4
#define MODEL_KIND_NPC_XBOX 8

struct RGeom
{
	int big_endian;

	int version;
	int kind;     // 0 - PC or XBox or GameCube rigid model (.rgeom) 
	              // 1 - PC npc model (.npcgeom)
	              // 2 - GameCube npc model (.npcgeom, big-endian)
	              // 4 - PS2 npc or rigid model (.npcgeom, .rgeom)
	              // 8 - XBox npc model (.npcgeom)
	
	float bbox[6];
	
	int names_count;
	struct RGeom_Name *names;
	
	int visuals_count;
	struct RGeom_Visual *visuals;
	
	int vertexbuffers_count;
	struct RGeom_VertexBuffer *vertexbuffers;
	
	int indexbuffers_count;
	struct RGeom_IndexBuffer *indexbuffers;
	
	int materials_count;
	struct RGeom_Material *materials;
	
	int unknownthingy2_count;
	char *unknownthingy2;
	
	int joints_count;
	struct RGeom_Joint *joints;
	
	// used on XBox, but not on PC
	int remap_count;
	struct RGeom_JointRemap *remaps;
};

struct RCollision;

#ifdef __cplusplus
extern "C" {
#endif

int RGeom_Load(struct RGeom *rgeom, struct RCollision *collision, const char *filename);
void RGeom_Unload(struct RGeom *rgeom);
int RGeom_SaveOBJ(struct RGeom *rgeom, const char *filename, int name_mtl_after_tex);

struct RTree
{
	int points_count;
	int unknown;
	float sprite_points[4][3];
	float flt_unknown[22];
	
	float (*points)[3];
	
	struct RGeom_Material material;
};

int RTree_Load(struct RTree *rtree, const char *filename);
void RTree_Unload(struct RTree *rtree);
int RTree_Save(struct RTree *rtree, const char *filename);

void RGeom_SaveToFILE(struct RGeom *rgeom, FILE *f, int big_endian);

#define RCOLLISION_VERSION 0x000007D2L

struct RCollision
{
	int version;
	int unknown1;
	int unknown2;
	
	float bbox[6];
	
	float points_add[3];
	float points_scale[3];
	float bbox_size[3];
	
	int unknown3;
	int unknown4;
	int unknown5;
	int unknown6;
	
	int tris_count;
	int unknown7_count;
	int points_count;
	int data_size;
	
	void *tris; // if points_count > 255 it's 8-bit indices, else 16-bit
	unsigned short *points;
	
	int unknown7_count2;
	unsigned short *unknown7;
};

int RCollision_SaveOBJ(struct RCollision *coll, const char *filename);
void RCollision_Unload(struct RCollision *collision);

void RCollision_SaveToFILE(struct RCollision *c, FILE *f, int big_endian);

//
void clear_material(struct RGeom_Material *mtl);
void read_material(struct RGeom_Material *mtl, FILE *fp, int big_endian);
void save_material(struct RGeom_Material *m, FILE *fp, int big_endian);

#ifdef __cplusplus
}
#endif

#endif

