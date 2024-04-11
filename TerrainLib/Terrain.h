#pragma once
#include	<cglm/call.h>

#define	TER_MISS		0
#define	TER_HIT			1	//start outside hit
#define	TER_INSIDE		2	//whole line segment inside
#define	TER_HIT_INSIDE	4	//start inside hit

typedef struct	Terrain_t			Terrain;
typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	Material_t			Material;
typedef struct	CBKeeper_t			CBKeeper;

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

void	Terrain_DrawMat(Terrain *pTer, GraphicsDevice *pGD, CBKeeper *pCBK, const Material *pMat);
void	Terrain_Draw(Terrain *pTer, GraphicsDevice *pGD, const StuffKeeper *pSK);
void	Terrain_SetSRV(Terrain *pTer, const char *szSRV, const StuffKeeper *pSK);
void	Terrain_GetQuadTreeLeafBoxes(Terrain *pTer, vec3 **ppMins, vec3 **ppMaxs, int *pNumBounds);
void	Terrain_GetBounds(const Terrain *pTer, vec3 mins, vec3 maxs);

//collision
//int	Terrain_LineIntersect(const Terrain *pTer, const vec3 start, const vec3 end,
//							vec3 intersection, vec4 planeHit);
int	Terrain_SweptSphereIntersect(const Terrain *pTer, const vec3 start, const vec3 end,
									float radius, vec3 intersection, vec4 planeHit);
int	Terrain_SphereIntersect(const Terrain *pTer, const vec3 pos, float radius, vec4 planeHit);
//int	Terrain_SweptBoundIntersect(const Terrain *pTer, const vec3 start, const vec3 end,
//								const vec3 min, const vec3 max,
//								vec3 intersection, vec4 planeHit);

//movement
//return footing
int	Terrain_MoveSphere(const Terrain *pTer, const vec3 start, const vec3 end,
					   float radius, vec3 finalPos);