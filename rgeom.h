#ifndef __RGEOM_H__
#define __RGEOM_H__

#define RGEOM_VERSION_OLD_7D0 0x000007D0L
#define RGEOM_VERSION 0x000007D1L

#define RGEOM_KIND_RIGID        0
#define RGEOM_KIND_SKINNED_PC   1
#define RGEOM_KIND_SKINNED_XBOX 8

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
	int ib_count;          // index buffer count
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
	int unknown5;
	int unknown6;
	
	char *data;	
};

struct RGeom_MaterialConstantRecord
{
	float flt_unknown1; // = 255.f
	float flt_unknown2; // = 255.f
	float flt_unknown3; // = 255.f
	float flt_unknown4; // = 0.f, not sure if it is float indeed
	int unknown1;
	int unknown2; // usually 0x0000FFFF
	int unknown3; // looks like animparams
};

struct RGeom_MaterialTextureRecord // 120 (78h) bytes
{
	int tc_index;       // texcoord index of this texture
	int unknown1;
	int unknown2;
	int texfilter;      // 1 - D3DTEXF_LINEAR, 2 - D3DTEXF_POINT
	char unknown3[16];  // string? often contains trash
	int uvframecount;   // UV frame count div 2, something related to animation
	int unknown4;
	int animparams;     // 0xFF - unknown (if 2 animation is smooth, if 3 game crashes, if 1 weird behavior), 
	                    // 0x00FFFFFF - speed (frames per second I guess)
	int  unk_maxint1;   // usually 0x7FFFFFFF
	int  unknown5;
	int  unknown6;
	char unknown7[64];  // string? usually contains trash
};

struct RGeom_MaterialData
{
	char material_name[256];
	int unknown1; // = -1
	int unknown2; // if equals to 2, then alpha test is enabled
	int unknown3; 
	int unknown4;
	int unknown5;
	int unknown6;
	int unknown7; // bitmask
	              // 0x01 - disable culling ( two sided surface )
	              // 0x02 - enable alpha test ( only test, blending is set somewhere else )
	              // 0x04 - ???
	              // 0x20 - ???

	struct RGeom_MaterialConstantRecord constants[10];
	
	int ntexused; // count of texture records that are indeed used
	struct RGeom_MaterialTextureRecord tex[16];
	
	char pad1[24];    // ?
	int data1_ptr;      // 32-Bit pointer
	int data1_count;
	int unknown17;
	int texture_count;
	char pad2[36]; // ?
}; // 2564 bytes

struct RGeom_Material
{
	int sign1; // = 0x3EC;
	struct RGeom_MaterialData data;
	int sign2; // = 0x3EC;
	
	int *data1; // UV animation (floats)
	char (*texture_names)[256];
};

struct RGeom_UnknownThingy3 // skeleton ??
{
	int parent_id;  /* -1 if root */
	int num_childs;
	float s[3];     /* scale */
	float q[4];     /* quaternion */
	float t[3];     /* translate */
};

struct RGeom_UnknownThingy4
{
	short unknown1;
	short unknown2;
};

struct RGeom
{
	int version;
	int unknown1; // 0 - PC or XBox or GameCube rigid model (.rgeom) 
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
	
	int unknownthingy3_count;
	struct RGeom_UnknownThingy3 *unknownthingy3;
	
	int unknownthingy4_count;
	struct RGeom_UnknownThingy4 *unknownthingy4;
};

int RGeom_Load(struct RGeom *rgeom, const char *filename);
void RGeom_Unload(struct RGeom *rgeom);
int RGeom_SaveOBJ(struct RGeom *rgeom, const char *filename);

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

#endif

