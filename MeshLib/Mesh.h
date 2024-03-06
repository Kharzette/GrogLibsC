#pragma once


typedef struct	Mesh_t				Mesh;
typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	Material_t			Material;
typedef struct	CBKeeper_t			CBKeeper;


Mesh	*Mesh_Read(GraphicsDevice *pGD, StuffKeeper *pSK, const char *szFileName);
void	Mesh_DrawMat(Mesh *pMesh, GraphicsDevice *pGD, CBKeeper *pCBK, Material *pMat);
void	Mesh_Draw(Mesh *pMesh, GraphicsDevice *pGD, StuffKeeper *pSK,
			const char *szVS, const char *szPS, const char *szTex);