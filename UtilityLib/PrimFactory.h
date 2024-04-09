#pragma once
#include	<d3d11_1.h>
#include	<cglm/call.h>
#include	"GraphicsDevice.h"


typedef struct	ConvexVolume_t	ConvexVolume;

typedef struct	PrimObject_t
{
	ID3D11Buffer	*mpVB, *mpIB;	//vert and index buffers
	int				mVertCount;		//num verts
	int				mIndexCount;	//num indexes
	int				mVertSize;		//size in bytes of vert format in use
}	PrimObject;

//a single triangle
PrimObject	*PF_CreateTri(const vec3 tri[3], GraphicsDevice *pGD);

//cubes
PrimObject	*PF_CreateCubeFromBounds(const vec3 min, const vec3 max, GraphicsDevice *pGD);
PrimObject	*PF_CreateCubeFromCorners(const vec3 *pCorners, bool bFlipped, GraphicsDevice *pGD);
PrimObject	*PF_CreateCube(float size, bool bFlipped, GraphicsDevice *pGD);
PrimObject	*PF_CreateCubesFromBoundArray(const vec3 *pMins, const vec3 *pMaxs, int numBounds, GraphicsDevice *pGD);
PrimObject	*PF_CreateManyCubes(const vec3 *pCubeCenters, int numCubes,
								float size, GraphicsDevice *pGD);

//prismsmsmsmsms
PrimObject	*PF_CreatePrism(float size, float sizeY, GraphicsDevice *pGD);
PrimObject	*PF_CreateHalfPrism(float size, float sizeY, GraphicsDevice *pGD);

//round stuff
PrimObject	*PF_CreateSphere(vec3 center, float radius, GraphicsDevice *pGD);
PrimObject	*PF_CreateCapsule(float radius, float len, GraphicsDevice *pGD);

//other shapes
PrimObject	*PF_CreateCV(const ConvexVolume *pVol, GraphicsDevice *pGD);
PrimObject	*PF_CreateManyRays(const vec3 *pStarts, const vec3 *pEnds, const vec4 *pColours,
								int numRays, float rayWidth, GraphicsDevice *pGD);

//free
void	PF_DestroyPO(PrimObject **ppObj);