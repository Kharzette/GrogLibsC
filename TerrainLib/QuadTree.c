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


int	QT_LineIntersect(const QuadTree *pQT, const vec3 start, const vec3 end,
					vec3 intersection, vec3 hitNorm)
{
	int	res	=LineIntersectBounds(pQT->mMins, pQT->mMaxs, start, end, intersection, hitNorm);
	if(res == MISS)
	{
		return	res;
	}

	//reset intersection
	intersection[0]	=intersection[1]	=intersection[2]	=FLT_MAX;

	return	QN_LineIntersect(pQT->mpRoot, start, end, intersection, hitNorm);
}