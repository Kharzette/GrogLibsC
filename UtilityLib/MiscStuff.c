#include	<stdint.h>
#include	<x86intrin.h>
#include	<string.h>
#include	<cglm/call.h>
#include	<cglm/box.h>
#include	<assert.h>
#include	"MiscStuff.h"
#include	"ConvexVolume.h"


#define		MIN_MAX_BOUNDS	15192.0f


static __m128i	Convert4F32ToF16m128i(float f0, float f1, float f2, float f3)
{
	__attribute__((aligned(16)))	float	vec[4]	={ f0, f1, f2, f3 };

	__m128	arg	=_mm_load_ps(vec);

	//according to stacko:
	//0	nearest
	//1	floor
	//2	ceil
	//3	trunc
	return	_mm_cvtps_ph(arg, 0);
}

void	Misc_Convert4ToF16(float f0, float f1, float f2, float f3, uint16_t *pDest)
{
	__m128i	converted	=Convert4F32ToF16m128i(f0, f1, f2, f3);

	memcpy(pDest, &converted, 8);
}

void	Misc_Convert2ToF16(float f0, float f1, uint16_t *pDest)
{
	__m128i	converted	=Convert4F32ToF16m128i(f0, f1, f0, f1);

	memcpy(pDest, &converted, 4);
}

void	Misc_ConvertVec2ToF16(const vec2 vec, uint16_t *pDest)
{
	__m128i	converted	=Convert4F32ToF16m128i(vec[0], vec[1], 1.0f, 1.0f);

	memcpy(pDest, &converted, 4);
}

void	Misc_ConvertVec3ToF16(const vec3 vec, uint16_t *pDest)
{
	__m128i	converted	=Convert4F32ToF16m128i(vec[0], vec[1], vec[2], 1.0f);

	memcpy(pDest, &converted, 8);
}

void	Misc_ConvertVec4ToF16(const vec4 vec, uint16_t *pDest)
{
	__m128i	converted	=Convert4F32ToF16m128i(vec[0], vec[1], vec[2], vec[3]);

	memcpy(pDest, &converted, 8);
}

void	Misc_SSE_ReciprocalVec3(const vec3 vector, vec3 recip)
{
	__attribute__((aligned(16)))	float	vec[4]	={ vector[0], vector[1], vector[2], 1.0f };

	__m128	arg	=_mm_load_ps(vec);

	__m128	result	=_mm_rcp_ps(arg);

	memcpy(recip, &result, 12);
}

int	Misc_SSE_RoundFToI(float val)
{
	__attribute__((aligned(16)))	float	vec[4]	={ val, val, val, val };

	__m128	arg	=_mm_load_ps(vec);

	__m128i	result	=_mm_cvtps_epi32(arg);

	int	res;

	memcpy(&res, &result, sizeof(int));

	return	res;
}

void	Misc_ClearBounds(vec3 min, vec3 max)
{
	for(int i=0;i < 3;i++)
	{
		min[i]	=MIN_MAX_BOUNDS;
		max[i]	=-MIN_MAX_BOUNDS;
	}
}

void	Misc_AddPointToBounds(vec3 min, vec3 max, const vec3 pnt)
{
	for(int i=0;i < 3;i++)
	{
		if(pnt[i] < min[i])
		{
			min[i]	=pnt[i];
		}

		if(pnt[i] > max[i])
		{
			max[i]	=pnt[i];
		}
	}
}

bool	Misc_IsPointInBounds(const vec3 min, const vec3 max, const vec3 pnt)
{
	for(int i=0;i < 3;i++)
	{
		if(pnt[i] < min[i])
		{
			return	false;
		}
		if(pnt[i] > max[i])
		{
			return	false;
		}
	}
	return	true;
}

