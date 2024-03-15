#include	<stdint.h>
#include	<stdio.h>
#include	<string.h>
#include	<assert.h>
#include	"../UtilityLib/FileStuff.h"
#include	"../UtilityLib/MiscStuff.h"
#include	"../UtilityLib/ConvexVolume.h"
#include	"Terrain.h"
#include	"QuadNode.h"


typedef struct	QuadTree_t
{
	QuadNode	*mpRoot;

	//bounds
	vec3	mMins, mMaxs;
}	QuadTree;


QuadTree	*QT_Create(TerrainVert *pVerts, int w, int h)
{
	QuadTree	*pQT	=malloc(sizeof(QuadTree));

	memset(pQT, 0, sizeof(QuadTree));

	//make a simpler grid of heights
	float	*pHeights	=malloc(sizeof(float) * w * h);

	float	minHeight	=FLT_MAX;
	float	maxHeight	=-FLT_MAX;
	for(int y=0;y < h;y++)
	{
		for(int x=0;x < w;x++)
		{
			int	ofs	=(y * w) + x;

			float	height	=pVerts[ofs].mPosition[1];
			pHeights[ofs]	=height;

			if(height < minHeight)
			{
				minHeight	=height;
			}
			if(height > maxHeight)
			{
				maxHeight	=height;
			}
		}
	}

	//set up bounds
	glm_vec3_zero(pQT->mMins);
	pQT->mMins[1]	=minHeight;

	pQT->mMaxs[0]	=w - 1;
	pQT->mMaxs[1]	=maxHeight;
	pQT->mMaxs[2]	=h - 1;

	pQT->mpRoot	=QN_Build(pHeights, w, h, pQT->mMins, pQT->mMaxs);

	return	pQT;
}


void	QT_GetBounds(const QuadTree *pQT, vec3 mins, vec3 maxs)
{
	memcpy(mins, pQT->mMins, sizeof(vec3));
	memcpy(maxs, pQT->mMaxs, sizeof(vec3));
}


//get a list of the bounds of all leafs
//This is only used for debug draw
void	QT_GatherLeafBounds(const QuadTree *pQT, vec3 **ppMins, vec3 **ppMaxs, int *pNumBounds)
{
	assert(ppMins != NULL);
	assert(ppMaxs != NULL);
	assert(pNumBounds != NULL);

	*pNumBounds	=0;

	//count bounds
	QN_CountLeafBounds(pQT->mpRoot, pNumBounds);

	if(*pNumBounds <= 0)
	{
		return;
	}

	//alloc space for the bounds
	*ppMins	=malloc(sizeof(vec3) * (*pNumBounds));
	*ppMaxs	=malloc(sizeof(vec3) * (*pNumBounds));

	int	index	=0;

	QN_GatherLeafBounds(pQT->mpRoot, *ppMins, *ppMaxs, &index);
}


//Note that a previous intersection might be passed in here.
//Only a new intersection closer to the start point would result
//in a hit.  This could be used to mix collision with statics or
//mobiles or bsps or whatever
int	QT_LineIntersect(const QuadTree *pQT, const vec3 start, const vec3 end,
					vec3 intersection, vec4 planeHit)
{
	//convert to a ray format
	vec3	rayDir;

	glm_vec3_sub(end, start, rayDir);

	float	rayLen	=glm_vec3_norm(rayDir);

	glm_vec3_scale(rayDir, 1.0f / rayLen, rayDir);

	vec3	invDir;
	Misc_SSE_ReciprocalVec3(rayDir, invDir);

	return	QN_LineIntersect(pQT->mpRoot, start, end, invDir, rayLen, intersection, planeHit);
}

//Note that a previous intersection might be passed in here.
//Only a new intersection closer to the start point would result
//in a hit.  This could be used to mix collision with statics or
//mobiles or bsps or whatever
int	QT_SweptSphereIntersect(const QuadTree *pQT, const vec3 start, const vec3 end,
							float radius, vec3 intersection, vec4 planeHit)
{
	//convert to a ray format
	vec3	rayDir;

	glm_vec3_sub(end, start, rayDir);

	float	rayLen	=glm_vec3_norm(rayDir);

	glm_vec3_scale(rayDir, 1.0f / rayLen, rayDir);

	vec3	invDir;
	Misc_SSE_ReciprocalVec3(rayDir, invDir);

	return	QN_SweptSphereIntersect(pQT->mpRoot, start, end, invDir, radius, rayLen, intersection, planeHit);
}

//Note that a previous intersection might be passed in here.
//Only a new intersection closer to the start point would result
//in a hit.  This could be used to mix collision with statics or
//mobiles or bsps or whatever
int	QT_SweptBoundIntersect(const QuadTree *pQT, const vec3 start, const vec3 end,
							const vec3 min, const vec3 max,
							vec3 intersection, vec4 planeHit)
{
	//convert to a ray format
	vec3	rayDir;

	glm_vec3_sub(end, start, rayDir);

	float	rayLen	=glm_vec3_norm(rayDir);

	glm_vec3_scale(rayDir, 1.0f / rayLen, rayDir);

	vec3	invDir;
	Misc_SSE_ReciprocalVec3(rayDir, invDir);

	return	QN_SweptBoundIntersect(pQT->mpRoot, start, end, invDir, rayLen, min, max, intersection, planeHit);
}