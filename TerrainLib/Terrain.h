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
