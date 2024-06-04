#pragma once
#include	<cglm/call.h>

typedef struct	Winding_t	Winding;

//creation stuff
bool	PM_FromVerts(const vec3 *pVerts, int numVerts, vec4 plane);
bool	PM_FromTri(const vec3 v0, const vec3 v1, const vec3 v2, vec4 plane);
void	PM_ToVerts(const vec4 plane, vec3 verts[4]);

//collides
int	PM_LineIntersectPlane(const vec4 plane, const vec3 start, const vec3 end, vec3 intersection);
int	PM_SweptSphereToTriIntersect(const vec3 tri[3], const vec3 start, const vec3 end, float radius,
								 vec3 hit, vec4 hitPlane);
int	PM_SphereToTriIntersect(const vec3 tri[3], const vec3 pos, float radius, vec4 hitPlane);

//chops
int		PM_ClipLine(const vec4 plane, bool bFront, vec3 start, vec3 end);
int		PM_ClipSweptSphere(const vec4 plane, bool bFront, vec3 start, vec3 end, float radius);
Winding	*PM_ClipWindingBehind(const vec4 plane, const Winding *pW);

//mathy
float	PM_Distance(const vec4 plane, const vec3 point);
bool	PM_IsGround(const vec4 plane);
void	PM_ReflectMove(const vec4 plane, const vec3 start, const vec3 end, vec3 newPos);
void	PM_ReflectPosition(const vec4 plane, vec3 pos);
void	PM_ReflectSphere(const vec4 plane, float radius, vec3 pos);
void	PM_ReflectMoveSphere(const vec4 plane, const vec3 start, const vec3 end, float radius, vec3 newPos);
bool	PM_MoveAlong(const vec4 plane, vec3 moveVec);