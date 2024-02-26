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
void	Misc_MakeBaseOrgBound(float width, float height, float depth, vec3 min, vec3 max)
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