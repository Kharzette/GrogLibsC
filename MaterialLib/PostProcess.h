#pragma once
#include	<stdint.h>

typedef struct	PostProcess_t		PostProcess;
typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	CBKeeper_t			CBKeeper;


PostProcess	*PP_Create(GraphicsDevice *pGD, StuffKeeper *pSK, CBKeeper *pCBK);

void	PP_MakePostTarget(PostProcess *pPP, GraphicsDevice *pGD,
	const char *szName, int resX, int resY, DXGI_FORMAT fmt);
void	PP_MakePostDepth(PostProcess *pPP, GraphicsDevice *pGD,
	const char *szName, int resX, int resY, DXGI_FORMAT fmt);


void PP_DrawStage(PostProcess *pPP, GraphicsDevice *pGD, CBKeeper *pCBK);


void PP_ClearTarget(PostProcess *pPP, GraphicsDevice *pGD, const char *szTarg);
void PP_ClearDepth(PostProcess *pPP, GraphicsDevice *pGD, const char *szDepth);
void PP_SetClearColor(PostProcess *pPP, const vec4 col);
void PP_SetTargets(PostProcess *pPP, GraphicsDevice *pGD, const char *szTarg, const char *szDepth);
void PP_SetSRV(PostProcess *pPP, GraphicsDevice *pGD, const char *szName, int slot);
