#pragma once
#include	<stdint.h>
#include	"d3d11.h"
#include	"utstring.h"

//forward struct decs
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	GrogFont_t			GrogFont;
typedef struct	StringList_t		StringList;


StuffKeeper	*StuffKeeper_Create(GraphicsDevice *pGD);
void		TestSKStuff(void);

//raw png load, used externally by terrain stuff
BYTE **SK_LoadTextureBytes(const char *pPath, int *pOutRowPitch,
							uint32_t *pOutWidth, uint32_t *pOutHeight);

//stored stuff gets
ID3D11DepthStencilState		*StuffKeeper_GetDepthStencilState(const StuffKeeper *pSK, const char *pStateName);
ID3D11BlendState			*StuffKeeper_GetBlendState(const StuffKeeper *pSK, const char *pStateName);
ID3D11SamplerState			*StuffKeeper_GetSamplerState(const StuffKeeper *pSK, const char *pStateName);
ID3D11Texture2D				*StuffKeeper_GetTexture2D(const StuffKeeper *pSK, const char *pName);
ID3D11VertexShader			*StuffKeeper_GetVertexShader(const StuffKeeper *pSK, const char *pName);
ID3D11PixelShader			*StuffKeeper_GetPixelShader(const StuffKeeper *pSK, const char *pName);
ID3D11InputLayout			*StuffKeeper_GetInputLayout(const StuffKeeper *pSK, const char *pName);
ID3D11ShaderResourceView	*StuffKeeper_GetSRV(const StuffKeeper *pSK, const char *pName);
GrogFont					*StuffKeeper_GetFont(const StuffKeeper *pSK, const char *pName);
ID3D11ShaderResourceView	*StuffKeeper_GetFontSRV(const StuffKeeper *pSK, const char *pName);
const StringList			*StuffKeeper_GetVSEntryList(const StuffKeeper *pSK, const UT_string *szKey);
const StringList			*StuffKeeper_GetPSEntryList(const StuffKeeper *pSK, const UT_string *szKey);
const UT_string				*StuffKeeper_GetVSName(const StuffKeeper *pSK, const ID3D11VertexShader *pVS);
const UT_string				*StuffKeeper_GetPSName(const StuffKeeper *pSK, const ID3D11PixelShader *pPS);