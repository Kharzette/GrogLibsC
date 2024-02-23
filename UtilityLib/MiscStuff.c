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

void	SSE_ReciprocalVec3(const vec3 vector, vec3 recip)
{
	__attribute__((aligned(16)))	float	vec[4]	={ vector[0], vector[1], vector[2], 1.0f };

	__m128	arg	=_mm_load_ps(vec);

	__m128	result	=_mm_rcp_ps(arg);

	memcpy(recip, &result, 12);
}


void	ClearBounds(vec3 min, vec3 max)
{
	for(int i=0;i < 3;i++)
	{
		min[i]	=MIN_MAX_BOUNDS;
		max[i]	=-MIN_MAX_BOUNDS;
	}
}

void	AddPointToBoundingBox(vec3 min, vec3 max, const vec3 pnt)
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

bool	IsPointInBounds(const vec3 min, const vec3 max, const vec3 pnt)
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

//based on reading https://tavianator.com/2011/ray_box.html and comments
//Mine checks distance as I'm usually working with finite distances
//invDir should be 1 divided by the direction vector
bool	RayIntersectBounds(const vec3 rayStart, const vec3 invDir, const float rayLen, const vec3 bounds[2])
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
int	LineIntersectBounds(const vec3 min, const vec3 max, const vec3 start, const vec3 end,
						vec3 intersection, vec3 hitNorm)
{
	vec4	boundPlanes[6];

	MakeConvexVolumeFromBound(min, max, boundPlanes);

	return	LineIntersectVolume(boundPlanes, 6, start, end, intersection, hitNorm);
}