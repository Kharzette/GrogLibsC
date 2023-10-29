#pragma once
#include	<stdint.h>

//forward struct decs
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	GraphicsDevice_t	GraphicsDevice;


//eventually stuff here about loading shaders and textures
extern StuffKeeper	*StuffKeeper_Create(GraphicsDevice *pGD);

//raw png load, used externally by terrain stuff
BYTE **SK_LoadTextureBytes(const char *pPath, int *pOutRowPitch,
							uint32_t *pOutWidth, uint32_t *pOutHeight);

//stored stuff gets
extern ID3D11DepthStencilState	*StuffKeeper_GetDepthStencilState(StuffKeeper *pSK, const char *pStateName);
extern ID3D11BlendState			*StuffKeeper_GetBlendState(StuffKeeper *pSK, const char *pStateName);
extern ID3D11SamplerState		*StuffKeeper_GetSamplerState(StuffKeeper *pSK, const char *pStateName);
extern ID3D11Texture2D			*StuffKeeper_GetTexture2D(StuffKeeper *pSK, const char *pName);
extern ID3D11VertexShader		*StuffKeeper_GetVertexShader(StuffKeeper *pSK, const char *pName);
extern ID3D11PixelShader		*StuffKeeper_GetPixelShader(StuffKeeper *pSK, const char *pName);
extern ID3D11InputLayout		*StuffKeeper_GetInputLayout(StuffKeeper *pSK, const char *pName);
extern ID3D11ShaderResourceView	*StuffKeeper_GetSRV(StuffKeeper *pSK, const char *pName);
