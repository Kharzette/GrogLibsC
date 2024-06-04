#pragma once
#include	<cglm/call.h>


typedef struct	QuadNode_t		QuadNode;
typedef struct	TerrainVert_t	TerrainVert;
typedef struct	Vec4List_t		Vec4List;


QuadNode	*QN_Build(float *pHeights, int w, int h, const vec3 mins, const vec3 maxs);
void		QN_CountLeafBounds(const QuadNode *pQN, int *pNumBounds);
void		QN_GatherLeafBounds(const QuadNode *pQN, vec3 *pMins, vec3 *pMaxs, int *pIndex);

//collision
//int	QN_LineIntersect(const QuadNode *pQN, const vec3 rayStart, const vec3 end,
//					const vec3 invDir, const float rayLen,
//					vec3 intersection, vec4 planeHit);
int	QN_SweptSphereIntersect(const QuadNode *pQN, const vec3 rayStart, const vec3 end,
							const vec3 invDir, float radius, float rayLen,
							vec3 intersection, vec4 planeHit);
int	QN_SphereIntersect(const QuadNode *pQN, const vec3 pos, float radius, vec4 planeHit);
//int	QN_SweptBoundIntersect(const QuadNode *pQN, const vec3 rayStart, const vec3 end,
//							const vec3 invDir, float rayLen,
//							const vec3 min, const vec3 max,
//							vec3 intersection, vec4 planeHit);

void	QN_SweptSphereIntersectPL(const QuadNode *pQN, const vec3 rayStart, const vec3 end,
								const vec3 invDir, float radius, float rayLen,
								Vec4List **pPlanesHit);
