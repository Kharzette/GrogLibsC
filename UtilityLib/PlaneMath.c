#include	<stdint.h>
#include	<x86intrin.h>
#include	<string.h>
#include	<cglm/call.h>
#include	<cglm/box.h>
#include	<assert.h>
#include	"MiscStuff.h"
#include	"ConvexVolume.h"
#include	"PlaneMath.h"


#define	MIN_MAX_BOUNDS			15192.0f
#define	DIST_EPSILON			0.01f
#define	RAMP_ANGLE				0.7f	//steepness can traverse on foot
#define	STEP_HEIGHT				18.0f	//quake units
#define	STOP_DOWN_HEIGHT		10.0f	//quake units
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

static Winding *CopyWinding(const Winding *pW);


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

	memcpy(tri[0], v0, sizeof(vec3));
	memcpy(tri[1], v1, sizeof(vec3));
	memcpy(tri[2], v2, sizeof(vec3));

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

//chop start->end by plane
//Modify start and end to return the front side of plane segment.
//Note that back side clips aren't really safe to do as the radius
//can cause the volume to turn inside out and go negative.
//For quadtree terrain I shift only the visible surface plane.
int	PM_ClipSweptSphere(const vec4 plane, bool bFront, vec3 start, vec3 end, float radius)
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
	else if(startDist <= radius && endDist <= radius)
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
			int	res	=PM_LineIntersectPlane(bumped, start, end, hit);
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
			int	res	=PM_LineIntersectPlane(bumped, start, end, hit);
			if(res != PLANE_HIT)
			{
				//this can happen rarely due to fp error
				return	res;
			}

			if(startDist < radius)
			{
				//start is the one inside the radius
				glm_vec3_copy(hit, end);
			}
			else
			{
				//end is the one inside the radius
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

			return	PLANE_FRONT;
		}
	}
}

int	PM_SweptSphereToTriIntersect(const vec3 tri[3], const vec3 start, const vec3 end, float radius,
								 vec3 hit, vec4 hitPlane)
{
	//plane of the triangle
	vec4	triPlane;
	PM_FromTri(tri[0], tri[1], tri[2], triPlane);

	//expand by radius
	triPlane[3]	+=radius;

	vec3	intersection;
	int	res	=PM_LineIntersectPlane(triPlane, start, end, intersection);
	if(res == PLANE_FRONT)
	{
		//short of impact
		return	MISS;
	}
	else if(res == PLANE_BACK)
	{
		//start and end both behind triangle
		//This could early out in a volumetric dataset
//		return	INSIDE;
	}

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

	//get intersection distance to edge planes
	float	dist01	=PM_Distance(plane01, intersection);
	float	dist02	=PM_Distance(plane02, intersection);
	float	dist12	=PM_Distance(plane12, intersection);

	if(dist01 > radius || dist02 > radius || dist12 > radius)
	{
		return	MISS;
	}

	//direct plane hit
	if(dist01 <= 0.0f && dist02 <= 0.0f && dist12 <= 0.0f)
	{
		glm_vec4_copy(triPlane, hitPlane);
		glm_vec3_copy(intersection, hit);

		return	INTERSECT;
	}

	//For closed meshes I THINK this is enough...
	//For free floating triangles I think additional checks for
	//edge and vertex collisions would need to be done here.
	return	MISS;
}

Winding *PM_ClipWindingBehind(const vec4 plane, const Winding *pW)
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


float	PM_Distance(const vec4 plane, const vec3 point)
{
	return	glm_vec3_dot(plane, point) - plane[3];
}

//push slightly to the front side
void	PM_ReflectMove(const vec4 plane, const vec3 start, const vec3 end, vec3 newPos)
{
	float	startDist	=PM_Distance(plane, start);
	float	dist		=PM_Distance(plane, end);

	//is the direction vector valid to find a collision response?
	if(startDist <= 0.0f || dist >= DIST_EPSILON)
	{
		vec3	scaledNorm;
		glm_vec3_scale(plane, dist, scaledNorm);

		//place newPos directly on the plane
		glm_vec3_sub(end, scaledNorm, newPos);

		//adjust it to the front side
		glm_vec3_scale(plane, DIST_EPSILON, scaledNorm);
		glm_vec3_add(scaledNorm, newPos, newPos);
	}
	else
	{
		vec3	scaledNorm;
		glm_vec3_scale(plane, dist - DIST_EPSILON, scaledNorm);
		glm_vec3_sub(end, scaledNorm, newPos);
	}
}

//adjust a position just off the front side
void	PM_ReflectPosition(const vec4 plane, vec3 pos)
{
	float	dist	=PM_Distance(plane, pos);

	//directly on or off a bit?
	if(dist >= DIST_EPSILON)
	{
		vec3	scaledNorm;
		glm_vec3_scale(plane, dist, scaledNorm);

		//place newPos directly on the plane
		glm_vec3_sub(pos, scaledNorm, pos);

		//adjust it to the front side
		glm_vec3_scale(plane, DIST_EPSILON, scaledNorm);
		glm_vec3_add(scaledNorm, pos, pos);
	}
	else
	{
		vec3	scaledNorm;
		glm_vec3_scale(plane, dist - DIST_EPSILON, scaledNorm);
		glm_vec3_sub(pos, scaledNorm, pos);
	}
}

//adjust a movement vec along the plane
bool	PM_MoveAlong(const vec4 plane, vec3 moveVec)
{
	assert(PM_IsGround(plane));

	if(glm_vec3_eq(moveVec, 0.0f))
	{
		return	false;
	}

	//save length
	float	len	=glm_vec3_norm(moveVec);

	//normalized move vec
	vec3	moveNorm;
	glm_vec3_scale(moveVec, 1.0f / len, moveNorm);

	float	dot	=glm_vec3_dot(plane, moveNorm);

	if(dot < -(1.0f -RAMP_ANGLE) || dot > (1.0f - RAMP_ANGLE))
	{
		return	false;	//movement is too perp for alignment
	}

	vec3	sideVec;
	glm_vec3_cross(moveVec, plane, sideVec);

	vec3	newVec;
	glm_vec3_cross(plane, sideVec, newVec);

	glm_vec3_normalize(newVec);

	glm_vec3_scale(newVec, len, moveVec);

	return	true;
}

bool	PM_IsGround(const vec4 plane)
{
	float	dotY	=glm_vec3_dot(plane, UnitY);

	return	(dotY > RAMP_ANGLE);
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