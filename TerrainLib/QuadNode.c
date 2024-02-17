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
	vec3	mMins, mMaxs;		//bounds

	//quadrants abcd below
	QuadNode	*mpChildA;
	QuadNode	*mpChildB;
	QuadNode	*mpChildC;
	QuadNode	*mpChildD;

	TerrainVert	*mpHeights;			//null unless leaf

}	QuadNode;


//split mins and maxs evenly through x and z
//return the resulting 4 bounds
//    ←X→
// *   *   *
//
//   C   D    
//↑         ↑
//Z*   *   *Z
//↓         ↓
//   B   A   
//
// *   *   *org
//    ←X→
static void	SplitBound(const vec3 mins, const vec3 maxs,
		vec3 aMins, vec3 aMaxs,
		vec3 bMins, vec3 bMaxs,
		vec3 cMins, vec3 cMaxs,
		vec3 dMins, vec3 dMaxs)
{
	vec3	middle;

	glmc_vec3_add(mins, maxs, middle);
	glmc_vec3_scale(middle, 0.5f, middle);

	//copy initial values
	for(int i=0;i < 3;i++)
	{
		aMins[i]	=mins[i];
		aMaxs[i]	=maxs[i];
		
		bMins[i]	=mins[i];
		bMaxs[i]	=maxs[i];
		
		dMins[i]	=mins[i];
		dMaxs[i]	=maxs[i];
		
		cMins[i]	=mins[i];
		cMaxs[i]	=maxs[i];		
	}

	//quadrant A max x and z to the midpoint
	aMaxs[0]	=middle[0];
	aMaxs[2]	=middle[2];

	//quadrant C min x and z to the midpoint
	cMins[0]	=middle[0];
	cMins[2]	=middle[2];

	//quadrant D max x and min z to the midpoint
	dMaxs[0]	=middle[0];
	dMins[2]	=middle[2];

	//quadrant B min x and max z to the midpoint
	bMins[0]	=middle[0];
	bMaxs[2]	=middle[2];
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
		QN_FixBoxHeights(pQN->mpChildB);
		QN_FixBoxHeights(pQN->mpChildA);
		QN_FixBoxHeights(pQN->mpChildD);
		QN_FixBoxHeights(pQN->mpChildC);
		return;
	}

	//recompute height of bounds
	pQN->mMins[1]	=FLT_MAX;
	pQN->mMaxs[1]	=-FLT_MAX;

	//see how many heights in each side
	float	xSide	=pQN->mMaxs[0] - pQN->mMins[0] + 1.0f;
	float	zSide	=pQN->mMaxs[2] - pQN->mMins[2] + 1.0f;

	int	ixLen	=(int)xSide;
	int	izLen	=(int)zSide;

	for(int z=0;z < izLen;z++)
	{
		for(int x=0;x < ixLen;x++)
		{
			float	height	=pQN->mpHeights[(z * izLen) + x].mPosition[1];

			if(height < pQN->mMins[1])
			{
				pQN->mMins[1]	=height;
			}
			if(height > pQN->mMaxs[1])
			{
				pQN->mMaxs[1]	=height;
			}
		}
	}

	//check for superflat boxes, those are problematic
	float	yDim	=pQN->mMaxs[1] - pQN->mMins[1];
	if(yDim < TOO_THIN)
	{
		float	amount	=TOO_THIN - yDim;

		amount	*=0.5f;

		//inflate the box a little bit
		pQN->mMaxs[1]	+=amount;
		pQN->mMins[1]	-=amount;
	}
}


