#include	<stdint.h>
#include	<string.h>
#include	<assert.h>
#include	<utlist.h>
#include	<cglm/call.h>
#include	"ConvexVolume.h"
#include	"MiscStuff.h"


static const vec4	BoxPlanes[6]	=
{
	{	1, 0, 0, 0	},
	{	0, 1, 0, 0	},
	{	0, 0, 1, 0	},
	{	-1, 0, 0, 0	},
	{	0, -1, 0, 0	},
	{	0, 0, -1, 0	}
};


//return a list of planes
vec4	*CV_AllocConvexVolumeFromBound(const vec3 min, const vec3 max)
{
	vec4	*pRet	=malloc(sizeof(vec4) * 6);

	CV_MakeConvexVolumeFromBound(min, max, pRet);

	return	pRet;
}


//pass in a vec4[6]
void	CV_MakeConvexVolumeFromBound(const vec3 min, const vec3 max, vec4 *pVol)
{
	//copy base box planes
	memcpy(pVol, BoxPlanes, sizeof(vec4) * 6);

	//find plane distances
	pVol[0][3]	=max[0];	
	pVol[1][3]	=max[1];
	pVol[2][3]	=max[2];

	pVol[3][3]	=-min[0];	
	pVol[4][3]	=-min[1];
	pVol[5][3]	=-min[2];
}

static Winding *CopyWinding(const Winding *pW)
{
	Winding	*pRet	=malloc(sizeof(Winding));

	pRet->mNumVerts	=pW->mNumVerts;
	pRet->next		=pW->next;

	pRet->mpVerts	=malloc(sizeof(vec3) * pW->mNumVerts);

	for(int i=0;i < pW->mNumVerts;i++)
	{
		glm_vec3_copy(pW->mpVerts[i], pRet->mpVerts[i]);
	}
	return	pRet;
}

static Winding *ClipWindingBehindPlane(const vec4 plane, const Winding *pW)
{
	vec3	norm	={	plane[0], plane[1], plane[2]	};

	float	pDists[pW->mNumVerts];
	int		fCount	=0;
	int		bCount	=0;

	//check which side verts are on
	for(int i=0;i < pW->mNumVerts;i++)
	{
		pDists[i]	=glm_vec3_dot(pW->mpVerts[i], norm) - plane[3];

		if(pDists[i] > 0.0f)
		{
			fCount++;
		}
		else if(pDists[i] < 0.0f)
		{
			bCount++;
		}
	}

	if(fCount == 0)
	{
		//all behind
		return	CopyWinding(pW);
	}

	if(bCount == 0)
	{
		return	NULL;
	}

	vec3	chops[pW->mNumVerts];
	int		chop	=0;
	for(int i=0;i < pW->mNumVerts;i++)
	{
		if(pDists[i] <= 0.0f)
		{
			glm_vec3_copy(pW->mpVerts[i], chops[chop++]);
		}

		int	nextVert	=(i + 1) % pW->mNumVerts;

		if(pDists[nextVert] == 0.0f)
		{
			continue;
		}

		//same side?
		if(pDists[nextVert] < 0.0f && pDists[i] < 0.0f)
		{
			continue;
		}

		//same side?
		if(pDists[nextVert] > 0.0f && pDists[i] > 0.0f)
		{
			continue;
		}

		//split factor
		float	scale	=pDists[i] / (pDists[i] - pDists[nextVert]);

		vec3	split;
		glm_vec3_sub(pW->mpVerts[nextVert], pW->mpVerts[i], split);
		glm_vec3_scale(split, scale, split);
		glm_vec3_add(pW->mpVerts[i], split, split);

		glm_vec3_copy(split, chops[chop++]);

		assert(chop <= pW->mNumVerts);
	}

	if(chop < 3)
	{
		return	NULL;
	}

	Winding	*pRet	=malloc(sizeof(Winding));

	pRet->mNumVerts	=chop;
	pRet->mpVerts	=malloc(sizeof(vec3) * chop);
	pRet->next		=NULL;

	for(int i=0;i < chop;i++)
	{
		glm_vec3_copy(chops[i], pRet->mpVerts[i]);
	}

	return	pRet;
}

