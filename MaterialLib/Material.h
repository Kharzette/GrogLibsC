#pragma once
#include	<stdint.h>
#include	<cglm/call.h>

typedef struct	Material_t			Material;
typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	CBKeeper_t			CBKeeper;


Material	*MAT_Create(GraphicsDevice *pGD);
Material	*MAT_Read(FILE *f, const StuffKeeper *pSK);

void	MAT_Apply(const Material *pMat, CBKeeper *pCBK, GraphicsDevice *pGD);

bool	MAT_SetLayout(Material *pMat, const char *szLayName, const StuffKeeper *pSK);
bool	MAT_SetVShader(Material *pMat, const char *szVSName, const StuffKeeper *pSK);
bool	MAT_SetPShader(Material *pMat, const char *szPSName, const StuffKeeper *pSK);
bool	MAT_SetSRV0(Material *pMat, const char *szTex, const StuffKeeper *pSK);
bool	MAT_SetSRV1(Material *pMat, const char *szTex, const StuffKeeper *pSK);

void	MAT_SetLights(Material *pMat, const vec3 tri0, const vec3 tri1, const vec3 tri2, const vec3 lightDir);
void	MAT_SetLightDirection(Material *pMat, const vec3 lightDir);
void	MAT_SetSolidColour(Material *pMat, const vec4 col);
void	MAT_SetSpecular(Material *pMat, const vec3 spec, float specPower);
void	MAT_SetDanglyForce(Material *pMat, const vec3 force);
void	MAT_SetMaterialID(Material *pMat, int id);
void	MAT_SetWorld(Material *pMat, const mat4 world);