#ifndef __VECTOR_H__
#define __VECTOR_H__

#include <math.h>
#include <string.h> /* memcpy */

/* lineary interpolation of scalar */
static __inline float lerp(float a, float b, float f) 
{
	return(a * (1.f - f) + b * f);
}

/* set vector components to x, y, z */
static __inline void vset2(float v[3], float x, float y)
{
	v[0] = x;
	v[1] = y;
}

/* set vector components to x, y, z */
static __inline void vset3(float v[3], float x, float y, float z)
{
	v[0] = x;
	v[1] = y;
	v[2] = z;
}

/* set vector components to x, y, z, w */
static __inline void vset4(float v[4], float x, float y, float z, float w)
{
	v[0] = x;
	v[1] = y;
	v[2] = z;
	v[3] = w;
}

/* copy one vector to another */
static __inline void vcopy(float a[3], const float b[3])
{
	a[0] = b[0];
	a[1] = b[1];
	a[2] = b[2];
}

/* add two vectors */
static __inline void vadd(float v1[3], const float v2[3])
{
	v1[0] += v2[0];
	v1[1] += v2[1];
	v1[2] += v2[2];
}

static __inline void vadd_3(float d[3], const float v1[3], const float v2[3])
{
	d[0] = v1[0] + v2[0];
	d[1] = v1[1] + v2[1];
	d[2] = v1[2] + v2[2];
}

/* calculate vector from point v2 to point v1 */
static __inline void vsub(float v1[3], const float v2[3])
{
	v1[0] -= v2[0];
	v1[1] -= v2[1];
	v1[2] -= v2[2];
}

static __inline void vsub_3(float d[3], const float v1[3], const float v2[3])
{
	d[0] = v1[0] - v2[0];
	d[1] = v1[1] - v2[1];
	d[2] = v1[2] - v2[2];
}

/* calculate dot product between two vectors */
static __inline float vdot3(const float v1[3], const float v2[3])
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

/* calculate cross product */
static __inline void vcross(float dest[3], const float v1[3], const float v2[3])
{
	dest[0] = v1[1]*v2[2] - v1[2]*v2[1];
	dest[1] = v1[2]*v2[0] - v1[0]*v2[2];
	dest[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

/* normalize vector */
static __inline void vnormalize(float v[3])
{
	float m = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	v[0] /= m;
	v[1] /= m;
	v[2] /= m;
}

static __inline void vnormalize_l(float v[3], float *l)
{
	float m = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	v[0] /= m;
	v[1] /= m;
	v[2] /= m;
	*l = m;
}

/* calculate length of a vector */
static __inline float vlen(float v[3])
{
	return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

/* calculate triangle normal */
static __inline void vtrinormal(float dest[3], const float p1[3], const float p2[3], const float p3[3])
{
	float e1[3], e2[3];
	
	e1[0] = p2[0] - p1[0];
	e1[1] = p2[1] - p1[1];
	e1[2] = p2[2] - p1[2];
	
	e2[0] = p3[0] - p1[0];
	e2[1] = p3[1] - p1[1];
	e2[2] = p3[2] - p1[2];
	
	vcross(dest, e1, e2);
	vnormalize(dest);
}

/* calculate distance between two points */
static __inline float vdist(const float v1[3], const float v2[3])
{
	return sqrt((v1[0]-v2[0])*(v1[0]-v2[0]) + (v1[1]-v2[1])*(v1[1]-v2[1]) + (v1[2]-v2[2])*(v1[2]-v2[2]));
}

/* */
static __inline void vmin(float a[3], const float b[3])
{
	if(a[0] > b[0]) a[0] = b[0];
	if(a[1] > b[1]) a[1] = b[1];
	if(a[2] > b[2]) a[2] = b[2];
}

static __inline void vmin_3(float v[3], float a[3], const float b[3])
{
	v[0] = a[0] > b[0] ? b[0] : a[0];
	v[1] = a[1] > b[1] ? b[1] : a[1];
	v[2] = a[2] > b[2] ? b[2] : a[2];
}

/* */
static __inline void vmax(float a[3], const float b[3])
{
	if(a[0] < b[0]) a[0] = b[0];
	if(a[1] < b[1]) a[1] = b[1];
	if(a[2] < b[2]) a[2] = b[2];
}

static __inline void vmax_3(float v[3], float a[3], const float b[3])
{
	v[0] = a[0] < b[0] ? b[0] : a[0];
	v[1] = a[1] < b[1] ? b[1] : a[1];
	v[2] = a[2] < b[2] ? b[2] : a[2];
}

/* 4x4 transform (with projection) */
static __inline void transform44(float v[3], /*const*/ float m[4][4])
{
	float x = v[0], y = v[1], z = v[2], w;
	
	v[0] = x * m[0][0] + y * m[1][0] + z * m[2][0] + m[3][0];
	v[1] = x * m[0][1] + y * m[1][1] + z * m[2][1] + m[3][1];
	v[2] = x * m[0][2] + y * m[1][2] + z * m[2][2] + m[3][2];
	
	w = x * m[0][3] + y * m[1][3] + z * m[2][3] + m[3][3];
	
	v[0] /= w;
	v[1] /= w;
	v[2] /= w;
}

/* 4x3 transform (no projection) */
static __inline void transform(float v[3], /*const*/ float m[4][4])
{
	float x = v[0], y = v[1], z = v[2];
	
	v[0] = x * m[0][0] + y * m[1][0] + z * m[2][0] + m[3][0];
	v[1] = x * m[0][1] + y * m[1][1] + z * m[2][1] + m[3][1];
	v[2] = x * m[0][2] + y * m[1][2] + z * m[2][2] + m[3][2];
}

/* 4x3 transform (no projection), with separate result argument */
static __inline void transform_3(float dest[3], float v[3], /*const*/ float m[4][4])
{
	dest[0] = v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0] + m[3][0];
	dest[1] = v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1] + m[3][1];
	dest[2] = v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2] + m[3][2];
}

