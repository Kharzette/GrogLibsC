#pragma once
#include	<cglm/call.h>


typedef struct	QuadTree_t		QuadTree;
typedef struct	TerrainVert_t	TerrainVert;


QuadTree	*QT_Create(TerrainVert *pVerts, int w, int h);
void		QT_GetBounds(const QuadTree *pQT, vec3 mins, vec3 maxs);
void		QT_GatherLeafBounds(const QuadTree *pQT, vec3 **ppMins, vec3 **ppMaxs, int *pNumBounds);

//collides
//int	QT_LineIntersect(const QuadTree *pQT, const vec3 start, const vec3 end,
//						vec3 intersection, vec4 planeHit);
int	QT_SweptSphereIntersect(const QuadTree *pQT, const vec3 start, const vec3 end,
							float radius, vec3 intersection, vec4 planeHit);
int	QT_SphereIntersect(const QuadTree *pQT, const vec3 pos, float radius, vec4 planeHit);							
//int	QT_SweptBoundIntersect(const QuadTree *pQT, const vec3 start, const vec3 end,
//							const vec3 min, const vec3 max,
//							vec3 intersection, vec4 planeHit);

void	QT_SweptSphereIntersectPL(const QuadTree *pQT, const vec3 start, const vec3 end,
								float radius, Vec4List **ppPlanesHit);