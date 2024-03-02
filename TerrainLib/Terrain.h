#pragma once
#include	<cglm/call.h>

typedef struct	Terrain_t			Terrain;
typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	StuffKeeper_t		StuffKeeper;

//the vertex structure used by the buffer,
//but also used in the quadtree stuff
typedef struct	TerrainVert_t
{
	vec3		mPosition;
	uint16_t	mNormal[4];		//16 bit float4

	//these are percentages of each texture in the 8 way atlas
	uint16_t	mTexFactor0[4];	//16 bit float4
	uint16_t	mTexFactor1[4];	//16 bit float4

}	TerrainVert;

Terrain	*Terrain_Create(GraphicsDevice *pGD,
	const char *pName, const char *pPath, int numSmoothPasses, float heightScalar);

void	Terrain_Draw(Terrain *pTer, GraphicsDevice *pGD, const StuffKeeper *pSK);
void	Terrain_GetQuadTreeLeafBoxes(Terrain *pTer, vec3 **ppMins, vec3 **ppMaxs, int *pNumBounds);
void	Terrain_GetBounds(const Terrain *pTer, vec3 mins, vec3 maxs);
bool	Terrain_LineIntersect(const Terrain *pTer, const vec3 start, const vec3 end,
								vec3 intersection, vec4 planeHit);