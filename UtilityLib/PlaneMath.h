#pragma once
#include	<cglm/call.h>


//creation stuff
bool	PM_FromVerts(const vec3 *pVerts, int numVerts, vec4 plane);
bool	PM_FromTri(const vec3 v0, const vec3 v1, const vec3 v2, vec4 plane);
void	PM_ToVerts(const vec4 plane, vec3 verts[4]);

//collides
int	PM_LineIntersectPlane(const vec4 plane, const vec3 start, const vec3 end, vec3 intersection);

//chops
int		PM_ClipLine(const vec4 plane, bool bFront, vec3 start, vec3 end);

//mathy
float	PM_Distance(const vec4 plane, const vec3 point);
bool	PM_IsGround(const vec4 plane);
bool	PM_IsGroundNormal(const vec3 norm);
bool	PM_IsGroundNormalAng(const vec3 norm, float ang);
