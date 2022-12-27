#pragma once

typedef struct	PostProcess_t		PostProcess;
typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	StuffKeeper_t		StuffKeeper;


PostProcess	*PP_Create(GraphicsDevice *pGD, StuffKeeper *pSK);