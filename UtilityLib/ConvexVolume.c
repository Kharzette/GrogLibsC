#include	<stdint.h>
#include	<string.h>
#include	<assert.h>
#include	<utlist.h>
#include	<cglm/call.h>
#include	"ConvexVolume.h"
#include	"MiscStuff.h"
#include	"PlaneMath.h"


static const vec4	BoxPlanes[6]	=
{
	{	1, 0, 0, 0	},
	{	0, 1, 0, 0	},
	{	0, 0, 1, 0	},
	{	-1, 0, 0, 0	},
	{	0, -1, 0, 0	},
	{	0, 0, -1, 0	}
};


ConvexVolume	*CV_AllocConvexVolumeFromBound(const vec3 min, const vec3 max)
{
	ConvexVolume	*pRet	=malloc(sizeof(ConvexVolume));

	pRet->mpPlanes		=malloc(sizeof(vec4) * 6);
	pRet->mNumPlanes	=6;

	CV_MakeConvexVolumeFromBound(min, max, pRet);

	return	pRet;
}


//pass in a 6 plane CV
void	CV_MakeConvexVolumeFromBound(const vec3 min, const vec3 max, ConvexVolume *pVol)
{
	assert(pVol->mNumPlanes == 6);

	//copy base box planes
	memcpy(pVol->mpPlanes, BoxPlanes, sizeof(vec4) * 6);

	//find plane distances
	pVol->mpPlanes[0][3]	=max[0];	
	pVol->mpPlanes[1][3]	=max[1];
	pVol->mpPlanes[2][3]	=max[2];

	pVol->mpPlanes[3][3]	=-min[0];	
	pVol->mpPlanes[4][3]	=-min[1];
	pVol->mpPlanes[5][3]	=-min[2];
}


