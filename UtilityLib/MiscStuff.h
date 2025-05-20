#pragma once
#include	<stdint.h>
#include	<cglm/call.h>


#define	PLANE_BACK	0
#define	PLANE_FRONT	1
#define	PLANE_HIT	2

#define	MISS				0	//no intersection
#define	INTERSECT			1	//intersection with bound from outside
#define	INSIDE				2	//segment fully inside
#define	INSIDE_INTERSECT	4	//start point inside but segment intersects

//simd stuff
extern void	Misc_Convert4ToF16(float f0, float f1, float f2, float f3, uint16_t *pDest);
extern void	Misc_Convert2ToF16(float f0, float f1, uint16_t *pDest);
extern void	Misc_ConvertVec3ToF16(const vec3 vec, uint16_t *pDest);
extern void	Misc_ConvertVec4ToF16(const vec4 vec, uint16_t *pDest);
extern void	Misc_ConvertVec2ToF16(const vec2 vec, uint16_t *pDest);
extern void	Misc_InterleaveVec4ToF16(const vec4 vec0, const vec4 vec1, uint32_t *pDest);
extern void	Misc_InterleaveVec34ToF16(const vec3 vec0, const vec4 vec1, uint32_t *pDest);
extern void	Misc_InterleaveVec3IdxToF16(const vec3 vec0, const vec3 vec1, uint16_t idx, uint32_t *pDest);
extern void	Misc_ConvertFlippedUVVec2ToF16(const vec2 vec, uint16_t *pDest);
extern void	Misc_SSE_ReciprocalVec3(const vec3 vector, vec3 recip);
extern int	Misc_SSE_RoundFToI(float val);
extern uint32_t	Misc_SSE_Vec3ToRGBA(const vec3 v);
extern uint32_t	Misc_SSE_Vec4ToRGBA(const vec4 v);
extern void	Misc_RGBAToVec3(uint32_t col, vec3 ret);
extern void	Misc_RGBAToVec4(uint32_t col, vec4 ret);
extern void	Misc_RGBA16ToVec4(uint64_t col, vec4 ret);
extern void	Misc_RGBA16ToVec3(uint64_t col, vec3 ret);

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
extern void		Misc_RandomColour(vec4 col);
extern void		Misc_BuildBasisVecsFromDirection(const vec3 direction, vec3 baseX, vec3 baseY, vec3 baseZ);
extern float	Misc_ComputeAngleSum(const vec3 pointOnPlane, const vec3 *pVerts, int numVerts);
extern bool		Misc_CompareVec4s(const vec4 a, const vec4 b);
extern bool		Misc_CompareVec3s(const vec3 a, const vec3 b);
extern void		Misc_LinearToSRGB(const vec4 vLin, vec4 vSRGB);
extern void		Misc_SRGBToLinear(const vec4 vSRGB, vec4 vLin);
extern void		Misc_SRGBToLinear255(const vec4 vSRGB, vec4 vLin);

//collision stuff
extern bool	Misc_CheckTwoAABBOverlap(const vec3 aMin, const vec3 aMax, const vec3 bMin, const vec3 bMax);
extern bool	Misc_IsPointInBounds(const vec3 min, const vec3 max, const vec3 pnt);
extern bool	Misc_SphereIntersectBounds(const vec3 pos, float radius, const vec3 bounds[2]);
extern bool	Misc_RayIntersectBounds(const vec3 rayStart, const vec3 invDir, const float rayLen, const vec3 bounds[2]);