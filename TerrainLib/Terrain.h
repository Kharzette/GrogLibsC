#pragma once

typedef struct	Terrain_t			Terrain;
typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	StuffKeeper_t		StuffKeeper;


Terrain	*Terrain_Create(GraphicsDevice *pGD,
	const char *pName, const char *pPath);

void	Terrain_Draw(Terrain *pTer, GraphicsDevice *pGD);