/* 4x3 transform (no projection) by transposed matrix, with separate result argument */
static __inline void transform_3t(float dest[3], float v[3], /*const*/ float m[4][4])
{
	dest[0] = v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m[0][2] + m[0][3];
	dest[1] = v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m[1][2] + m[1][3];
	dest[2] = v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m[2][2] + m[2][3];
}

/* 3x3 transform (only rotation & scale) */
static __inline void transform33(float v[3], /*const*/ float m[4][4])
{
	float x = v[0], y = v[1], z = v[2];
	
	v[0] = x * m[0][0] + y * m[1][0] + z * m[2][0];
	v[1] = x * m[0][1] + y * m[1][1] + z * m[2][1];
	v[2] = x * m[0][2] + y * m[1][2] + z * m[2][2];
}

/* 3x3 transform (only rotation & scale) with separate result argument */
static __inline void transform33_3(float dest[3], const float v[3], /*const*/ float m[4][4])
{
	dest[0] = v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0];
	dest[1] = v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1];
	dest[2] = v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2];
}

/* 3x3 transform (only rotation & scale) by transposed matrix, with separate result argument */
static __inline void transform33_3t(float dest[3], const float v[3], /*const*/ float m[4][4])
{
	dest[0] = v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m[0][2];
	dest[1] = v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m[1][2];
	dest[2] = v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m[2][2];
}

/* multiply d matrix by s matrix and store result in a */
static __inline void mul44(float a[4][4], /*const*/ float d[4][4], /*const*/ float s[4][4])
{
	a[0][0] = s[0][0] * d[0][0] + s[0][1] * d[1][0] + s[0][2] * d[2][0] + s[0][3] * d[3][0];
	a[0][1] = s[0][0] * d[0][1] + s[0][1] * d[1][1] + s[0][2] * d[2][1] + s[0][3] * d[3][1];
	a[0][2] = s[0][0] * d[0][2] + s[0][1] * d[1][2] + s[0][2] * d[2][2] + s[0][3] * d[3][2];
	a[0][3] = s[0][0] * d[0][3] + s[0][1] * d[1][3] + s[0][2] * d[2][3] + s[0][3] * d[3][3];
	
	a[1][0] = s[1][0] * d[0][0] + s[1][1] * d[1][0] + s[1][2] * d[2][0] + s[1][3] * d[3][0];
	a[1][1] = s[1][0] * d[0][1] + s[1][1] * d[1][1] + s[1][2] * d[2][1] + s[1][3] * d[3][1];
	a[1][2] = s[1][0] * d[0][2] + s[1][1] * d[1][2] + s[1][2] * d[2][2] + s[1][3] * d[3][2];
	a[1][3] = s[1][0] * d[0][3] + s[1][1] * d[1][3] + s[1][2] * d[2][3] + s[1][3] * d[3][3];
	
	a[2][0] = s[2][0] * d[0][0] + s[2][1] * d[1][0] + s[2][2] * d[2][0] + s[2][3] * d[3][0];
	a[2][1] = s[2][0] * d[0][1] + s[2][1] * d[1][1] + s[2][2] * d[2][1] + s[2][3] * d[3][1];
	a[2][2] = s[2][0] * d[0][2] + s[2][1] * d[1][2] + s[2][2] * d[2][2] + s[2][3] * d[3][2];
	a[2][3] = s[2][0] * d[0][3] + s[2][1] * d[1][3] + s[2][2] * d[2][3] + s[2][3] * d[3][3];
	
	a[3][0] = s[3][0] * d[0][0] + s[3][1] * d[1][0] + s[3][2] * d[2][0] + s[3][3] * d[3][0];
	a[3][1] = s[3][0] * d[0][1] + s[3][1] * d[1][1] + s[3][2] * d[2][1] + s[3][3] * d[3][1];
	a[3][2] = s[3][0] * d[0][2] + s[3][1] * d[1][2] + s[3][2] * d[2][2] + s[3][3] * d[3][2];
	a[3][3] = s[3][0] * d[0][3] + s[3][1] * d[1][3] + s[3][2] * d[2][3] + s[3][3] * d[3][3];
}

