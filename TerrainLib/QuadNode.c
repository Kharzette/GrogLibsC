#include	<stdint.h>
#include	<utstring.h>
#include	<utlist.h>
#include	<cglm/call.h>
#include	<stdio.h>
#include	<string.h>
#include	<assert.h>
#include	"../UtilityLib/FileStuff.h"
#include	"../UtilityLib/MiscStuff.h"
#include	"../UtilityLib/PlaneMath.h"
#include	"../UtilityLib/ConvexVolume.h"
#include	"QuadNode.h"
#include	"Terrain.h"

#define	TOO_THIN	0.1f	//minimum vertical box thickness?
#define	NODE_SIZE	2		//size in meters
#define	LEAF_QUADS	(NODE_SIZE * NODE_SIZE)
#define	LEAF_TRIS	LEAF_QUADS * 2

typedef struct	CVList_t
{
	ConvexVolume	*mpCV;

	struct CVList_t	*next;
}	CVList;

typedef struct	QuadNode_t
{
	vec3	mMins, mMaxs;		//bounds

	//quadrants abcd below
	QuadNode	*mpChildA;
	QuadNode	*mpChildB;
	QuadNode	*mpChildC;
	QuadNode	*mpChildD;

	//convex volumes for leafs
	CVList		*mpCVs;
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

	glm_vec3_add(mins, maxs, middle);
	glm_vec3_scale(middle, 0.5f, middle);

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

static int	LineIntersectLeafNode(const QuadNode *pQN, const vec3 start, const vec3 end,
									vec3 intersection, vec4 planeHit)
{
	assert(pQN->mpCVs);

	//invalidate
	glm_vec3_fill(intersection, FLT_MAX);

	CVList	*pCur	=NULL;
	int		ret		=-1;

	LL_FOREACH(pQN->mpCVs, pCur)
	{
		vec3	hit;
		vec4	hitPlane;

		int	res	=CV_LineIntersectVolume(pCur->mpCV, start, end, hit, hitPlane);
		if(res == VOL_MISS)
		{
			continue;
		}

		//completely contained within one volume?
		if(res == VOL_INSIDE)
		{
			return	VOL_INSIDE;
		}

		//favour visible surface hits
		if(res & VOL_HIT_VISIBLE)
		{
			float	hitDist		=glm_vec3_distance2(start, hit);
			float	prevHitDist	=glm_vec3_distance2(start, intersection);
			if(hitDist < prevHitDist)
			{
				glm_vec3_copy(hit, intersection);
				glm_vec4_copy(hitPlane, planeHit);
				ret	=res;
			}
		}
	}
	return	ret;
}

static void	MakeLeafVolumes(QuadNode *pQN, const float *pHeights, int w, int h)
{
	pQN->mpCVs	=NULL;

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

	//these really should probably be square
	assert(xSize == zSize);

	int	numQuads	=(xSize - 1) * (zSize - 1);
	int	numTris		=numQuads * 2;

	//make sure 8 bits is good enough
	assert((numTris * 3) < 256);

	//height triangles
	vec3	lvArray[xSize * zSize];
	int		inds[numTris * 3];

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

			lvArray[curDest][0]	=x;
			lvArray[curDest][1]	=height;
			lvArray[curDest][2]	=z;

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
	int	vIdx	=0;
	for(int i=0;i < numQuads;i++)
	{
		inds[curIdx++]	=vIdx;
		inds[curIdx++]	=vIdx + 1;
		inds[curIdx++]	=vIdx + xSize;

		inds[curIdx++]	=vIdx + 1;
		inds[curIdx++]	=vIdx + xSize + 1;
		inds[curIdx++]	=vIdx + xSize;

		//watch for quad boundary
		if(vIdx == (xSize - 2))
		{
			vIdx	+=2;
		}
		else
		{
			vIdx++;
		}
	}

	//build a volume from each triangle
	for(int i=0;i < curIdx;i+=3)
	{
		vec3	tri[3];

		glm_vec3_copy(lvArray[inds[i + 0]], tri[0]);
		glm_vec3_copy(lvArray[inds[i + 1]], tri[1]);
		glm_vec3_copy(lvArray[inds[i + 2]], tri[2]);

		ConvexVolume	*pCV	=CV_MakeFromTri(tri, minHeight);
		if(pCV != NULL)
		{
			CVList	*pCVL	=malloc(sizeof(CVList));

			pCVL->mpCV	=pCV;
			pCVL->next	=NULL;

			LL_PREPEND(pQN->mpCVs, pCVL);
		}
	}
}


QuadNode	*QN_Build(float *pHeights, int w, int h, const vec3 mins, const vec3 maxs)
{
	QuadNode	*pNode	=malloc(sizeof(QuadNode));

	memset(pNode, 0, sizeof(QuadNode));

	//copy bounds
	memcpy(pNode->mMins, mins, sizeof(vec3));
	memcpy(pNode->mMaxs, maxs, sizeof(vec3));

	int	nodeSize	=Misc_SSE_RoundFToI(maxs[0] - mins[0]);

	if(nodeSize <= NODE_SIZE)
	{
		//leaf
		MakeLeafVolumes(pNode, pHeights, w, h);

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
	if(pQN->mpCVs)
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
	if(pQN->mpCVs)
	{
		glm_vec3_copy(pQN->mMins, pMins[*pIndex]);
		glm_vec3_copy(pQN->mMaxs, pMaxs[*pIndex]);
		(*pIndex)++;
		return;
	}

	QN_GatherLeafBounds(pQN->mpChildA, pMins, pMaxs, pIndex);
	QN_GatherLeafBounds(pQN->mpChildB, pMins, pMaxs, pIndex);
	QN_GatherLeafBounds(pQN->mpChildC, pMins, pMaxs, pIndex);
	QN_GatherLeafBounds(pQN->mpChildD, pMins, pMaxs, pIndex);
}


//invDir should be 1 divided by the unit direction vector
int	QN_LineIntersect(const QuadNode *pQN, const vec3 rayStart, const vec3 end,
						const vec3 invDir, const float rayLen,
						vec3 intersection, vec4 planeHit)
{
	//broad phase
//	if(!Misc_BPIntersectLineAABB(rayStart, end, pQN->mMins, pQN->mMaxs))
//	{
//		return	false;
//	}

	vec3	bounds[2];
	glm_vec3_copy(pQN->mMins, bounds[0]);
	glm_vec3_copy(pQN->mMaxs, bounds[1]);

	//fast intersect check for nodes
	//don't need plane hit or point returned, just a yes or no
	if(!Misc_RayIntersectBounds(rayStart, invDir, rayLen, bounds))
	{
		return	VOL_MISS;
	}

	if(pQN->mpCVs)	//leaf?
	{
		vec3	hit;
		vec4	hitPlane;
		int	res	=LineIntersectLeafNode(pQN, rayStart, end, hit, hitPlane);

		if(res == VOL_INSIDE)
		{
			//line fully contained in solid space
			return	VOL_INSIDE;
		}

		if(res != VOL_MISS)
		{
			//can check squared distance for comparison
			float	curDist	=glm_vec3_distance2(rayStart, intersection);
			float	newDist	=glm_vec3_distance2(rayStart, hit);

			//new hit nearer than previous hits?
			if(newDist < curDist)
			{
				glm_vec3_copy(hit, intersection);
				glm_vec4_copy(hitPlane, planeHit);
				return	res;	//some form of hit
			}
		}
		return	VOL_MISS;	//too distant
	}

	int	ret	=QN_LineIntersect(pQN->mpChildA, rayStart, end, invDir, rayLen, intersection, planeHit);
	if(ret == VOL_INSIDE)	//early exit check
	{
		return	VOL_INSIDE;
	}

	int	ret2	=QN_LineIntersect(pQN->mpChildB, rayStart, end, invDir, rayLen, intersection, planeHit);
	if(ret2 == VOL_INSIDE)	//early exit check
	{
		return	VOL_INSIDE;
	}
	else if(ret2 != VOL_MISS)
	{
		ret	=ret2;	//newer hits supercede old
	}

	ret2	=QN_LineIntersect(pQN->mpChildC, rayStart, end, invDir, rayLen, intersection, planeHit);
	if(ret2 == VOL_INSIDE)	//early exit check
	{
		return	VOL_INSIDE;
	}
	else if(ret2 != VOL_MISS)
	{
		ret	=ret2;	//newer hits supercede old
	}

	ret2	=QN_LineIntersect(pQN->mpChildD, rayStart, end, invDir, rayLen, intersection, planeHit);
	if(ret2 == VOL_INSIDE)	//early exit check
	{
		return	VOL_INSIDE;
	}
	else if(ret2 != VOL_MISS)
	{
		ret	=ret2;	//newer hits supercede old
	}

	return	ret;
}

//invDir should be 1 divided by the direction vector
//This is a swept sphere around a ray
//see https://www.gamedeveloper.com/programming/bsp-collision-detection-as-used-in-mdk2-and-neverwinter-nights
//for problems related to corners
bool	QN_LineSphereIntersect(const QuadNode *pQN, const vec3 rayStart, const vec3 end,
								const vec3 invDir, float radius, float rayLen,
								vec3 intersection, vec4 planeHit)
{
	//not ready to do this one yet
	return	false;

	vec3	bounds[2];
	glm_vec3_copy(pQN->mMins, bounds[0]);
	glm_vec3_copy(pQN->mMaxs, bounds[1]);

	//expand by radius
	Misc_ExpandBounds(bounds[0], bounds[1], radius);

	//broad phase
//	if(!Misc_BPIntersectLineAABB(rayStart, end, pQN->mMins, pQN->mMaxs))
//	{
//		return	false;
//	}

	//fast intersect check for nodes
	//don't need plane hit or point returned, just a yes or no
	if(!Misc_RayIntersectBounds(rayStart, invDir, rayLen, bounds))
	{
		return	false;
	}

	if(pQN->mpCVs)	//leaf?
	{
		vec3	hit;
		vec4	hitPlane;
		if(LineIntersectLeafNodeGLM(pQN, rayStart, end, hit, hitPlane))
		{
			//can check squared distance for comparison
			float	curDist	=glm_vec3_distance2(rayStart, intersection);
			float	newDist	=glm_vec3_distance2(rayStart, hit);

			//new hit nearer than previous hits?
			if(newDist < curDist)
			{
				glm_vec3_copy(hit, intersection);
				glm_vec4_copy(hitPlane, planeHit);
				return	true;
			}
		}
		return	false;
	}

	bool	ret	=QN_LineSphereIntersect(pQN->mpChildA, rayStart, end, invDir, radius, rayLen, intersection, planeHit);
	ret			|=QN_LineSphereIntersect(pQN->mpChildB, rayStart, end, invDir, radius, rayLen, intersection, planeHit);
	ret			|=QN_LineSphereIntersect(pQN->mpChildC, rayStart, end, invDir, radius, rayLen, intersection, planeHit);
	ret			|=QN_LineSphereIntersect(pQN->mpChildD, rayStart, end, invDir, radius, rayLen, intersection, planeHit);

	return	ret;
}