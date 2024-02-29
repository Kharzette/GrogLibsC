#pragma once
#include	<stdint.h>
#include	<cglm/call.h>


#define	PLANE_BACK	0
#define	PLANE_FRONT	1
#define	PLANE_HIT	2

#define	MISS				0	//no intersection
#define	INTERSECT			1	//intersection with bound from outside
#define	INSIDE				2	//segment fully inside
#define	INSIDE_INTERSECT	3	//start point inside but segment intersects

extern void	Misc_Convert4ToF16(float f0, float f1, float f2, float f3, uint16_t *pDest);
extern void	Misc_Convert2ToF16(float f0, float f1, uint16_t *pDest);
extern void	Misc_ConvertVec3ToF16(const vec3 vec, uint16_t *pDest);
extern void	Misc_ConvertVec2ToF16(const vec2 vec, uint16_t *pDest);
extern void	Misc_SSE_ReciprocalVec3(const vec3 vector, vec3 recip);
extern int	Misc_SSE_RoundFToI(float val);

extern void	Misc_ClearBounds(vec3 min, vec3 max);
extern void	Misc_AddPointToBounds(vec3 min, vec3 max, const vec3 pnt);
extern void	Misc_MakeBound(float width, float height, float depth, vec3 min, vec3 max);
extern void	Misc_MakeBaseZOrgBound(float width, float height, float depth, vec3 min, vec3 max);
extern void	Misc_RandomPointInBound(const vec3 mins, const vec3 maxs, vec3 result);

extern int	Misc_LineIntersectPlane(const vec4 plane, const vec3 start, const vec3 end, vec3 intersection);
extern bool	Misc_IsPointInBounds(const vec3 min, const vec3 max, const vec3 pnt);
extern int	Misc_LineIntersectBounds(const vec3 min, const vec3 max, const vec3 start, const vec3 end,
								vec3 intersection, vec3 hitNorm);
extern bool	Misc_RayIntersectBounds(const vec3 rayStart, const vec3 invDir, const float rayLen, const vec3 bounds[2]);

extern bool	Misc_PlaneFromVerts(const vec3 *pVerts, int numVerts, vec4 plane);
extern bool	Misc_PlaneFromTri(const vec3 v0, const vec3 v1, const vec3 v2, vec4 plane);

extern float	Misc_ComputeAngleSum(const vec3 pointOnPlane, const vec3 *pVerts, int numVerts);