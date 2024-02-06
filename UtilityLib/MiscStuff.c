#include	<stdint.h>
#include	<x86intrin.h>
#include	<string.h>
#include	<cglm/call.h>


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