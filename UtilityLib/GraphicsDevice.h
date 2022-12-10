#pragma	once
#include	<stdint.h>

typedef struct GraphicsDevice_t	GraphicsDevice;


extern bool	GraphicsDevice_Init(GraphicsDevice **ppGD, const char *pWindowTitle, int w, int h);
extern void	GraphicsDevice_Destroy(GraphicsDevice **ppGD);
