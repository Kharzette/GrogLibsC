#include	<stdint.h>
#include	<string.h>
#include	<assert.h>
#include	<cglm/call.h>
#include	"ConvexVolume.h"


static vec4	BoxPlanes[6]	=
{
	{	1, 0, 0, 0	},
	{	0, 1, 0, 0	},
	{	0, 0, 1, 0	},
	{	-1, 0, 0, 0	},
	{	0, -1, 0, 0	},
	{	0, 0, -1, 0	}
};


//return a list of planes
vec4	*AllocConvexVolumeFromBound(const vec3 min, const vec3 max)
{
	vec4	*pRet	=malloc(sizeof(vec4) * 6);

	MakeConvexVolumeFromBound(min, max, pRet);

	return	pRet;
}


//pass in a vec4[6]
void	MakeConvexVolumeFromBound(const vec3 min, const vec3 max, vec4 *pVol)
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


//intersection of line and plane
int	LineIntersectPlane(const vec4 plane, const vec3 start, const vec3 end, vec3 intersection)
{
	float	startDist	=glm_vec3_dot(plane, start) - plane[3];
	float	endDist		=glm_vec3_dot(plane, end) - plane[3];

	if(startDist > 0.0f && endDist > 0.0f)
	{
		return	PLANE_FRONT;
	}
	else if(startDist < 0.0f && endDist < 0.0f)
	{
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

	glm_vec3_scale(segVec, ratio * segLength, intersection);

	glm_vec3_add(intersection, start, intersection);

	return	PLANE_HIT;
}


//chop start->end by plane
//modify start and end to return the front or back side of plane segment
int	ClipLineSegmentToPlane(const vec4 plane, bool bFront, vec3 start, vec3 end)
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


int	LineIntersectVolume(const vec4 *pPlanes, int numPlanes, const vec3 start, const vec3 end,
						vec3 intersection)
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

		int	res	=ClipLineSegmentToPlane(pPlanes[i], false, backStart, backEnd);
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

	glm_vec3_copy(backStart, intersection);

	if(bAllInside)
	{
		return	VOL_INSIDE;
	}
	else if(bStartInside)
	{
		//I'm guessing if starting inside, user would
		//want the point where the ray leaves the volume?
		glm_vec3_copy(backEnd, intersection);
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