/* reset matrix to identity */
static __inline float *identity(float m[4][4])
{
	m[0][0] = 1.f; m[0][1] = 0.f; m[0][2] = 0.f; m[0][3] = 0.f;
	m[1][0] = 0.f; m[1][1] = 1.f; m[1][2] = 0.f; m[1][3] = 0.f;
	m[2][0] = 0.f; m[2][1] = 0.f; m[2][2] = 1.f; m[2][3] = 0.f;
	m[3][0] = 0.f; m[3][1] = 0.f; m[3][2] = 0.f; m[3][3] = 1.f;
	return &m[0][0];
}

/* calculate rotation matrix around X, Y or Z axis */
static __inline void rotate_x(float m[4][4], float angle) 
{
	m[0][0] = 1.f; m[0][1] = 0.f;         m[0][2] = 0.f;        m[0][3] = 0.f;
	m[1][0] = 0.f; m[1][1] = cos(angle);  m[1][2] = sin(angle); m[1][3] = 0.f;
	m[2][0] = 0.f; m[2][1] = -sin(angle); m[2][2] = cos(angle); m[2][3] = 0.f;
	m[3][0] = 0.f; m[3][1] = 0.f;         m[3][2] = 0.f;        m[3][3] = 1.f;
}

static __inline void rotate_y(float m[4][4], float angle) 
{
	m[0][0] = cos(angle); m[0][1] = 0.f; m[0][2] = -sin(angle); m[0][3] = 0.f;
	m[1][0] = 0.f;        m[1][1] = 1.f; m[1][2] = 0.f;         m[1][3] = 0.f;
	m[2][0] = sin(angle); m[2][1] = 0.f; m[2][2] = cos(angle);  m[2][3] = 0.f;
	m[3][0] = 0.f;        m[3][1] = 0.f; m[3][2] = 0.f;         m[3][3] = 1.f;
}

static __inline void rotate_z(float m[4][4], float angle) 
{
	m[0][0] = cos(angle);  m[0][1] = sin(angle); m[0][2] = 0.f; m[0][3] = 0.f;
	m[1][0] = -sin(angle); m[1][1] = cos(angle); m[1][2] = 0.f; m[1][3] = 0.f;
	m[2][0] = 0.f;         m[2][1] = 0.f;        m[2][2] = 1.f; m[2][3] = 0.f;
	m[3][0] = 0.f;         m[3][1] = 0.f;        m[3][2] = 0.f; m[3][3] = 1.f;
}

/* calculate rotation matrix around X, Y or Z axis, and multiply m argument by it */
static __inline void mul_rotate_x(float m[4][4], float angle)
{
	float copy[4][4];
	float rx[4][4];
	
	memcpy(copy, m, 64);
	rotate_x(rx, angle);
	
	mul44(m, copy, rx);
}

static __inline void mul_rotate_y(float m[4][4], float angle)
{
	float copy[4][4];
	float ry[4][4];
	
	memcpy(copy, m, 64);
	rotate_y(ry, angle);
	
	mul44(m, copy, ry);
}

static __inline void mul_rotate_z(float m[4][4], float angle)
{
	float copy[4][4];
	float rz[4][4];
	
	memcpy(copy, m, 64);
	rotate_z(rz, angle);
	
	mul44(m, copy, rz);
}