void	Misc_MakeBound(float width, float height, float depth, vec3 min, vec3 max)
{
	float	halfWidth	=width * 0.5f;
	float	halfHeight	=height * 0.5f;
	float	halfDepth	=depth * 0.5f;

	glm_vec3_zero(min);
	glm_vec3_zero(max);

	min[0]	=-halfWidth;
	max[0]	=halfWidth;
	
	min[1]	=-halfHeight;
	max[1]	=halfHeight;

	min[2]	=-halfDepth;
	max[2]	=halfDepth;
}

//returns a box with the Z base at the origin
void	Misc_MakeBaseZOrgBound(float width, float height, float depth, vec3 min, vec3 max)
{
	float	halfWidth	=width * 0.5f;
	float	halfHeight	=height * 0.5f;
	float	halfDepth	=depth * 0.5f;

	glm_vec3_zero(min);
	glm_vec3_zero(max);

	min[0]	=-halfWidth;
	max[0]	=halfWidth;
	
	min[1]	=-halfHeight;
	max[1]	=halfHeight;

	min[2]	=0;
	max[2]	=depth;
}

//make a random point within a bound
void	Misc_RandomPointInBound(const vec3 mins, const vec3 maxs, vec3 result)
{
	double	scalarX	=(maxs[0] - mins[0]) / RAND_MAX;
	double	scalarY	=(maxs[1] - mins[1]) / RAND_MAX;
	double	scalarZ	=(maxs[2] - mins[2]) / RAND_MAX;

	int	x	=rand();
	int	y	=rand();
	int	z	=rand();

	result[0]	=scalarX * x;
	result[1]	=scalarY * y;
	result[2]	=scalarZ * z;
}

//make a random direction (unit vector)
void	Misc_RandomDirection(vec3 dir)
{
	float	len		=0.0f;

	for(;;)
	{
		dir[0]	=rand() - (RAND_MAX >> 1);
		dir[1]	=rand() - (RAND_MAX >> 1);
		dir[2]	=rand() - (RAND_MAX >> 1);

		len	=glm_vec3_norm(dir);
		if(len > FLT_EPSILON)
		{
			break;
		}
	}

	glm_vec3_scale(dir, 1.0f / len, dir);
}


//intersection of line and plane
int	Misc_LineIntersectPlane(const vec4 plane, const vec3 start, const vec3 end, vec3 intersection)
{
	float	startDist	=glm_vec3_dot(plane, start) - plane[3];
	float	endDist		=glm_vec3_dot(plane, end) - plane[3];

	if(startDist > 0.0f && endDist > 0.0f)
	{
		return	PLANE_FRONT;
	}
	else if(startDist < 0.0f && endDist < 0.0f)
	{
		return	PLANE_BACK;
	}

	//get a unit vector of the line segment
	vec3	segVec;
	glm_vec3_sub(end, start, segVec);

	//vector length is named strangely in glm
	float	segLength	=glm_vec3_norm(segVec);

	//normalize
	glm_vec3_scale(segVec, 1.0f / segLength, segVec);

	float	ratio	=startDist / (startDist - endDist);

	glm_vec3_scale(segVec, ratio * segLength, intersection);

	glm_vec3_add(intersection, start, intersection);

	return	PLANE_HIT;
}

