#pragma once
#include	<d3d11_1.h>
#include	<cglm/call.h>
#include	"../UtilityLib/GraphicsDevice.h"
#include	"../MaterialLib/CBKeeper.h"
#include	"../UtilityLib/PrimFactory.h"


typedef struct	AxisXYZ_t	AxisXYZ;
typedef struct	LightRay_t	LightRay;


//creates
AxisXYZ		*CP_CreateAxis(float length, float width, GraphicsDevice *pGD);
LightRay	*CP_CreateLightRay(float length, float width, GraphicsDevice *pGD);

//draws
void	CP_DrawLightRay(LightRay *pRay, const vec3 lightDir, const vec4 rayColour,
						CBKeeper *pCBK, GraphicsDevice *pGD);
void	CP_DrawAxis(AxisXYZ *pAxis, const vec3 lightDir,
					const vec4 xCol, const vec4 yCol, const vec4 zCol,
					CBKeeper *pCBK, GraphicsDevice *pGD);