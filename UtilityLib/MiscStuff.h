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

//simd stuff
extern void	Misc_Convert4ToF16(float f0, float f1, float f2, float f3, uint16_t *pDest);
extern void	Misc_Convert2ToF16(float f0, float f1, uint16_t *pDest);
extern void	Misc_ConvertVec3ToF16(const vec3 vec, uint16_t *pDest);
extern void	Misc_ConvertVec4ToF16(const vec4 vec, uint16_t *pDest);
extern void	Misc_ConvertVec2ToF16(const vec2 vec, uint16_t *pDest);
extern void	Misc_SSE_ReciprocalVec3(const vec3 vector, vec3 recip);
extern int	Misc_SSE_RoundFToI(float val);

//bound stuff
extern void		Misc_ClearBounds(vec3 min, vec3 max);
extern void		Misc_AddPointToBounds(vec3 min, vec3 max, const vec3 pnt);
extern void		Misc_ExpandBounds(vec3 min, vec3 max, float radius);
extern void		Misc_ExpandBoundsByBounds(vec3 min, vec3 max, const vec3 minX, const vec3 maxX);
extern void		Misc_MakeBound(float width, float height, float depth, vec3 min, vec3 max);
extern void		Misc_MakeBaseZOrgBound(float width, float height, float depth, vec3 min, vec3 max);
extern void		Misc_RandomPointInBound(const vec3 mins, const vec3 maxs, vec3 result);
extern float	Misc_BoundDistanceForNormal(const vec4 plane, const vec3 min, const vec3 max);

//vector stuff
extern void		Misc_RandomDirection(vec3 dir);
extern void		Misc_BuildBasisVecsFromDirection(const vec3 direction, vec3 baseX, vec3 baseY, vec3 baseZ);
extern float	Misc_ComputeAngleSum(const vec3 pointOnPlane, const vec3 *pVerts, int numVerts);

//collision stuff
extern bool	Misc_CheckTwoAABBOverlap(const vec3 aMin, const vec3 aMax, const vec3 bMin, const vec3 bMax);
extern bool	Misc_BPIntersectLineAABB(const vec3 start, const vec3 end,			//line segment
									const vec3 statMin, const vec3 statMax);	//static aabb
extern bool	Misc_BPIntersectSweptAABBLineAABB(const vec3 min, const vec3 max,			//moving aabb
											const vec3 start, const vec3 end,			//line segment
											const vec3 statMin, const vec3 statMax);	//static aabb
extern bool	Misc_IsPointInBounds(const vec3 min, const vec3 max, const vec3 pnt);
extern int	Misc_LineIntersectBounds(const vec3 min, const vec3 max, const vec3 start, const vec3 end,
								vec3 intersection, vec3 hitNorm);
extern bool	Misc_RayIntersectBounds(const vec3 rayStart, const vec3 invDir, const float rayLen, const vec3 bounds[2]);
extern int	Misc_CapsuleIntersectBounds(const vec3 min, const vec3 max, const vec3 start, const vec3 end,
										float radius, vec3 intersection, vec3 hitNorm);