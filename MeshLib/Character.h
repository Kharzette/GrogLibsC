#pragma once
#include	<cglm/call.h>

typedef struct	Character_t			Character;
typedef struct	Skeleton_t			Skeleton;
typedef struct	Skin_t				Skin;
typedef struct	StringList_t		StringList;
typedef struct	DictSZ_t			DictSZ;
typedef struct	MaterialLib_t		MaterialLib;
typedef struct	AnimLib_t			AnimLib;
typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	CBKeeper_t			CBKeeper;
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	Mesh_t				Mesh;


Character	*Character_Create(Skin *pSkin, Mesh *pMeshes[], int numMeshes);
Character	*Character_Read(GraphicsDevice *pGD, StuffKeeper *pSK,
							const char *szFileName, bool bEditor);
void		Character_Write(const Character *pChar, const char *szFileName);

void		Character_Destroy(Character *pChar);
void		Character_AssignMaterial(Character *pChar, int partIndex, const char *pMatName);
void		Character_ReNamePart(Character *pChar, const char *pOldName, const char *pNewName);
void		Character_DeletePartIndex(Character *pChar, int idx);
void		Character_DeletePart(Character *pChar, const char *szName);

int			Character_GetNumParts(const Character *pChar);
StringList	*Character_GetPartList(const Character *pChar);
const char	*Character_GetMaterialForPart(const Character *pChar, const char *szPartName);
const Skin	*Character_GetConstSkin(const Character *pChar);
Skin		*Character_GetSkin(const Character *pChar);

void	Character_Draw(const Character *pChar,
						const MaterialLib *pML, const AnimLib *pAL,
						GraphicsDevice *pGD, CBKeeper *pCBK);
