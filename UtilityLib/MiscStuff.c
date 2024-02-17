#include	<stdint.h>
#include	<x86intrin.h>
#include	<string.h>
#include	<cglm/call.h>
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

static vec3	NormX		={	1.0f, 0.0f, 0.0f	};
static vec3	NormY		={	0.0f, 1.0f, 0.0f	};
static vec3	NormZ		={	0.0f, 0.0f, 1.0f	};
static vec3	NormNegX	={	-1.0f, 0.0f, 0.0f	};
static vec3	NormNegY	={	0.0f, -1.0f, 0.0f	};
static vec3	NormNegZ	={	0.0f, 0.0f, -1.0f	};

//return one of the above values along with the intersection point and normal
int	LineIntersectBounds(const vec3 min, const vec3 max, const vec3 start, const vec3 end,
						vec3 intersection, vec3 hitNorm)
{
	vec4	boundPlanes[6];

	MakeConvexVolumeFromBound(min, max, boundPlanes);

	return	LineIntersectVolume(boundPlanes, 6, start, end, intersection, hitNorm);

	//start point inside bounds?
	bool	bStartInside	=IsPointInBounds(min, max, start);

	//fully inside?
	if(bStartInside && IsPointInBounds(min, max, end))
	{
		return	INSIDE;
	}

	//get a unit vector of the line segment
	vec3	segVec;
	glm_vec3_sub(end, start, segVec);

	//vector length is named strangely in glm
	float	segLength	=glm_vec3_norm(segVec);

	//normalize
	glm_vec3_scale(segVec, 1.0f / segLength, segVec);


	//get plane distances from origin
	float	minDistX	=glm_vec3_dot(min, NormNegX);
	float	minDistY	=glm_vec3_dot(min, NormNegY);
	float	minDistZ	=glm_vec3_dot(min, NormNegZ);
	float	maxDistX	=glm_vec3_dot(max, NormX);
	float	maxDistY	=glm_vec3_dot(max, NormY);
	float	maxDistZ	=glm_vec3_dot(max, NormZ);
	
	//test against bound planes

	//-X
	float	startNegX	=glm_vec3_dot(start, NormNegX) - minDistX;
	float	endNegX		=glm_vec3_dot(end, NormNegX) - minDistX;
	if(startNegX > 0.0f && endNegX > 0.0f)
	{
		return	MISS;
	}

	//X
	float	startX	=glm_vec3_dot(start, NormX) - maxDistX;
	float	endX	=glm_vec3_dot(end, NormX) - maxDistX;
	if(startX > 0.0f && endX > 0.0f)
	{
		return	MISS;
	}

	//-Y
	float	startNegY	=glm_vec3_dot(start, NormNegY) - minDistY;
	float	endNegY		=glm_vec3_dot(end, NormNegY) - minDistY;
	if(startNegY > 0.0f && endNegY > 0.0f)
	{
		return	MISS;
	}

	//Y
	float	startY	=glm_vec3_dot(start, NormY) - maxDistY;
	float	endY	=glm_vec3_dot(end, NormY) - maxDistY;
	if(startY > 0.0f && endY > 0.0f)
	{
		return	MISS;
	}

	//-Z
	float	startNegZ	=glm_vec3_dot(start, NormNegZ) - minDistZ;
	float	endNegZ		=glm_vec3_dot(end, NormNegZ) - minDistZ;
	if(startNegZ > 0.0f && endNegZ > 0.0f)
	{
		return	MISS;
	}

	//Z
	float	startZ	=glm_vec3_dot(start, NormZ) - maxDistZ;
	float	endZ	=glm_vec3_dot(end, NormZ) - maxDistZ;
	if(startZ > 0.0f && endZ > 0.0f)
	{
		return	MISS;
	}

	//One or more intersections have happened.
	//Find the vector length along start->end
	//where the intersection happened for each plane.
	//Negative indicates no intersection.

	int		nearest			=-1;
	float	nearSect		=FLT_MAX;
	float	sectNegX		=-1.0f;
	if(startNegX > 0.0f && endNegX < 0.0f)
	{
		sectNegX	=(startNegX / (startNegX - endNegX)) * segLength;
	}
	else if(startNegX < 0.0f && endNegX > 0.0f)
	{
		sectNegX	=(endNegX / (endNegX - startNegX)) * segLength;
	}

	if(sectNegX >= 0.0f && sectNegX < nearSect)
	{
		nearest		=0;
		nearSect	=sectNegX;
	}

	float	sectNegY	=-1.0f;
	if(startNegY > 0.0f && endNegY < 0.0f)
	{
		sectNegY	=(startNegY / (startNegY - endNegY)) * segLength;
	}
	else if(startNegY < 0.0f && endNegY > 0.0f)
	{
		sectNegY	=(endNegY / (endNegY - startNegY)) * segLength;
	}

	if(sectNegY >= 0.0f && sectNegY < nearSect)
	{
		nearest		=1;
		nearSect	=sectNegY;
	}

	float	sectNegZ	=-1.0f;
	if(startNegZ > 0.0f && endNegZ < 0.0f)
	{
		sectNegZ	=(startNegZ / (startNegZ - endNegZ)) * segLength;
	}
	else if(startNegZ < 0.0f && endNegZ > 0.0f)
	{
		sectNegZ	=(endNegZ / (endNegZ - startNegZ)) * segLength;
	}

	if(sectNegZ >= 0.0f && sectNegZ < nearSect)
	{
		nearest		=2;
		nearSect	=sectNegZ;
	}

	float	sectX	=-1.0f;
	if(startX > 0.0f && endX < 0.0f)
	{
		sectX	=(startX / (startX - endX)) * segLength;
	}
	else if(startX < 0.0f && endX > 0.0f)
	{
		sectX	=(endX / (endX - startX)) * segLength;
	}

	if(sectX >= 0.0f && sectX < nearSect)
	{
		nearest		=3;
		nearSect	=sectX;
	}

	float	sectY	=-1.0f;
	if(startY > 0.0f && endY < 0.0f)
	{
		sectY	=(startY / (startY - endY)) * segLength;
	}
	else if(startY < 0.0f && endY > 0.0f)
	{
		sectY	=(endY / (endY - startY)) * segLength;
	}

	if(sectY >= 0.0f && sectY < nearSect)
	{
		nearest		=4;
		nearSect	=sectY;
	}

	float	sectZ	=-1.0f;
	if(startZ > 0.0f && endZ < 0.0f)
	{
		sectZ	=(startZ / (startZ - endZ)) * segLength;
	}
	else if(startZ < 0.0f && endZ > 0.0f)
	{
		sectZ	=(endZ / (endZ - startZ)) * segLength;
	}

	if(sectZ >= 0.0f && sectZ < nearSect)
	{
		nearest		=5;
		nearSect	=sectZ;
	}

	glm_vec3_scale(segVec, nearSect, intersection);

	glm_vec3_add(intersection, start, intersection);

	if(nearest < 0)
	{
		assert(0);
		return	INSIDE;
	}
	else if(nearest == 0)
	{
		glm_vec3_copy(NormNegX, hitNorm);
	}
	else if(nearest == 1)
	{
		glm_vec3_copy(NormNegY, hitNorm);
	}
	else if(nearest == 2)
	{
		glm_vec3_copy(NormNegZ, hitNorm);
	}
	else if(nearest == 3)
	{
		glm_vec3_copy(NormX, hitNorm);
	}
	else if(nearest == 4)
	{
		glm_vec3_copy(NormY, hitNorm);
	}
	else if(nearest == 5)
	{
		glm_vec3_copy(NormZ, hitNorm);
	}

	if(bStartInside)
	{
		return	INSIDE_INTERSECT;
	}
	return	INTERSECT;
}