#pragma once
#include	<cglm/call.h>


#define	VOL_MISS		0
#define	VOL_INSIDE		1	//whole line segment inside
#define	VOL_HIT			2	//start outside hit
#define	VOL_HIT_INSIDE	3	//start inside hit


typedef struct	Winding_t
{
	vec3	*mpVerts;
	int		mNumVerts;

	struct Winding_t	*next;
}	Winding;


vec4	*CV_AllocConvexVolumeFromBound(const vec3 min, const vec3 max);
void	CV_MakeConvexVolumeFromBound(const vec3 min, const vec3 max, vec4 *pVol);
vec4	*CV_MakeFromTri(const vec3 tri[3], float bottomY);
int		CV_GenerateWindings(const vec4 *pPlanes, int numPlanes, Winding **pWL);

bool	CV_PointInVolume(const vec4 *pPlanes, int numPlanes, const vec3 point);
int		CV_LineIntersectPlane(const vec4 plane, const vec3 start, const vec3 end, vec3 intersection);
int		CV_CapsuleIntersectPlane(const vec4 plane, const vec3 start, const vec3 end, float radius, vec3 intersection);
int		CV_ClipLineSegmentToPlane(const vec4 plane, bool bFront, vec3 start, vec3 end);
int		CV_LineIntersectVolume(const vec4 *pPlanes, int numPlanes, const vec3 start, const vec3 end,
							vec3 intersection, vec3 hitNorm);
int		CV_CapsuleIntersectVolume(const vec4 *pPlanes, int numPlanes, const vec3 start, const vec3 end,
									float radius, vec3 intersection, vec3 hitNorm);
