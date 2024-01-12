#include	<stdint.h>
#include	<utstring.h>
#include	<cglm/call.h>
#include	<stdio.h>
#include	<string.h>
#include	<assert.h>
#include	"../UtilityLib/FileStuff.h"
#include	"../UtilityLib/MiscStuff.h"
#include	"QuadNode.h"
#include	"Terrain.h"


typedef struct	QuadNode_t	QuadNode;

typedef struct	QuadNode_t
{
	vec3	mBoxMins, mBoxMaxs;		//bounds

	QuadNode	*mpChildNW;			//northwest
	QuadNode	*mpChildNE;			//northeast
	QuadNode	*mpChildSW;			//southwest
	QuadNode	*mpChildSE;			//southeast

	float	*mpHeights;		//null unless leaf

}	QuadNode;


static void	SplitBound(const vec3 mins, const vec3 maxs,
		vec3 nwMins, vec3 nwMaxs,
		vec3 neMins, vec3 neMaxs,
		vec3 swMins, vec3 swMaxs,
		vec3 seMins, vec3 seMaxs)
{
	vec3	middle;

	glmc_vec3_add(mins, maxs, middle);
	glmc_vec3_scale(middle, 0.5f, middle);

	//copy initial values
	for(int i=0;i < 3;i++)
	{
		nwMins[i]	=mins[i];
		nwMaxs[i]	=maxs[i];
		
		neMins[i]	=mins[i];
		neMaxs[i]	=maxs[i];
		
		swMins[i]	=mins[i];
		swMaxs[i]	=maxs[i];
		
		seMins[i]	=mins[i];
		seMaxs[i]	=maxs[i];		
	}

	//northwest max x and z to the midpoint
	nwMaxs[0]	=middle[0];
	nwMaxs[2]	=middle[2];

	//southeast min x and z to the midpoint
	seMins[0]	=middle[0];
	seMins[2]	=middle[2];

	//southwest max x and z to the midpoint
	swMaxs[0]	=middle[0];
	swMins[2]	=middle[2];

	//northeast min x and max z to the midpoint
	neMins[0]	=middle[0];
	neMaxs[2]	=middle[2];
}

static void	SplitHeights(const TerrainVert *pVerts, int w, int h,
	TerrainVert **pNW, TerrainVert **pNE, TerrainVert **pSW, TerrainVert **pSE)
{
	int	halfW	=w / 2;
	int	halfH	=h / 2;

	//alloc split verts
	*pNW	=malloc(sizeof(TerrainVert) * (halfW * halfH));
	*pNE	=malloc(sizeof(TerrainVert) * (halfW * halfH));
	*pSW	=malloc(sizeof(TerrainVert) * (halfW * halfH));
	*pSE	=malloc(sizeof(TerrainVert) * (halfW * halfH));

	for(int y=0;y < halfH;y++)
	{
		//copy rows
		memcpy(
			&((*pNW)[(y * halfW)]),	//tricksy deref, want the address of an array location
									//from a pointerpointer			
			&pVerts[(y * w)], sizeof(TerrainVert) * halfW);

		memcpy(&((*pNE)[(y * halfW)]), &pVerts[(y * w) + halfW], sizeof(TerrainVert) * halfW);

		memcpy(&((*pSW)[(y * halfW)]), &pVerts[((y + halfH) * w)], sizeof(TerrainVert) * halfW);

		memcpy(&((*pSE)[(y * halfW)]), &pVerts[((y + halfH) * w) + halfW], sizeof(TerrainVert) * halfW);
	}


}


QuadNode	*QN_Build(const TerrainVert *pVerts, int w, int h, const vec3 mins, const vec3 maxs)
{
	if((w * h) <= 4)
	{
		//leaf
		return	NULL;
	}

	QuadNode	*pNode	=malloc(sizeof(QuadNode));

	memset(pNode, 0, sizeof(QuadNode));

	//copy bounds
	for(int i=0;i < 3;i++)
	{
		pNode->mBoxMins[i]	=mins[i];
		pNode->mBoxMaxs[i]	=maxs[i];
	}

	vec3	nwMins, nwMaxs, neMins, neMaxs, swMins, swMaxs, seMins, seMaxs;

	SplitBound(mins, maxs, nwMins, nwMaxs, neMins, neMaxs, swMins, swMaxs, seMins, seMaxs);

	TerrainVert	*pNW, *pNE, *pSW, *pSE;

	SplitHeights(pVerts, w, h, &pNW, &pNE, &pSW, &pSE);

	pNode->mpChildNW	=QN_Build(pNW, (w / 2), (h / 2), nwMins, nwMaxs);
	pNode->mpChildNE	=QN_Build(pNE, (w / 2), (h / 2), neMins, neMaxs);
	pNode->mpChildSW	=QN_Build(pSW, (w / 2), (h / 2), swMins, swMaxs);
	pNode->mpChildSE	=QN_Build(pSE, (w / 2), (h / 2), seMins, seMaxs);

	return	pNode;
}