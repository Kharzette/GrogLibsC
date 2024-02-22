#include	<stdint.h>
#include	<stdio.h>
#include	<string.h>
#include	<assert.h>
#include	"../UtilityLib/FileStuff.h"
#include	"../UtilityLib/MiscStuff.h"
#include	"Terrain.h"
#include	"QuadNode.h"


typedef struct	QuadTree_t
{
	QuadNode	*mpRoot;

	//bounds
	vec3	mMins, mMaxs;
}	QuadTree;


static void	BoundVerts(const TerrainVert *pVerts, int numVerts, vec3 mins, vec3 maxs)
{
	assert(numVerts > 0);

	ClearBounds(mins, maxs);

	for(int i=0;i < numVerts;i++)
	{
		AddPointToBoundingBox(mins, maxs, pVerts[i].mPosition);
	}
}


QuadTree	*QT_Create(TerrainVert *pVerts, int w, int h)
{
	QuadTree	*pQT	=malloc(sizeof(QuadTree));

	memset(pQT, 0, sizeof(QuadTree));

	BoundVerts(pVerts, w * h, pQT->mMins, pQT->mMaxs);

	pQT->mpRoot	=QN_Build(pVerts, w * h, pQT->mMins, pQT->mMaxs);

	QN_FixBoxHeights(pQT->mpRoot);

	return	pQT;
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
					vec3 intersection, vec3 hitNorm)
{
	//convert to a ray format
	vec3	rayDir;

	glm_vec3_sub(end, start, rayDir);

	float	rayLen	=glm_vec3_norm(rayDir);

	glm_vec3_scale(rayDir, 1.0f / rayLen, rayDir);

	vec3	invDir;
	SSE_ReciprocalVec3(rayDir, invDir);

	vec3	bounds[2];
	glm_vec3_copy(pQT->mMins, bounds[0]);
	glm_vec3_copy(pQT->mMaxs, bounds[1]);

	//check against bounds encompassing entire tree
	if(!RayIntersectBounds(start, invDir, rayLen, bounds))
	{
		return	MISS;
	}	

	return	QN_LineIntersect(pQT->mpRoot, start, invDir, rayLen, intersection, hitNorm);
}