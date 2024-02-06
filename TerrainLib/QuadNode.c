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

//minimum vertical box thickness?
#define	TOO_THIN	0.1f

typedef struct	QuadNode_t	QuadNode;

typedef struct	QuadNode_t
{
	vec3	mBoxMins, mBoxMaxs;		//bounds

	QuadNode	*mpChildNW;			//northwest
	QuadNode	*mpChildNE;			//northeast
	QuadNode	*mpChildSW;			//southwest
	QuadNode	*mpChildSE;			//southeast

	TerrainVert	*mpHeights;			//null unless leaf

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

	//southwest max x and min z to the midpoint
	swMaxs[0]	=middle[0];
	swMins[2]	=middle[2];

	//northeast min x and max z to the midpoint
	neMins[0]	=middle[0];
	neMaxs[2]	=middle[2];
}

static void	SplitHeights(const TerrainVert *pVerts, int w, const vec3 mins, const vec3 maxs,
	TerrainVert **pNW, TerrainVert **pNE, TerrainVert **pSW, TerrainVert **pSE)
{
	//see how many heights in each side
	float	width	=maxs[0] - mins[0] + 1.0f;
	float	height	=maxs[2] - mins[2] + 1.0f;

	//how many heights go into the split data
	int	halfW	=roundf(width / 2.0f);
	int	halfH	=roundf(height / 2.0f);

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

		memcpy(&((*pNE)[(y * halfW)]), &pVerts[(y * w) + halfW - 1], sizeof(TerrainVert) * halfW);

		memcpy(&((*pSW)[(y * halfW)]), &pVerts[((y + halfH) * w)], sizeof(TerrainVert) * halfW);

		memcpy(&((*pSE)[(y * halfW)]), &pVerts[((y + halfH) * w) + halfW - 1], sizeof(TerrainVert) * halfW);
	}
}

static void	BoundVerts(const TerrainVert *pVerts, int numVerts, vec3 mins, vec3 maxs)
{
	assert(numVerts > 0);

	ClearBounds(mins, maxs);

	for(int i=0;i < numVerts;i++)
	{
		AddPointToBoundingBox(mins, maxs, pVerts[i].mPosition);
	}
}


//return the verts within the bounds provided
static void	GetBoundedHeights(const TerrainVert *pVerts, int numVerts, vec3 mins, vec3 maxs,
								TerrainVert **ppBounded, int *numBounded)
{
	*numBounded	=0;
	for(int i=0;i < numVerts;i++)
	{
		if(IsPointInBounds(mins, maxs, pVerts[i].mPosition))
		{
			*numBounded	=(*numBounded) + 1;
		}
	}

	*ppBounded	=malloc(sizeof(TerrainVert) * (*numBounded));

	//lazy
	int	idx	=0;
	for(int i=0;i < numVerts;i++)
	{
		if(IsPointInBounds(mins, maxs, pVerts[i].mPosition))
		{
			memcpy(&((*ppBounded)[idx]), &pVerts[i], sizeof(TerrainVert));
			idx++;
		}
	}
}

//quadtrees are essentially 2D, and are assumed convex from a vertical
//point of view, so during splitting the Y axis is kind of ignored, and
//needs to be smooshed later.
void	QN_FixBoxHeights(QuadNode *pQN)
{
	if(pQN->mpHeights == NULL)
	{
		QN_FixBoxHeights(pQN->mpChildNE);
		QN_FixBoxHeights(pQN->mpChildNW);
		QN_FixBoxHeights(pQN->mpChildSE);
		QN_FixBoxHeights(pQN->mpChildSW);
		return;
	}

	//recompute height of bounds
	pQN->mBoxMins[1]	=FLT_MAX;
	pQN->mBoxMaxs[1]	=-FLT_MAX;

	//see how many heights in each side
	float	xSide	=pQN->mBoxMaxs[0] - pQN->mBoxMins[0] + 1.0f;
	float	zSide	=pQN->mBoxMaxs[2] - pQN->mBoxMins[2] + 1.0f;

	int	ixLen	=(int)xSide;
	int	izLen	=(int)zSide;

	for(int z=0;z < izLen;z++)
	{
		for(int x=0;x < ixLen;x++)
		{
			float	height	=pQN->mpHeights[(z * izLen) + x].mPosition[1];

			if(height < pQN->mBoxMins[1])
			{
				pQN->mBoxMins[1]	=height;
			}
			if(height > pQN->mBoxMaxs[1])
			{
				pQN->mBoxMaxs[1]	=height;
			}
		}
	}

	//check for superflat boxes, those are problematic
	float	yDim	=pQN->mBoxMaxs[1] - pQN->mBoxMins[1];
	if(yDim < TOO_THIN)
	{
		float	amount	=TOO_THIN - yDim;

		amount	*=0.5f;

		//inflate the box a little bit
		pQN->mBoxMaxs[1]	+=amount;
		pQN->mBoxMins[1]	-=amount;
	}
}


