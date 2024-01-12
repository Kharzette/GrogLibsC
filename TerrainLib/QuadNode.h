#pragma once
#include	<cglm/call.h>


typedef struct	QuadNode_t		QuadNode;
typedef struct	TerrainVert_t	TerrainVert;


QuadNode	*QN_Build(const TerrainVert *pVerts, int w, int h, const vec3 mins, const vec3 maxs);