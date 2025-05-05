#pragma once
#include	<utstring.h>


typedef struct	Mesh_t				Mesh;
typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	Material_t			Material;
typedef struct	CBKeeper_t			CBKeeper;

//new gltf way
Mesh	*Mesh_Create(GraphicsDevice *pGD, const StuffKeeper *pSK,
	const UT_string *pName, void *pVData, const void *pIndData,
	const int vertElems[], int velCount,
	int	numVerts, int numInds, int vertSize);

const UT_string	*Mesh_GetName(const Mesh *pMesh);

Mesh	*Mesh_ReadFromFile(GraphicsDevice *pGD, StuffKeeper *pSK, const char *szFileName, bool bEditor);
Mesh	*Mesh_Read(GraphicsDevice *pGD, StuffKeeper *pSK, FILE *, bool bEditor);
void	Mesh_WriteToFile(const Mesh *pMesh, const char *szFileName);
void	Mesh_Write(const Mesh *pMesh, FILE *f);
void	Mesh_Destroy(Mesh *pMesh);
void	Mesh_SetName(Mesh *pMesh, const char *szNew);
void	Mesh_DrawMat(const Mesh *pMesh, GraphicsDevice *pGD, CBKeeper *pCBK, const Material *pMat);
void	Mesh_Draw(Mesh *pMesh, GraphicsDevice *pGD, StuffKeeper *pSK,
			const char *szVS, const char *szPS, const char *szTex);