QuadNode	*QN_Build(TerrainVert *pVerts, int count, const vec3 mins, const vec3 maxs)
{
	QuadNode	*pNode	=malloc(sizeof(QuadNode));

	memset(pNode, 0, sizeof(QuadNode));

	//copy bounds
	for(int i=0;i < 3;i++)
	{
		pNode->mMins[i]	=mins[i];
		pNode->mMaxs[i]	=maxs[i];
	}

	if(count <= 62)
	{
		//leaf
		pNode->mpHeights	=pVerts;

		return	pNode;
	}

	TerrainVert	*pA, *pB, *pC, *pD;
	int			numA, numB, numC, numD;

	vec3	aMins, aMaxs, bMins, bMaxs, cMins, cMaxs, dMins, dMaxs;

	SplitBound(mins, maxs, aMins, aMaxs, bMins, bMaxs, cMins, cMaxs, dMins, dMaxs);

	GetBoundedHeights(pVerts, count, aMins, aMaxs, &pA, &numA);
	GetBoundedHeights(pVerts, count, bMins, bMaxs, &pB, &numB);
	GetBoundedHeights(pVerts, count, cMins, cMaxs, &pC, &numC);
	GetBoundedHeights(pVerts, count, dMins, dMaxs, &pD, &numD);

	pNode->mpChildA	=QN_Build(pA, numA, aMins, aMaxs);
	pNode->mpChildB	=QN_Build(pB, numB, bMins, bMaxs);
	pNode->mpChildC	=QN_Build(pC, numC, cMins, cMaxs);
	pNode->mpChildD	=QN_Build(pD, numD, dMins, dMaxs);

	//after returning from recursion, free non leaf vert data
	if(pNode->mpChildA->mpHeights == NULL)
	{
		free(pA);
	}
	if(pNode->mpChildB->mpHeights == NULL)
	{
		free(pB);
	}
	if(pNode->mpChildC->mpHeights == NULL)
	{
		free(pC);
	}
	if(pNode->mpChildD->mpHeights == NULL)
	{
		free(pD);
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

	QN_CountLeafBounds(pQN->mpChildA, pNumBounds);
	QN_CountLeafBounds(pQN->mpChildB, pNumBounds);
	QN_CountLeafBounds(pQN->mpChildC, pNumBounds);
	QN_CountLeafBounds(pQN->mpChildD, pNumBounds);
}


void	QN_GatherLeafBounds(const QuadNode *pQN, vec3 *pMins, vec3 *pMaxs, int *pIndex)
{
	if(pQN->mpHeights != NULL)
	{
		glmc_vec3_copy(pQN->mMins, pMins[*pIndex]);
		glmc_vec3_copy(pQN->mMaxs, pMaxs[*pIndex]);
		(*pIndex)++;
		return;
	}

	QN_GatherLeafBounds(pQN->mpChildA, pMins, pMaxs, pIndex);
	QN_GatherLeafBounds(pQN->mpChildB, pMins, pMaxs, pIndex);
	QN_GatherLeafBounds(pQN->mpChildC, pMins, pMaxs, pIndex);
	QN_GatherLeafBounds(pQN->mpChildD, pMins, pMaxs, pIndex);
}


int	QN_LineIntersect(const QuadNode *pQN, const vec3 start, const vec3 end,
					vec3 intersection, vec3 hitNorm)
{
	if(pQN->mpHeights != NULL)
	{
		vec3	hit, hitN;
		int	res	=LineIntersectBounds(pQN->mMins, pQN->mMaxs, start, end, hit, hitN);
		if(res == MISS)
		{
			return	res;
		}

		float	hitNormLen	=glm_vec3_norm(hitNorm);
		if(hitNormLen > 1.001 || hitNormLen < 0.999)
		{
			//no hits yet
			glm_vec3_copy(hit, intersection);
			glm_vec3_copy(hitN, hitNorm);
			return	res;
		}

		float	curDist	=glm_vec3_distance(start, intersection);
		float	newDist	=glm_vec3_distance(start, hit);

		//new hit nearer than previous hits?
		if(newDist < curDist)
		{
			glm_vec3_copy(hit, intersection);
			glm_vec3_copy(hitN, hitNorm);
			return	res;
		}		
		return	MISS;
	}

	int	ret	=QN_LineIntersect(pQN->mpChildA, start, end, intersection, hitNorm);
	ret		|=QN_LineIntersect(pQN->mpChildB, start, end, intersection, hitNorm);
	ret		|=QN_LineIntersect(pQN->mpChildC, start, end, intersection, hitNorm);
	ret		|=QN_LineIntersect(pQN->mpChildD, start, end, intersection, hitNorm);

	return	ret;
}
