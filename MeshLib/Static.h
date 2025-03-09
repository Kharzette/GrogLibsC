#pragma once
#include	<cglm/call.h>

typedef struct	Static_t			Static;
typedef struct	StringList_t		StringList;
typedef struct	DictSZ_t			DictSZ;
typedef struct	MaterialLib_t		MaterialLib;
typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	CBKeeper_t			CBKeeper;
typedef struct	StuffKeeper_t		StuffKeeper;


Static	*Static_Create(Mesh *pMeshes[], int numMeshes);
Static	*Static_Read(GraphicsDevice *pGD, StuffKeeper *pSK,
					const char *szFileName, bool bEditor);
void	Static_Write(const Static *pStat, const char *szFileName);

void	Static_Draw(const Static *pStat, MaterialLib *pML,
					GraphicsDevice *pGD, CBKeeper *pCBK);

int			Static_GetNumParts(const Static *pStat);
StringList	*Static_GetPartList(const Static *pStat);
void		Static_ReNamePart(Static *pStat, const char *pOldName, const char *pNewName);
void		Static_AssignMaterial(Static *pStat, int partIndex, const char *pMatName);

void	Static_DeletePartIndex(Static *pStat, int idx);
void	Static_DeletePart(Static *pStat, const char *szName);
void	Static_Destroy(Static *pStat);

const char	*Static_GetMaterialForPart(const Static *pStat, const char *szPartName);