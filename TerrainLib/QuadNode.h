#pragma once
#include	<cglm/call.h>


typedef struct	QuadNode_t		QuadNode;
typedef struct	TerrainVert_t	TerrainVert;


QuadNode	*QN_Build(TerrainVert *pVerts, int count, const vec3 mins, const vec3 maxs);
void		QN_CountLeafBounds(const QuadNode *pQN, int *pNumBounds);
void		QN_GatherLeafBounds(const QuadNode *pQN, vec3 *pMins, vec3 *pMaxs, int *pIndex);
void		QN_FixBoxHeights(QuadNode *pQN);