//returns num windings
int	CV_GenerateWindings(const vec4 *pPlanes, int numPlanes, Winding **ppWL)
{
	*ppWL	=NULL;

	//generate huge faces
	int	windCount	=0;
	for(int i=0;i < numPlanes;i++)
	{
		vec3	wind[4];

		Misc_WindingFromPlane(pPlanes[i], wind);

		Winding	*pA	=malloc(sizeof(Winding));

		pA->mNumVerts	=4;
		pA->mpVerts		=malloc(sizeof(vec3) * 4);
		pA->next		=NULL;

		for(int j=0;j < 4;j++)
		{
			glm_vec3_copy(wind[j], pA->mpVerts[j]);
		}

		//clip behind all planes
		for(int j=0;j < numPlanes;j++)
		{
			if(j == i)
			{
				continue;
			}

			Winding	*pNewW	=ClipWindingBehindPlane(pPlanes[j], pA);

			if(pNewW == NULL)
			{
				//fully clipped away
				free(pA);
				break;
			}
			else
			{
				free(pA->mpVerts);
				free(pA);
				pA	=pNewW;
			}
		}

		if(pA == NULL)
		{
			//fully clipped away
			continue;
		}

		LL_PREPEND(*ppWL, pA);
		windCount++;
	}
	return	windCount;
}

//Given a triangle, make a volume with the triangle
//as the top (Y) surface, and bottomY as the -Y
vec4	*CV_MakeFromTri(const vec3 tri[3], float bottomY)
{
	//top Y plane
	vec4	triPlane;
	Misc_PlaneFromTri(tri[0], tri[1], tri[2], triPlane);

	//edge plane 0 -> 1
	vec3	edgeVec;
	glm_vec3_sub(tri[0], tri[1], edgeVec);

	vec3	upVec	={	0.0f, 1.0f, 0.0f	};

	vec3	norm;
	glm_vec3_cross(edgeVec, upVec, norm);

	vec4	plane01	={	norm[0], norm[1], norm[2], 0.0f	};

	plane01[3]	=glm_vec3_dot(norm, tri[0]);

	///edge plane 0 -> 2
	glm_vec3_sub(tri[0], tri[2], edgeVec);

	glm_vec3_cross(upVec, edgeVec, norm);

	vec4	plane02	={	norm[0], norm[1], norm[2], 0.0f	};

	plane02[3]	=glm_vec3_dot(norm, tri[0]);

	///edge plane 1 -> 2
	glm_vec3_sub(tri[1], tri[2], edgeVec);

	glm_vec3_cross(edgeVec, upVec, norm);

	vec4	plane12	={	norm[0], norm[1], norm[2], 0.0f	};

	plane12[3]	=glm_vec3_dot(norm, tri[1]);

	vec4	bottomPlane	={	0.0f, -1.0f, 0.0f, -bottomY	};

	vec4	*pRet	=malloc(sizeof(vec4) * 5);

	//copy generated planes
	glm_vec4_copy(triPlane, pRet[0]);
	glm_vec4_copy(bottomPlane, pRet[1]);
	glm_vec4_copy(plane01, pRet[2]);
	glm_vec4_copy(plane02, pRet[3]);
	glm_vec4_copy(plane12, pRet[4]);

	return	pRet;
}


