#include	<stdint.h>
#include	<string.h>
#include	<cglm/call.h>
#include	<cglm/box.h>
#include	<assert.h>
#include	"MiscStuff.h"
#include	"PlaneMath.h"


#define	MIN_MAX_BOUNDS			15192.0f
#define	RAMP_ANGLE				0.7f	//steepness can traverse on foot
#define	METERS_TO_QUAKEUNITS	37.6471f
#define	METERS_TO_VALVEUNITS	39.37001f
#define	METERS_TO_GROGUNITS		METERS_TO_VALVEUNITS
#define	METERS_TO_CENTIMETERS	100f

__attribute_maybe_unused__
static const vec3	UnitX	={	1.0f, 0.0f, 0.0f	};
__attribute_maybe_unused__
static const vec3	UnitY	={	0.0f, 1.0f, 0.0f	};
__attribute_maybe_unused__
static const vec3	UnitZ	={	0.0f, 0.0f, 1.0f	};


//fails if the triangle has no plane (colinear points etc)
bool	PM_FromVerts(const vec3 *pVerts, int numVerts, vec4 plane)
{
	int	i;

	glm_vec4_zero(plane);

	if(numVerts < 3)
	{
		return	false;
	}

	for(i=0;i < numVerts;i++)
	{
		vec3	v1, v2;

		//edge vectors
		glm_vec3_sub(pVerts[i], pVerts[(i + 1) % numVerts], v1);
		glm_vec3_sub(pVerts[(i + 2) % numVerts], pVerts[(i + 1) % numVerts], v2);

		//gen a plane normal from the cross of edge vectors
		glm_vec3_cross(v1, v2, plane);

		if(!glm_vec3_eq_eps(plane, 0.0f))
		{
			break;
		}
		//try the next three if there are three
	}

	if(i >= numVerts)
	{
		return	false;
	}

	glm_vec3_normalize(plane);

	//plane distance
	plane[3]	=glm_vec3_dot(pVerts[0], plane);

	return	true;
}

bool	PM_FromTri(const vec3 v0, const vec3 v1, const vec3 v2, vec4 plane)
{
	vec3	tri[3];

	glm_vec3_copy(v0, tri[0]);
	glm_vec3_copy(v1, tri[1]);
	glm_vec3_copy(v2, tri[2]);

	return	PM_FromVerts(tri, 3, plane);
}

//make a huge quad from a plane
void	PM_ToVerts(const vec4 plane, vec3 verts[4])
{
	vec3	dir	={	plane[0], plane[1], plane[2]	};

	vec3	vX, vY, vZ;
	Misc_BuildBasisVecsFromDirection(dir, vX, vY, vZ);

	//plane centerish
	vec3	org;
	glm_vec3_scale(vZ, plane[3], org);

	//scale up X and Y
	glm_vec3_scale(vX, MIN_MAX_BOUNDS, vX);
	glm_vec3_scale(vY, MIN_MAX_BOUNDS, vY);

	glm_vec3_sub(org, vX, verts[0]);
	glm_vec3_add(verts[0], vY, verts[0]);

	glm_vec3_add(org, vX, verts[1]);
	glm_vec3_add(verts[1], vY, verts[1]);

	glm_vec3_add(org, vX, verts[2]);
	glm_vec3_sub(verts[2], vY, verts[2]);

	glm_vec3_sub(org, vX, verts[3]);
	glm_vec3_sub(verts[3], vY, verts[3]);
}


//intersection of line and plane
int	PM_LineIntersectPlane(const vec4 plane, const vec3 start, const vec3 end, vec3 intersection)
{
	float	startDist	=glm_vec3_dot(plane, start) - plane[3];
	float	endDist		=glm_vec3_dot(plane, end) - plane[3];

	if(startDist > 0.0f && endDist > 0.0f)
	{
		return	PLANE_FRONT;
	}
	else if(startDist <= 0.0f && endDist <= 0.0f)
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
int	PM_ClipLine(const vec4 plane, bool bFront, vec3 start, vec3 end)
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

float	PM_Distance(const vec4 plane, const vec3 point)
{
	return	glm_vec3_dot(plane, point) - plane[3];
}

bool	PM_IsGround(const vec4 plane)
{
	return	PM_IsGroundNormal(plane);
}

bool	PM_IsGroundNormal(const vec3 norm)
{
	return	PM_IsGroundNormalAng(norm, RAMP_ANGLE);
}

bool	PM_IsGroundNormalAng(const vec3 norm, float ang)
{
	float	dotY	=glm_vec3_dot(norm, UnitY);

	return	(dotY > ang);
}
