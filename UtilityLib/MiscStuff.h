#pragma once
#include	<stdint.h>
#include	<cglm/call.h>


#define	MISS				0	//no intersection
#define	INTERSECT			1	//intersection with bound from outside
#define	INSIDE				2	//segment fully inside
#define	INSIDE_INTERSECT	3	//start point inside but segment intersects

extern void	Misc_Convert4ToF16(float f0, float f1, float f2, float f3, uint16_t *pDest);
extern void	Misc_Convert2ToF16(float f0, float f1, uint16_t *pDest);
extern void	Misc_ConvertVec3ToF16(const vec3 vec, uint16_t *pDest);
extern void	Misc_ConvertVec2ToF16(const vec2 vec, uint16_t *pDest);
extern void	Misc_SSE_ReciprocalVec3(const vec3 vector, vec3 recip);

extern void	Misc_ClearBounds(vec3 min, vec3 max);
extern void	Misc_AddPointToBounds(vec3 min, vec3 max, const vec3 pnt);
extern void	Misc_MakeBound(float width, float height, float depth, vec3 min, vec3 max);
extern void	Misc_MakeBaseOrgBound(float width, float height, float depth, vec3 min, vec3 max);

extern bool	Misc_IsPointInBounds(const vec3 min, const vec3 max, const vec3 pnt);
extern int	Misc_LineIntersectBounds(const vec3 min, const vec3 max, const vec3 start, const vec3 end,
								vec3 intersection, vec3 hitNorm);
extern bool	Misc_RayIntersectBounds(const vec3 rayStart, const vec3 invDir, const float rayLen, const vec3 bounds[2]);