//based on reading https://tavianator.com/2011/ray_box.html and comments
//Mine checks distance as I'm usually working with finite distances
//invDir should be 1 divided by the direction vector
bool	Misc_RayIntersectBounds(const vec3 rayStart, const vec3 invDir, const float rayLen, const vec3 bounds[2])
{
	float	tmin, tmax, tymin, tymax, tzmin, tzmax;

	char	signx	=(invDir[0] < 0);
	char	signy	=(invDir[1] < 0);
	char	signz	=(invDir[2] < 0);
	
	tmin	=(bounds[signx][0] - rayStart[0]) * invDir[0];	
	tmax	=(bounds[1 - signx][0] - rayStart[0]) * invDir[0];
	
	tymin	=(bounds[signy][1] - rayStart[1]) * invDir[1];
	tymax	=(bounds[1 - signy][1] - rayStart[1]) * invDir[1];
	
	if((tmin > tymax) || (tymin > tmax))
	{
		return false;
	}

	if(tymin > tmin)
	{
		tmin	=tymin;
	}
	if(tymax < tmax)
	{
		tmax	=tymax;
	}
	
	tzmin	=(bounds[signz][2] - rayStart[2]) * invDir[2];
	tzmax	=(bounds[1 - signz][2] - rayStart[2]) * invDir[2];
	
	if((tmin > tzmax) || (tzmin > tmax))
	{
		return false;
	}
	
	if(tzmin > tmin)
	{
		tmin	=tzmin;
	}
	if(tzmax < tmax)
	{
		tmax	=tzmax;
	}

	//is the intersection within the ray length?
	if(tmin < rayLen)
	{
		return	true;
	}
	return	false;
}

//return one of the above values along with the intersection point and normal
int	Misc_LineIntersectBounds(const vec3 min, const vec3 max, const vec3 start, const vec3 end,
						vec3 intersection, vec3 hitNorm)
{
	vec4	boundPlanes[6];

	MakeConvexVolumeFromBound(min, max, boundPlanes);

	return	LineIntersectVolume(boundPlanes, 6, start, end, intersection, hitNorm);
}

//fails if the triangle has no plane (colinear points etc)
bool	Misc_PlaneFromVerts(const vec3 *pVerts, int numVerts, vec4 plane)
{
	int	i;

	glm_vec4_zero(plane);

	if(numVerts < 3)
	{
		return	false;
	}

	for(i=0;i < numVerts;i++)
	{
		vec3	v1, v2;

		//edge vectors
		glm_vec3_sub(pVerts[i], pVerts[(i + 1) % numVerts], v1);
		glm_vec3_sub(pVerts[(i + 2) % numVerts], pVerts[(i + 1) % numVerts], v2);

		//gen a plane normal from the cross of edge vectors
		glm_vec3_cross(v1, v2, plane);

		if(!glm_vec3_eq_eps(plane, 0.0f))
		{
			break;
		}
		//try the next three if there are three
	}

	if(i >= numVerts)
	{
		return	false;
	}

	glm_vec3_normalize(plane);

	//plane distance
	plane[3]	=glm_vec3_dot(pVerts[0], plane);

	return	true;
}

bool	Misc_PlaneFromTri(const vec3 v0, const vec3 v1, const vec3 v2, vec4 plane)
{
	vec3	tri[3];

	memcpy(tri[0], v0, sizeof(vec3));
	memcpy(tri[1], v1, sizeof(vec3));
	memcpy(tri[2], v2, sizeof(vec3));

	return	Misc_PlaneFromVerts(tri, 3, plane);
}

//uses the add up the angles trick to determine point in poly
//point is inside if return is around 2 pi or over
float	Misc_ComputeAngleSum(const vec3 pointOnPlane, const vec3 *pVerts, int numVerts)
{
	float	dotSum	=0.0f;
	for(int i=0;i < numVerts;i++)
	{
		int	vIdx0	=i;
		int	vIdx1	=((i + 1) % numVerts);

		vec3	v1, v2;

		glm_vec3_sub(pVerts[vIdx0], pointOnPlane, v1);
		glm_vec3_sub(pVerts[vIdx1], pointOnPlane, v2);

		float	len1	=glm_vec3_norm(v1);;
		float	len2	=glm_vec3_norm(v2);;

		if((len1 * len2) < 0.0001f)
		{
			return	GLM_2_PI;
		}

		glm_vec3_scale(v1, 1.0f / len1, v1);
		glm_vec3_scale(v2, 1.0f / len2, v2);

		float	dot	=glm_vec3_dot(v1, v2);
		if(dot > 1.0f)
		{
			dot	=1.0f;
		}

		dotSum	+=acosf(dot);
	}
	return	dotSum;
}