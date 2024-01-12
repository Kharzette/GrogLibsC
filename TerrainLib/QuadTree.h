#pragma once
#include	<cglm/call.h>


typedef struct	QuadTree_t		QuadTree;
typedef struct	TerrainVert_t	TerrainVert;


QuadTree	*QT_Create(const TerrainVert *pVerts, int w, int h);