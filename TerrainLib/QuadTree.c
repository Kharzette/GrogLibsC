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


QuadTree	*QT_Create(const TerrainVert *pVerts, int w, int h)
{
	QuadTree	*pQT	=malloc(sizeof(QuadTree));

	memset(pQT, 0, sizeof(QuadTree));

	BoundVerts(pVerts, w * h, pQT->mMins, pQT->mMaxs);

	QN_Build(pVerts, w, h, pQT->mMins, pQT->mMaxs);
}