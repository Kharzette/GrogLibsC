#pragma once
#include	<stdint.h>

//forward struct decs
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	GraphicsDevice_t	GraphicsDevice;


//eventually stuff here about loading shaders and textures
extern StuffKeeper	*StuffKeeper_Create(GraphicsDevice *pGD);