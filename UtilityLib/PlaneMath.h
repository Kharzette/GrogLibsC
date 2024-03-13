#pragma once
#include	<cglm/call.h>

typedef struct	Winding_t	Winding;

//creation stuff
bool	PM_PlaneFromVerts(const vec3 *pVerts, int numVerts, vec4 plane);
bool	PM_PlaneFromTri(const vec3 v0, const vec3 v1, const vec3 v2, vec4 plane);
void	PM_VertsFromPlane(const vec4 plane, vec3 verts[4]);

//collides
int	PM_LineIntersectPlane(const vec4 plane, const vec3 start, const vec3 end, vec3 intersection);

//chops
int		PM_ClipLineToPlane(const vec4 plane, bool bFront, vec3 start, vec3 end);
int		PM_ClipCapsuleToPlane(const vec4 plane, bool bFront, vec3 start, vec3 end, float radius);
Winding	*PM_ClipWindingBehindPlane(const vec4 plane, const Winding *pW);