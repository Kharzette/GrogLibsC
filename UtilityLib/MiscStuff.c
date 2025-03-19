#include	<stdint.h>
#include	<x86intrin.h>
#include	<string.h>
#include	<cglm/call.h>
#include	<cglm/box.h>
#include	<assert.h>
#include	"MiscStuff.h"


#define		MIN_MAX_BOUNDS	15192.0f

__attribute_maybe_unused__
static const vec3	UnitX	={	1.0f, 0.0f, 0.0f	};
__attribute_maybe_unused__
static const vec3	UnitY	={	0.0f, 1.0f, 0.0f	};
__attribute_maybe_unused__
static const vec3	UnitZ	={	0.0f, 0.0f, 1.0f	};

//for converting colours
__attribute_maybe_unused__
__attribute__((aligned(16)))	static const float	byteRecip[4]	=
{
	1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f
};
__attribute__((aligned(16)))	static const float	byteMul[4]	=
{
	255.0f, 255.0f, 255.0f, 255.0f
};


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

void	Misc_ConvertFlippedUVVec2ToF16(const vec2 vec, uint16_t *pDest)
{
	//note the 1-
	__m128i	converted	=Convert4F32ToF16m128i(vec[0], 1.0f - vec[1], 1.0f, 1.0f);

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

uint32_t	Misc_SSE_Vec4ToRGBA(const vec4 v)
{
	__attribute__((aligned(16)))	float	vec[4]	={ v[0], v[1], v[2], v[3] };

	__m128	arg	=_mm_load_ps(vec);

	__m128	bRec	=_mm_load_ps(byteMul);

	//scale by 255
	arg	=_mm_mul_ps(bRec, arg);

	//convert down
	__m128i	gg	=_mm_cvtps_epi32(arg);
	gg	=_mm_packus_epi32(gg, gg);
	gg	=_mm_packus_epi16(gg, gg);

	return	_mm_cvtsi128_si32(gg);
}

uint32_t	Misc_SSE_Vec3ToRGBA(const vec3 v)
{	
	__attribute__((aligned(16)))	float	vec[4]	={ v[0], v[1], v[2], 1 };

	__m128	arg	=_mm_load_ps(vec);

	__m128	bRec	=_mm_load_ps(byteMul);

	//scale by 255
	arg	=_mm_mul_ps(bRec, arg);

	//convert down
	__m128i	gg	=_mm_cvtps_epi32(arg);
	gg	=_mm_packus_epi32(gg, gg);
	gg	=_mm_packus_epi16(gg, gg);

	return	_mm_cvtsi128_si32(gg);
}

void	Misc_RGBAToVec3(uint32_t col, vec3 ret)
{
	ret[0]	=col & 0xFF;
	ret[1]	=(col & 0xFF00) >> 8;
	ret[2]	=(col & 0xFF0000) >> 16;

	glm_vec3_mul(ret, byteRecip, ret);
}

void	Misc_RGBAToVec4(uint32_t col, vec4 ret)
{
	ret[0]	=col & 0xFF;
	ret[1]	=(col & 0xFF00) >> 8;
	ret[2]	=(col & 0xFF0000) >> 16;
	ret[3]	=(col & 0xFF000000) >> 24;

	glm_vec4_mul(ret, byteRecip, ret);
}

void	Misc_LinearToSRGB(const vec4 vLin, vec4 vSRGB)
{
	float	oo22	=1.0f / 2.2f;

	for(int i=0;i < 3;i++)
	{
		vSRGB[i]	=powf(vLin[i], oo22);
	}
	vSRGB[3]	=vLin[3];
}

void	Misc_SRGBToLinear(const vec4 vSRGB, vec4 vLin)
{
	for(int i=0;i < 3;i++)
	{
		vLin[i]	=powf(vSRGB[i], 2.2f);
	}
	vLin[3]	=vSRGB[3];
}

void	Misc_SRGBToLinear255(const vec4 vSRGB, vec4 vLin)
{
	for(int i=0;i < 3;i++)
	{
		vLin[i]	=powf(vSRGB[i], 2.2f);
	}
	vLin[3]	=vSRGB[3];

	__m128	arg	=_mm_load_ps(vLin);

	__m128	bRec	=_mm_load_ps(byteMul);

	//scale by 255
	arg	=_mm_mul_ps(bRec, arg);

	_mm_store_ps(vLin, arg);
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

void	Misc_ExpandBounds(vec3 min, vec3 max, float radius)
{
	for(int i=0;i < 3;i++)
	{
		min[i]	-=radius;
		max[i]	+=radius;
	}
}

void	Misc_ExpandBoundsByBounds(vec3 min, vec3 max, const vec3 minX, const vec3 maxX)
{
	for(int i=0;i < 3;i++)
	{
		float	size	=(maxX[i] - minX[i]) * 0.5f;

		min[i]	-=size;
		max[i]	+=size;
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

	glm_vec3_add(mins, result, result);
}

//return a radiuslike distance to adjust a plane by to keep the AABB just touching it
float	Misc_BoundDistanceForNormal(const vec4 plane, const vec3 min, const vec3 max)
{
	float	minDot	=glm_vec3_dot(plane, min);
	float	maxDot	=glm_vec3_dot(plane, max);

	return	fmax(minDot, maxDot);
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

//make a random colour
void	Misc_RandomColour(vec4 col)
{
	col[0]	=rand();
	col[1]	=rand();
	col[2]	=rand();
	col[3]	=RAND_MAX;

	glm_vec4_scale(col, 1.0f / RAND_MAX, col);
}

//build basis vectors for a transform or whatever
void	Misc_BuildBasisVecsFromDirection(const vec3 direction, vec3 baseX, vec3 baseY, vec3 baseZ)
{
	//z for direction
	glm_vec3_normalize_to(direction, baseZ);

	//generate a good side vector
	glm_vec3_cross(UnitY, baseZ, baseX);
	if(glm_vec3_eq_eps(baseX, 0.0f))
	{
		//first cross failed
		glm_vec3_cross(UnitX, baseZ, baseX);
	}

	//up vec
	glm_vec3_cross(baseZ, baseX, baseY);

	assert(!glm_vec3_eq_eps(baseX, 0.0f));
	assert(!glm_vec3_eq_eps(baseY, 0.0f));

	glm_vec3_normalize(baseX);
	glm_vec3_normalize(baseY);
}

//based on reading https://tavianator.com/2011/ray_box.html and comments
//Mine checks distance as I'm usually working with finite distances
//invDir should be 1 divided by the unit direction vector
bool	Misc_RayIntersectBounds(const vec3 rayStart, const vec3 invDir, const float rayLen, const vec3 bounds[2])
{
	float	tmin, tmax, tymin, tymax, tzmin, tzmax;

	bool	signx	=(invDir[0] < 0);
	bool	signy	=(invDir[1] < 0);
	bool	signz	=(invDir[2] < 0);
	
	tmin	=(bounds[(int)signx][0] - rayStart[0]) * invDir[0];	
	tmax	=(bounds[1 - (int)signx][0] - rayStart[0]) * invDir[0];
	
	tymin	=(bounds[(int)signy][1] - rayStart[1]) * invDir[1];
	tymax	=(bounds[1 - (int)signy][1] - rayStart[1]) * invDir[1];
	
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

//non moving sphere
bool	Misc_SphereIntersectBounds(const vec3 pos, float radius, const vec3 bounds[2])
{
	for(int i=0;i < 3;i++)
	{
		if(pos[i] < (bounds[0][i] - radius))
		{
			return	false;
		}
		if(pos[i] > (bounds[1][i] + radius))
		{
			return	false;
		}
	}
	return	true;
}

bool	Misc_CheckTwoAABBOverlap(const vec3 aMin, const vec3 aMax, const vec3 bMin, const vec3 bMax)
{
	return	aMin[0] <= bMax[0] && aMax[0] >= bMin[0] &&
			aMin[1] <= bMax[1] && aMax[1] >= bMin[1] &&
			aMin[2] <= bMax[2] && aMax[2] >= bMin[2];
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

bool	Misc_CompareVec4s(const vec4 a, const vec4 b)
{
	return	((a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]) && (a[3] == b[3]));
}

bool	Misc_CompareVec3s(const vec3 a, const vec3 b)
{
	return	((a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]));
}
