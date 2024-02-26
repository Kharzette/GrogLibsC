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

	vec3	*mpLVArray;		//leaf vert array
	uint8_t	*mpLVInds;		//leaf triangles
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

static int	LineIntersectLeafNode(const QuadNode *pQN, const vec3 start, const vec3 end)
{
	assert(pQN->mpLVArray);
	assert(pQN->mpLVInds);

}

static void	MakeLeafTris(QuadNode *pQN, const float *pHeights, int w, int h)
{
	int	xSize	=Misc_SSE_RoundFToI(pQN->mMaxs[0] - pQN->mMins[0]);
	int	zSize	=Misc_SSE_RoundFToI(pQN->mMaxs[2] - pQN->mMins[2]);

	int	xStart	=Misc_SSE_RoundFToI(pQN->mMins[0]);
	int	zStart	=Misc_SSE_RoundFToI(pQN->mMins[2]);

	int	xEnd	=Misc_SSE_RoundFToI(pQN->mMaxs[0]);
	int	zEnd	=Misc_SSE_RoundFToI(pQN->mMaxs[2]);

	//bounds are right on vert boundaries
	//so need a +1
	xSize++;
	zSize++;

	//these really should be square
	assert(xSize == zSize);

	int	numQuads	=(xSize - 1) * (zSize - 1);
	int	numTris		=numQuads * 2;

	//make sure 8 bits is good enough
	assert((numTris * 3) < 256);

	//alloc
	pQN->mpLVArray	=malloc(sizeof(vec3) * xSize * zSize);
	pQN->mpLVInds	=malloc(numTris * 3);

	//watch height values while copying
	float	minHeight	=FLT_MAX;
	float	maxHeight	=-FLT_MAX;

	//copy heights
	int	curDest	=0;
	for(int z=zStart;z <= zEnd;z++)
	{
		for(int x=xStart;x <= xEnd;x++)
		{
			int	ofs	=(z * w) + x;

			float	height	=pHeights[ofs];

			pQN->mpLVArray[curDest][0]	=x;
			pQN->mpLVArray[curDest][1]	=height;
			pQN->mpLVArray[curDest][2]	=z;

			curDest++;

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

	//check for superflat 
	float	yDim	=maxHeight - minHeight;
	if(yDim < TOO_THIN)
	{
		float	amount	=TOO_THIN - yDim;

		amount	*=0.5f;

		//inflate the box a little bit
		minHeight	-=amount;
		maxHeight	+=amount;
	}

	//fix bound heights
	pQN->mMins[1]	=minHeight;
	pQN->mMaxs[1]	=maxHeight;

	//index triangles
	int	curIdx	=0;
	for(int i=0;i < numQuads;i++)
	{
		pQN->mpLVInds[curIdx++]	=i;
		pQN->mpLVInds[curIdx++]	=i + 1;
		pQN->mpLVInds[curIdx++]	=i + xSize;

		pQN->mpLVInds[curIdx++]	=i + 1;
		pQN->mpLVInds[curIdx++]	=i + xSize + 1;
		pQN->mpLVInds[curIdx++]	=i;
	}
}


QuadNode	*QN_Build(float *pHeights, int w, int h, const vec3 mins, const vec3 maxs)
{
	QuadNode	*pNode	=malloc(sizeof(QuadNode));

	memset(pNode, 0, sizeof(QuadNode));

	//copy bounds
	memcpy(pNode->mMins, mins, sizeof(vec3));
	memcpy(pNode->mMaxs, maxs, sizeof(vec3));

	int	sv3	=sizeof(vec3);

	int	nodeSize	=Misc_SSE_RoundFToI(maxs[0] - mins[0]) + 1.0f;

	if(nodeSize <= 8)
	{
		//leaf
		MakeLeafTris(pNode, pHeights, w, h);

		return	pNode;
	}

	vec3	aMins, aMaxs, bMins, bMaxs, cMins, cMaxs, dMins, dMaxs;

	SplitBound(mins, maxs, aMins, aMaxs, bMins, bMaxs, cMins, cMaxs, dMins, dMaxs);

	pNode->mpChildA	=QN_Build(pHeights, w, h, aMins, aMaxs);
	pNode->mpChildB	=QN_Build(pHeights, w, h, bMins, bMaxs);
	pNode->mpChildC	=QN_Build(pHeights, w, h, cMins, cMaxs);
	pNode->mpChildD	=QN_Build(pHeights, w, h, dMins, dMaxs);

	return	pNode;
}


void	QN_CountLeafBounds(const QuadNode *pQN, int *pNumBounds)
{
	if(pQN->mpLVArray)
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
	if(pQN->mpLVArray)
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


//invDir should be 1 divided by the direction vector
int	QN_LineIntersect(const QuadNode *pQN,
					const vec3 rayStart, const vec3 end, const vec3 invDir, const float rayLen,
					vec3 intersection, vec3 hitNorm)
{
	vec3	bounds[2];
	glm_vec3_copy(pQN->mMins, bounds[0]);
	glm_vec3_copy(pQN->mMaxs, bounds[1]);

	int	res	=MISS;

	//fast intersect check for nodes
	//don't need plane hit or point returned, just a yes or no
	if(!Misc_RayIntersectBounds(rayStart, invDir, rayLen, bounds))
	{
		return	res;
	}

	if(pQN->mpLVArray)	//leaf?
	{
		vec3	hit, hitN;
		res	=Misc_LineIntersectBounds(pQN->mMins, pQN->mMaxs, rayStart, end, hit, hitN);
		if(res == MISS)
		{
			//if a miss, can safely assume all child nodes are a miss
			return	res;
		}

		//can check squared distance for comparison
		float	curDist	=glm_vec3_distance2(rayStart, intersection);
		float	newDist	=glm_vec3_distance2(rayStart, hit);

		//new hit nearer than previous hits?
		if(newDist < curDist)
		{
			glm_vec3_copy(hit, intersection);
			glm_vec3_copy(hitN, hitNorm);
			return	res;
		}		
		return	MISS;
	}

	int	ret	=QN_LineIntersect(pQN->mpChildA, rayStart, end, invDir, rayLen, intersection, hitNorm);
	ret		|=QN_LineIntersect(pQN->mpChildB, rayStart, end, invDir, rayLen, intersection, hitNorm);
	ret		|=QN_LineIntersect(pQN->mpChildC, rayStart, end, invDir, rayLen, intersection, hitNorm);
	ret		|=QN_LineIntersect(pQN->mpChildD, rayStart, end, invDir, rayLen, intersection, hitNorm);

	return	ret;
}

//slower trace with the convex volume routine
int	QN_LineIntersectCV(const QuadNode *pQN, const vec3 rayStart, const vec3 end,
						vec3 intersection, vec3 hitNorm)
{
	int		res	=MISS;
	vec3	hit, hitN;

	//first check node
	res	=Misc_LineIntersectBounds(pQN->mMins, pQN->mMaxs, rayStart, end, hit, hitN);
	if(res == MISS)
	{
		return	res;
	}

	if(pQN->mpLVArray)	//leaf?
	{
		//can check squared distance for comparison
		float	curDist	=glm_vec3_distance2(rayStart, intersection);
		float	newDist	=glm_vec3_distance2(rayStart, hit);

		//new hit nearer than previous hits?
		if(newDist < curDist)
		{
			glm_vec3_copy(hit, intersection);
			glm_vec3_copy(hitN, hitNorm);
			return	res;
		}		
		return	MISS;
	}

	int	ret	=QN_LineIntersectCV(pQN->mpChildA, rayStart, end, intersection, hitNorm);
	ret		|=QN_LineIntersectCV(pQN->mpChildB, rayStart, end, intersection, hitNorm);
	ret		|=QN_LineIntersectCV(pQN->mpChildC, rayStart, end, intersection, hitNorm);
	ret		|=QN_LineIntersectCV(pQN->mpChildD, rayStart, end, intersection, hitNorm);

	return	ret;
}