QuadNode	*QN_Build(TerrainVert *pVerts, int count, const vec3 mins, const vec3 maxs)
{
	QuadNode	*pNode	=malloc(sizeof(QuadNode));

	memset(pNode, 0, sizeof(QuadNode));

	//copy bounds
	for(int i=0;i < 3;i++)
	{
		pNode->mBoxMins[i]	=mins[i];
		pNode->mBoxMaxs[i]	=maxs[i];
	}

	if(count <= 62)
	{
		//leaf
		pNode->mpHeights	=pVerts;

		return	pNode;
	}

	TerrainVert	*pNW, *pNE, *pSW, *pSE;
	int			numNW, numNE, numSW, numSE;

	vec3	nwMins, nwMaxs, neMins, neMaxs, swMins, swMaxs, seMins, seMaxs;

	SplitBound(mins, maxs, nwMins, nwMaxs, neMins, neMaxs, swMins, swMaxs, seMins, seMaxs);

	GetBoundedHeights(pVerts, count, nwMins, nwMaxs, &pNW, &numNW);
	GetBoundedHeights(pVerts, count, neMins, neMaxs, &pNE, &numNE);
	GetBoundedHeights(pVerts, count, swMins, swMaxs, &pSW, &numSW);
	GetBoundedHeights(pVerts, count, seMins, seMaxs, &pSE, &numSE);

//	SplitHeights(pVerts, w, mins, maxs, &pNW, &pNE, &pSW, &pSE);

	pNode->mpChildNW	=QN_Build(pNW, numNW, nwMins, nwMaxs);
	pNode->mpChildNE	=QN_Build(pNE, numNE, neMins, neMaxs);
	pNode->mpChildSW	=QN_Build(pSW, numSW, swMins, swMaxs);
	pNode->mpChildSE	=QN_Build(pSE, numSE, seMins, seMaxs);

	//after returning from recursion, free non leaf vert data
	if(pNode->mpChildNW->mpHeights == NULL)
	{
		free(pNW);
	}
	if(pNode->mpChildNE->mpHeights == NULL)
	{
		free(pNE);
	}
	if(pNode->mpChildSW->mpHeights == NULL)
	{
		free(pSW);
	}
	if(pNode->mpChildSE->mpHeights == NULL)
	{
		free(pSE);
	}

	return	pNode;
}


void	QN_CountLeafBounds(const QuadNode *pQN, int *pNumBounds)
{
	if(pQN->mpHeights != NULL)
	{
		(*pNumBounds)++;
		return;
	}

	QN_CountLeafBounds(pQN->mpChildNW, pNumBounds);
	QN_CountLeafBounds(pQN->mpChildNE, pNumBounds);
	QN_CountLeafBounds(pQN->mpChildSW, pNumBounds);
	QN_CountLeafBounds(pQN->mpChildSE, pNumBounds);
}


void	QN_GatherLeafBounds(const QuadNode *pQN, vec3 *pMins, vec3 *pMaxs, int *pIndex)
{
	if(pQN->mpHeights != NULL)
	{
		glmc_vec3_copy(pQN->mBoxMins, pMins[*pIndex]);
		glmc_vec3_copy(pQN->mBoxMaxs, pMaxs[*pIndex]);
		(*pIndex)++;
		return;
	}

	QN_GatherLeafBounds(pQN->mpChildNW, pMins, pMaxs, pIndex);
	QN_GatherLeafBounds(pQN->mpChildNE, pMins, pMaxs, pIndex);
	QN_GatherLeafBounds(pQN->mpChildSW, pMins, pMaxs, pIndex);
	QN_GatherLeafBounds(pQN->mpChildSE, pMins, pMaxs, pIndex);
}