//returns num windings
int	CV_GenerateWindings(const ConvexVolume *pCV, Winding **ppWL)
{
	*ppWL	=NULL;

	//generate huge faces
	int	windCount	=0;
	for(int i=0;i < pCV->mNumPlanes;i++)
	{
		vec3	wind[4];

		PM_VertsFromPlane(pCV->mpPlanes[i], wind);

		Winding	*pA	=malloc(sizeof(Winding));

		pA->mNumVerts	=4;
		pA->mpVerts		=malloc(sizeof(vec3) * 4);
		pA->next		=NULL;

		for(int j=0;j < 4;j++)
		{
			glm_vec3_copy(wind[j], pA->mpVerts[j]);
		}

		//clip behind all planes
		for(int j=0;j < pCV->mNumPlanes;j++)
		{
			if(j == i)
			{
				continue;
			}

			Winding	*pNewW	=PM_ClipWindingBehindPlane(pCV->mpPlanes[j], pA);

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
//TODO: bevel sharp angles
ConvexVolume *CV_MakeFromTri(const vec3 tri[3], float bottomY)
{
	//top Y plane
	vec4	triPlane;
	PM_PlaneFromTri(tri[0], tri[1], tri[2], triPlane);

	//edge plane 0 -> 1
	vec3	edgeVec;
	glm_vec3_sub(tri[0], tri[1], edgeVec);

	vec3	upVec	={	0.0f, 1.0f, 0.0f	};

	vec3	norm;
	glm_vec3_cross(edgeVec, upVec, norm);
	glm_vec3_normalize(norm);

	vec4	plane01	={	norm[0], norm[1], norm[2], 0.0f	};

	plane01[3]	=glm_vec3_dot(norm, tri[0]);

	///edge plane 0 -> 2
	glm_vec3_sub(tri[0], tri[2], edgeVec);

	glm_vec3_cross(upVec, edgeVec, norm);
	glm_vec3_normalize(norm);

	vec4	plane02	={	norm[0], norm[1], norm[2], 0.0f	};

	plane02[3]	=glm_vec3_dot(norm, tri[0]);

	///edge plane 1 -> 2
	glm_vec3_sub(tri[1], tri[2], edgeVec);

	glm_vec3_cross(edgeVec, upVec, norm);
	glm_vec3_normalize(norm);

	vec4	plane12	={	norm[0], norm[1], norm[2], 0.0f	};

	plane12[3]	=glm_vec3_dot(norm, tri[1]);

	vec4	bottomPlane	={	0.0f, -1.0f, 0.0f, -bottomY	};

	ConvexVolume	*pRet	=malloc(sizeof(ConvexVolume));

	pRet->mpPlanes		=malloc(sizeof(vec4) * 5);
	pRet->mNumPlanes	=5;

	//copy generated planes
	//The triangle's plane goes at 0 index,
	//this is used to determine a hit on a visible surface later.
	glm_vec4_copy(triPlane, pRet->mpPlanes[0]);
	glm_vec4_copy(bottomPlane, pRet->mpPlanes[1]);
	glm_vec4_copy(plane01, pRet->mpPlanes[2]);
	glm_vec4_copy(plane02, pRet->mpPlanes[3]);
	glm_vec4_copy(plane12, pRet->mpPlanes[4]);

	return	pRet;
}


int	CV_LineIntersectVolume(const ConvexVolume *pVol, const vec3 start, const vec3 end,
							vec3 intersection, vec4 hitPlane)
{
	vec3	backStart, backEnd;

	//check if the start point is inside
	bool	bStartInside	=true;

	//see if the entire segment is inside
	bool	bAllInside		=true;

	glm_vec3_copy(start, backStart);
	glm_vec3_copy(end, backEnd);

	for(int i=0;i < pVol->mNumPlanes;i++)
	{
		//check the start point
		float	startDist	=glm_vec3_dot(pVol->mpPlanes[i], start) - pVol->mpPlanes[i][3];
		if(startDist > 0.0f)
		{
			bStartInside	=false;
		}

		int	res	=PM_ClipLineToPlane(pVol->mpPlanes[i], false, backStart, backEnd);
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
	int		hitPIdx	=-1;
	float	nearest		=FLT_MAX;
	for(int i=0;i < pVol->mNumPlanes;i++)
	{
		float	hitDist	=glm_vec3_dot(pVol->mpPlanes[i], intersection) - pVol->mpPlanes[i][3];

		hitDist	=fabs(hitDist);

		if(hitDist < nearest)
		{
			hitPIdx	=i;
			nearest	=hitDist;
		}
	}

	assert(hitPIdx != -1);

	glm_vec3_copy(pVol->mpPlanes[hitPIdx], hitPlane);

	if(bStartInside)
	{
		if(hitPIdx == 0)
		{
			return	VOL_HIT_INSIDE | VOL_HIT_VISIBLE;
		}
		else
		{
			return	VOL_HIT_INSIDE;
		}
	}
	if(hitPIdx == 0)
	{
		return	VOL_HIT | VOL_HIT_VISIBLE;
	}
	else
	{
		return	VOL_HIT;
	}
}


int	CV_SweptSphereIntersect(const ConvexVolume *pVol, const vec3 start, const vec3 end,
							float radius, vec3 intersection, vec4 hitPlane)
{
	vec3	backStart, backEnd;

	//check if the start point is inside
	bool	bStartInside	=true;

	//see if the entire segment is inside
	bool	bAllInside		=true;

	glm_vec3_copy(start, backStart);
	glm_vec3_copy(end, backEnd);

	for(int i=0;i < pVol->mNumPlanes;i++)
	{
		//check the start point
		float	startDist	=glm_vec3_dot(pVol->mpPlanes[i], start) - pVol->mpPlanes[i][3];
		if(startDist > radius)
		{
			bStartInside	=false;
		}

		int	res	=PM_ClipCapsuleToPlane(pVol->mpPlanes[i], false, backStart, backEnd, radius);
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
	int		hitPIdx	=-1;
	float	nearest	=FLT_MAX;
	for(int i=0;i < pVol->mNumPlanes;i++)
	{
		float	hitDist	=glm_vec3_dot(pVol->mpPlanes[i], intersection) - pVol->mpPlanes[i][3];

		hitDist	=fabs(hitDist);

		if(hitDist < nearest)
		{
			hitPIdx	=i;
			nearest	=hitDist;
		}
	}

	assert(hitPIdx != -1);

	glm_vec4_copy(pVol->mpPlanes[hitPIdx], hitPlane);

	if(bStartInside)
	{
		return	VOL_HIT_INSIDE;
	}
	return	VOL_HIT;
}


bool	CV_PointInVolume(const ConvexVolume *pVol, const vec3 point)
{
	for(int i=0;i < pVol->mNumPlanes;i++)
	{
		float	dist	=glm_vec3_dot(pVol->mpPlanes[i], point) - pVol->mpPlanes[i][3];
		if(dist > 0.0f)
		{
			return	false;
		}
	}
	return	true;
}