#pragma	once
#include	<stdint.h>
#include	<cglm/call.h>

typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	CBKeeper_t			CBKeeper;
typedef struct	GrogShapes_t		GrogShapes;

GrogShapes	*GShapes_Create(GraphicsDevice *pGD, const StuffKeeper *pSK, int maxVerts);
void		GShapes_FreeAll(GrogShapes *pGS);

void	GShapes_BeginDraw(GrogShapes *pGS);
void	GShapes_EndDraw(GrogShapes *pGS, GraphicsDevice *pGD);
void	GShapes_Render(GrogShapes *pGS, GraphicsDevice *pGD);

//shapes
void	GShapes_DrawRect(GrogShapes *pGS, float x, float y, float width, float height, vec4 color);