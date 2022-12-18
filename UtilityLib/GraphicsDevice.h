#pragma	once
#include	<stdint.h>
#include	<d3dcommon.h>
#include	<d3d11.h>

typedef struct GraphicsDevice_t	GraphicsDevice;


extern bool	GraphicsDevice_Init(GraphicsDevice **ppGD, const char *pWindowTitle, int w, int h, D3D_FEATURE_LEVEL desiredFeatureLevel);
extern void	GraphicsDevice_Destroy(GraphicsDevice **ppGD);

extern D3D_FEATURE_LEVEL	GraphicsDevice_GetFeatureLevel(GraphicsDevice *pGD);

extern ID3D11Texture2D	*GraphicsDevice_MakeTexture(GraphicsDevice *pGD,
							uint8_t **pRows, int w, int h, int rowPitch);

ID3D11VertexShader	*GraphicsDevice_CreateVertexShader(GraphicsDevice *pGD,
						uint8_t	*pCodeBytes, int codeLen);
ID3D11VertexShader	*GraphicsDevice_CreatePixelShader(GraphicsDevice *pGD,
						uint8_t	*pCodeBytes, int codeLen);