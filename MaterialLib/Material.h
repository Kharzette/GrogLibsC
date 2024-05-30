#pragma once
#include	<stdint.h>
#include	<cglm/call.h>
#include	"d3d11.h"

typedef struct	Material_t					Material;
typedef struct	GraphicsDevice_t			GraphicsDevice;
typedef struct	StuffKeeper_t				StuffKeeper;
typedef struct	CBKeeper_t					CBKeeper;


Material	*MAT_Create(GraphicsDevice *pGD);
Material	*MAT_Read(FILE *f, const StuffKeeper *pSK);
void		MAT_Write(const Material *pMat, FILE *f, const StuffKeeper *pSK);

void	MAT_Apply(const Material *pMat, CBKeeper *pCBK, GraphicsDevice *pGD);

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

//get functions for gui etc
const ID3D11VertexShader		*MAT_GetVShader(const Material *pMat);
const ID3D11PixelShader			*MAT_GetPShader(const Material *pMat);
const ID3D11ShaderResourceView	*MAT_GetSRV0(const Material *pMat);
const ID3D11ShaderResourceView	*MAT_GetSRV1(const Material *pMat);

void	MAT_GetTrilight(const Material *pMat, vec3 t0, vec3 t1, vec3 t2);
void	MAT_GetLightDir(const Material *pMat, vec3 lightDir);
void	MAT_GetSolidColour(const Material *pMat, vec4 sc);
void	MAT_GetSpecular(const Material *pMat, vec4 spec);