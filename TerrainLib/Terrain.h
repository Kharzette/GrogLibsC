#pragma once
#include	<cglm/call.h>
#include	"joltc.h"		//for bodyinterface

#define	TER_MISS		0
#define	TER_HIT			1	//start outside hit
#define	TER_INSIDE		2	//whole line segment inside
#define	TER_HIT_INSIDE	4	//start inside hit

typedef struct	Terrain_t			Terrain;
typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	Material_t			Material;
typedef struct	CBKeeper_t			CBKeeper;
typedef 		uint16_t			JPH_ObjectLayer;

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

Terrain	*Terrain_Create(GraphicsDevice *pGD, JPH_BodyInterface *pBI,
	const char *pName, const char *pPath, JPH_ObjectLayer objLayer,
	int numSmoothPasses, float heightScalar);
void	Terrain_Destroy(Terrain **ppTer, JPH_BodyInterface *pBI);

void	Terrain_DrawMat(Terrain *pTer, GraphicsDevice *pGD, CBKeeper *pCBK, const Material *pMat);
void	Terrain_Draw(Terrain *pTer, GraphicsDevice *pGD, const StuffKeeper *pSK);
void	Terrain_SetSRVAndLayout(Terrain *pTer, const char *szSRV, const StuffKeeper *pSK);