static __inline void scale(float m[4][4], float x, float y, float z)
{
	m[0][0] = x;   m[0][1] = 0.f; m[0][2] = 0.f; m[0][3] = 0.f;
	m[1][0] = 0.f; m[1][1] = y;   m[1][2] = 0.f; m[1][3] = 0.f;
	m[2][0] = 0.f; m[2][1] = 0.f; m[2][2] = z;   m[2][3] = 0.f;
	m[3][0] = 0.f; m[3][1] = 0.f; m[3][2] = 0.f; m[3][3] = 1.f;
}

static __inline void mul_scale(float m[4][4], float x, float y, float z)
{
	float copy[4][4];
	float s[4][4];
	
	memcpy(copy, m, 64);
	scale(s, x, y, z);
	
	mul44(m, copy, s);
}

static __inline void translate(float m[4][4], float x, float y, float z)
{
	m[0][0] = 1.f; m[0][1] = 0.f; m[0][2] = 0.f; m[0][3] = 0.f;
	m[1][0] = 0.f; m[1][1] = 1.f; m[1][2] = 0.f; m[1][3] = 0.f;
	m[2][0] = 0.f; m[2][1] = 0.f; m[2][2] = 1.f; m[2][3] = 0.f;
	m[3][0] = x;   m[3][1] = y;   m[3][2] = z;   m[3][3] = 1.f;
}

/* transpose 3x3 matrix */
static __inline void transpose33(float d[4][4], /*const*/ float s[4][4])
{
	d[0][0] = s[0][0]; d[0][1] = s[1][0]; d[0][2] = s[2][0]; d[0][3] = 0.f;
	d[1][0] = s[0][1]; d[1][1] = s[1][1]; d[1][2] = s[2][1]; d[1][3] = 0.f;
	d[2][0] = s[0][2]; d[2][1] = s[1][2]; d[2][2] = s[2][2]; d[2][3] = 0.f;
	d[3][0] = 0.f; d[3][1] = 0.f; d[3][2] = 0.f; d[3][3] = 1.f;
}

/* transpose 4x4 matrix */
static __inline void transpose44(float d[4][4], /*const*/ float s[4][4])
{
	d[0][0] = s[0][0]; d[0][1] = s[1][0]; d[0][2] = s[2][0]; d[0][3] = s[3][0];
	d[1][0] = s[0][1]; d[1][1] = s[1][1]; d[1][2] = s[2][1]; d[1][3] = s[3][1];
	d[2][0] = s[0][2]; d[2][1] = s[1][2]; d[2][2] = s[2][2]; d[2][3] = s[3][2];
	d[3][0] = s[0][3]; d[3][1] = s[1][3]; d[3][2] = s[2][3]; d[3][3] = s[3][3];
}

/* inverse 4x3 matrix */
static __inline void invert43(float d[4][4], /*const*/ float s[4][4])
{
	float det, det_inv;

	det = 
		s[0][0] * (s[1][1] * s[2][2] - s[1][2] * s[2][1]) - 
		s[0][1] * (s[1][0] * s[2][2] - s[1][2] * s[2][0]) +
		s[0][2] * (s[1][0] * s[2][1] - s[1][1] * s[2][0]);
		
	det_inv = 1.f / det;
	
	d[0][0] =  det_inv * (s[1][1] * s[2][2] - s[1][2] * s[2][1]);
	d[0][1] = -det_inv * (s[0][1] * s[2][2] - s[0][2] * s[2][1]);
	d[0][2] =  det_inv * (s[0][1] * s[1][2] - s[0][2] * s[1][1]);
	d[0][3] = 0.f;
	
	d[1][0] = -det_inv * (s[1][0] * s[2][2] - s[1][2] * s[2][0]);
	d[1][1] =  det_inv * (s[0][0] * s[2][2] - s[0][2] * s[2][0]);
	d[1][2] = -det_inv * (s[0][0] * s[1][2] - s[0][2] * s[1][0]);
	d[1][3] = 0.f;
	
	d[2][0] =  det_inv * (s[1][0] * s[2][1] - s[1][1] * s[2][0]);
	d[2][1] = -det_inv * (s[0][0] * s[2][1] - s[0][1] * s[2][0]);
	d[2][2] =  det_inv * (s[0][0] * s[1][1] - s[0][1] * s[1][0]);
	d[2][3] = 0.f;		
	
	d[3][0] = -(s[3][0] * d[0][0] + s[3][1] * d[1][0] + s[3][2] * d[2][0]);
	d[3][1] = -(s[3][0] * d[0][1] + s[3][1] * d[1][1] + s[3][2] * d[2][1]);
	d[3][2] = -(s[3][0] * d[0][2] + s[3][1] * d[1][2] + s[3][2] * d[2][2]);
	d[3][3] = 1.f;
}

#endif
