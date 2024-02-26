#pragma once
#include	<cglm/call.h>


typedef struct	QuadNode_t		QuadNode;
typedef struct	TerrainVert_t	TerrainVert;


QuadNode	*QN_Build(float *pHeights, int w, int h, const vec3 mins, const vec3 maxs);
void		QN_CountLeafBounds(const QuadNode *pQN, int *pNumBounds);
void		QN_GatherLeafBounds(const QuadNode *pQN, vec3 *pMins, vec3 *pMaxs, int *pIndex);
int			QN_LineIntersect(const QuadNode *pQT, const vec3 rayStart, const vec3 end,
							const vec3 invDir, const float rayLen,
							vec3 intersection, vec3 hitNorm);
int			QN_LineIntersectCV(const QuadNode *pQN, const vec3 rayStart, const vec3 end,
								vec3 intersection, vec3 hitNorm);