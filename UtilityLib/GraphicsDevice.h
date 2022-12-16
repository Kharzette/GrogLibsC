#pragma	once
#include	<stdint.h>
#include	<d3dcommon.h>

typedef struct GraphicsDevice_t	GraphicsDevice;


extern bool	GraphicsDevice_Init(GraphicsDevice **ppGD, const char *pWindowTitle, int w, int h, D3D_FEATURE_LEVEL desiredFeatureLevel);
extern void	GraphicsDevice_Destroy(GraphicsDevice **ppGD);

extern D3D_FEATURE_LEVEL	GraphicsDevice_GetFeatureLevel(GraphicsDevice *pGD);