//chop start->end by plane
//modify start and end to return the front or back side of plane segment
int	CV_ClipLineSegmentToPlane(const vec4 plane, bool bFront, vec3 start, vec3 end)
{
	float	startDist	=glm_vec3_dot(plane, start) - plane[3];
	float	endDist		=glm_vec3_dot(plane, end) - plane[3];

	if(startDist > 0.0f && endDist > 0.0f)
	{
		if(!bFront)
		{
			//zero out segment
			glm_vec3_zero(start);
			glm_vec3_zero(end);
		}
		return	PLANE_FRONT;
	}
	else if(startDist < 0.0f && endDist < 0.0f)
	{
		if(bFront)
		{
			//zero out segment
			glm_vec3_zero(start);
			glm_vec3_zero(end);
		}
		return	PLANE_BACK;
	}

	//get a unit vector of the line segment
	vec3	segVec;
	glm_vec3_sub(end, start, segVec);

	//vector length is named strangely in glm
	float	segLength	=glm_vec3_norm(segVec);

	//normalize
	glm_vec3_scale(segVec, 1.0f / segLength, segVec);

	float	ratio	=startDist / (startDist - endDist);

	vec3	intersection;
	glm_vec3_scale(segVec, ratio * segLength, intersection);

	glm_vec3_add(intersection, start, intersection);

	if(startDist > 0.0f)
	{
		if(bFront)
		{
			glm_vec3_copy(intersection, end);
		}
		else
		{
			glm_vec3_copy(intersection, start);
		}
	}
	else
	{
		if(bFront)
		{
			glm_vec3_copy(intersection, start);
		}
		else
		{
			glm_vec3_copy(intersection, end);
		}
	}

	return	PLANE_HIT;
}


//chop start->end by plane
//modify start and end to return the front or back side of plane segment
int	CV_ClipCapsuleToPlane(const vec4 plane, bool bFront, vec3 start, vec3 end, float radius)
{
	float	startDist	=glm_vec3_dot(plane, start) - plane[3];
	float	endDist		=glm_vec3_dot(plane, end) - plane[3];

	if(startDist > radius && endDist > radius)
	{
		if(!bFront)
		{
			//zero out segment
			glm_vec3_zero(start);
			glm_vec3_zero(end);
		}
		return	PLANE_FRONT;
	}
	else if(startDist < radius && endDist < radius)
	{
		if(bFront)
		{
			//zero out segment
			glm_vec3_zero(start);
			glm_vec3_zero(end);
		}
		return	PLANE_BACK;
	}

	//intersection for sure at this point
	if(bFront)
	{
		//see if either end extends beyond radius
		if(startDist > radius || endDist > radius)
		{
			vec4	bumped;
			glm_vec4_copy(plane, bumped);

			//bump the plane forward
			bumped[3]	+=radius;

			vec3	hit;
			int	res	=Misc_LineIntersectPlane(bumped, start, end, hit);
			assert(res == PLANE_HIT);

			if(startDist > radius)
			{
				//start is the one outside the radius
				glm_vec3_copy(hit, end);
			}
			else
			{
				//end is the one outside the radius
				glm_vec3_copy(hit, start);
			}
			return	PLANE_HIT;
		}
		else
		{
			//fully clipped away
			//zero out segment
			glm_vec3_zero(start);
			glm_vec3_zero(end);

			return	PLANE_BACK;
		}
	}
	else
	{
		//see if either end extends beyond radius
		if(startDist < radius || endDist < radius)
		{
			vec4	bumped;
			glm_vec4_copy(plane, bumped);

			//bump the plane forward
			bumped[3]	+=radius;

			vec3	hit;
			int	res	=Misc_LineIntersectPlane(bumped, start, end, hit);
			assert(res == PLANE_HIT);

			if(startDist < radius)
			{
				//start is the one outside the radius
				glm_vec3_copy(hit, end);
			}
			else
			{
				//end is the one outside the radius
				glm_vec3_copy(hit, start);
			}
			return	PLANE_HIT;
		}
		else
		{
			//fully clipped away
			//zero out segment
			glm_vec3_zero(start);
			glm_vec3_zero(end);

			return	PLANE_BACK;
		}
	}
}


