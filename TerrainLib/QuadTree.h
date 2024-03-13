#pragma once
#include	<cglm/call.h>


typedef struct	QuadTree_t		QuadTree;
typedef struct	TerrainVert_t	TerrainVert;


QuadTree	*QT_Create(TerrainVert *pVerts, int w, int h);
void		QT_GetBounds(const QuadTree *pQT, vec3 mins, vec3 maxs);
void		QT_GatherLeafBounds(const QuadTree *pQT, vec3 **ppMins, vec3 **ppMaxs, int *pNumBounds);
bool		QT_LineIntersect(const QuadTree *pQT, const vec3 start, const vec3 end,
							vec3 intersection, vec4 planeHit);
bool		QT_CapsuleIntersect(const QuadTree *pQT, const vec3 start, const vec3 end,
								float radius, vec3 intersection, vec4 planeHit);