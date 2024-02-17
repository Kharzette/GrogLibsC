#pragma once
#include	<cglm/call.h>


#define	PLANE_BACK	0
#define	PLANE_FRONT	1
#define	PLANE_HIT	2

#define	VOL_MISS		0
#define	VOL_INSIDE		1	//whole line segment inside
#define	VOL_HIT			2	//start outside hit
#define	VOL_HIT_INSIDE	3	//start inside hit


vec4	*AllocConvexVolumeFromBound(const vec3 min, const vec3 max);
void	MakeConvexVolumeFromBound(const vec3 min, const vec3 max, vec4 *pVol);

bool	PointInVolume(const vec4 *pPlanes, int numPlanes, const vec3 point);
int		LineIntersectPlane(const vec4 plane, const vec3 start, const vec3 end, vec3 intersection);
int		ClipLineSegmentToPlane(const vec4 plane, bool bFront, vec3 start, vec3 end);
int		LineIntersectVolume(const vec4 *pPlanes, int numPlanes, const vec3 start, const vec3 end,
							vec3 intersection, vec3 hitNorm);