int	CV_LineIntersectVolume(const vec4 *pPlanes, int numPlanes, const vec3 start, const vec3 end,
							vec3 intersection, vec3 hitNorm)
{
	vec3	backStart, backEnd;

	//check if the start point is inside
	bool	bStartInside	=true;

	//see if the entire segment is inside
	bool	bAllInside		=true;

	glm_vec3_copy(start, backStart);
	glm_vec3_copy(end, backEnd);

	for(int i=0;i < numPlanes;i++)
	{
		//check the start point
		float	startDist	=glm_vec3_dot(pPlanes[i], start) - pPlanes[i][3];
		if(startDist > 0.0f)
		{
			bStartInside	=false;
		}

		int	res	=CV_ClipLineSegmentToPlane(pPlanes[i], false, backStart, backEnd);
		if(res == PLANE_BACK)
		{
			continue;
		}
		else if(res == PLANE_FRONT)
		{
			return	VOL_MISS;
		}
		bAllInside	=false;
	}

	if(bStartInside)
	{
		//I'm guessing if starting inside, user would
		//want the point where the ray leaves the volume?
		glm_vec3_copy(backEnd, intersection);
	}
	else
	{
		glm_vec3_copy(backStart, intersection);
	}

	if(bAllInside)
	{
		return	VOL_INSIDE;
	}

	//see which plane was hit
	int		hitPlane	=-1;
	float	nearest		=FLT_MAX;
	for(int i=0;i < numPlanes;i++)
	{
		float	hitDist	=glm_vec3_dot(pPlanes[i], intersection) - pPlanes[i][3];

		hitDist	=fabs(hitDist);

		if(hitDist < nearest)
		{
			hitPlane	=i;
			nearest		=hitDist;
		}
	}

	assert(hitPlane != -1);

	glm_vec3_copy(pPlanes[hitPlane], hitNorm);

	if(bStartInside)
	{
		return	VOL_HIT_INSIDE;
	}
	return	VOL_HIT;
}


int	CV_CapsuleIntersectVolume(const vec4 *pPlanes, int numPlanes, const vec3 start, const vec3 end,
								float radius, vec3 intersection, vec3 hitNorm)
{
	vec3	backStart, backEnd;

	//check if the start point is inside
	bool	bStartInside	=true;

	//see if the entire segment is inside
	bool	bAllInside		=true;

	glm_vec3_copy(start, backStart);
	glm_vec3_copy(end, backEnd);

	for(int i=0;i < numPlanes;i++)
	{
		//check the start point
		float	startDist	=glm_vec3_dot(pPlanes[i], start) - pPlanes[i][3];
		if(startDist > radius)
		{
			bStartInside	=false;
		}

		int	res	=CV_ClipCapsuleToPlane(pPlanes[i], false, backStart, backEnd, radius);
		if(res == PLANE_BACK)
		{
			continue;
		}
		else if(res == PLANE_FRONT)
		{
			return	VOL_MISS;
		}
		bAllInside	=false;
	}

	if(bStartInside)
	{
		//I'm guessing if starting inside, user would
		//want the point where the ray leaves the volume?
		glm_vec3_copy(backEnd, intersection);
	}
	else
	{
		glm_vec3_copy(backStart, intersection);
	}

	if(bAllInside)
	{
		return	VOL_INSIDE;
	}

	//see which plane was hit
	int		hitPlane	=-1;
	float	nearest		=FLT_MAX;
	for(int i=0;i < numPlanes;i++)
	{
		float	hitDist	=glm_vec3_dot(pPlanes[i], intersection) - pPlanes[i][3];

		hitDist	=fabs(hitDist);

		if(hitDist < nearest)
		{
			hitPlane	=i;
			nearest		=hitDist;
		}
	}

	assert(hitPlane != -1);

	glm_vec3_copy(pPlanes[hitPlane], hitNorm);

	if(bStartInside)
	{
		return	VOL_HIT_INSIDE;
	}
	return	VOL_HIT;
}


bool	PointInVolume(const vec4 *pPlanes, int numPlanes, const vec3 point)
{
	for(int i=0;i < numPlanes;i++)
	{
		float	dist	=glm_vec3_dot(pPlanes[i], point) - pPlanes[i][3];
		if(dist > 0.0f)
		{
			return	false;
		}
	}
	return	true;
}