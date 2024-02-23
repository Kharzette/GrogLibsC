#pragma once
#include	<d3d11_1.h>
#include	<cglm/call.h>
#include	"GraphicsDevice.h"


typedef struct	PrimObject_t
{
	ID3D11Buffer	*mpVB, *mpIB;	//vert and index buffers
	int				mVertCount;		//num verts
	int				mIndexCount;	//num indexes
}	PrimObject;

PrimObject	*PF_CreateCubeFromCorners(const vec3 *pCorners, GraphicsDevice *pGD);
PrimObject	*PF_CreateCube(float size, GraphicsDevice *pGD);
PrimObject	*PF_CreateCubesFromBoundArray(const vec3 *pMins, const vec3 *pMaxs, int numBounds, GraphicsDevice *pGD);
PrimObject	*PF_CreatePrism(float size, float sizeY, GraphicsDevice *pGD);
PrimObject	*PF_CreateHalfPrism(float size, float sizeY, GraphicsDevice *pGD);
PrimObject	*PF_CreateSphere(vec3 center, float radius, GraphicsDevice *pGD);
PrimObject	*PF_CreateCapsule(float radius, float len, GraphicsDevice *pGD);