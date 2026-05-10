#ifndef __RGEOM_UTILS_H__
#define __RGEOM_UTILS_H__

struct PackedNormalXBox
{
	int x : 11;
	int y : 11;
	int z : 10;
};

static __inline void unpack_xbox_normal(float n[3], struct PackedNormalXBox np)
{
	n[0] = np.x / 1023.f;
	n[1] = np.y / 1023.f;
	n[2] = np.z / 511.f;
}

// 1-texture gamecube vertex
struct GCNVertex1T
{
	unsigned short p;  // index of point in points buffer
	unsigned short n;  // presumably packed normal
	unsigned short c;  // index of color in VertColors buffer
	unsigned short t;  // index of texcoord in texcoords buffer 
};

// 2-texture gamecube vertex
struct __attribute__((packed)) GCNVertex2T
{
	unsigned short p;  // index of point in points buffer
	unsigned short n;  // presumably packed normal
	unsigned short c;  // index of color in VertColors buffer
	unsigned short t;  // index of texcoord in texcoords buffer 
	unsigned short t2;  // index of second texcoord in texcoords buffer 
};

// 3-texture gamecube vertex
struct __attribute__((packed))  GCNVertex3T
{
	unsigned short p;  // index of point in points buffer
	unsigned short n;  // presumably packed normal
	unsigned short c;  // index of color in VertColors buffer
	unsigned short t;  // index of texcoord in texcoords buffer 
	unsigned short t2;  // index of second texcoord in texcoords buffer 
	unsigned short t3;  // index of second texcoord in texcoords buffer 
};

// 4-texture gamecube vertex
struct __attribute__((packed)) GCNVertex4T
{
	unsigned short p;  // index of point in points buffer
	unsigned short n;  // presumably packed normal
	unsigned short c;  // index of color in VertColors buffer
	unsigned short t;  // index of texcoord in texcoords buffer 
	unsigned short t2;  // index of second texcoord in texcoords buffer 
	unsigned short t3;  // index of second texcoord in texcoords buffer 
	unsigned short t4;  // index of second texcoord in texcoords buffer 
};

// 0x000000121B17001B
struct VertSkinnedXBox
{
	float x, y, z;
	float bone; // bone index multiplied by 3
	struct PackedNormalXBox normal;
	float u, v;
};

// 0x00001F1F1B17001F
struct VertSkinnedXBoxHQ
{
	float x, y, z;
	float w; // (?) equals to 1.f
	struct PackedNormalXBox normal;
	float u, v;
	float weight[4];
	float bone[4]; // bone index multipled by 4
};

// 0x000000121B17001B
struct VertSkinnedPC
{
	float x, y, z;
	float normal[3];
	int bone;
	float u, v;
};

// 0x0000100000000000
struct InfluenceMap
{
	int count;
	int bone[4];
	float weight[4];
};

#endif

