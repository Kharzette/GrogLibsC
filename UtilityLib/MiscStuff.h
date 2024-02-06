#pragma once
#include	<stdint.h>
#include	<cglm/call.h>


extern void	Misc_Convert4ToF16(float f0, float f1, float f2, float f3, uint16_t *pDest);
extern void	Misc_Convert2ToF16(float f0, float f1, uint16_t *pDest);

extern void	ClearBounds(vec3 min, vec3 max);
extern void	AddPointToBoundingBox(vec3 min, vec3 max, const vec3 pnt);
extern bool	IsPointInBounds(const vec3 min, const vec3 max, const vec3 pnt);