#pragma once
#include	<cglm/call.h>

typedef struct	Character_t			Character;
typedef struct	Skeleton_t			Skeleton;
typedef struct	StringList_t		StringList;
typedef struct	DictSZ_t			DictSZ;
typedef struct	MaterialLib_t		MaterialLib;
typedef struct	AnimLib_t			AnimLib;
typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	CBKeeper_t			CBKeeper;

Character	*Character_Read(const char *szFileName);
void		Character_Write(const Character *pChar, const char *szFileName);
void		Character_Destroy(Character *pChar);
void		Character_AssignMaterial(Character *pChar, int partIndex, const char *pMatName);
int			Character_GetNumParts(const Character *pChar);
StringList	*Character_GetPartList(const Character *pChar);
void		Character_ReNamePart(Character *pChar, const char *pOldName, const char *pNewName);
void		Character_DeletePartIndex(Character *pChar, int idx);
void		Character_DeletePart(Character *pChar, const char *szName);
const char	*Character_GetMaterialForPart(const Character *pChar, const char *szPartName);

void	Character_Draw(const Character *pChar, const DictSZ *pMeshes,
						const MaterialLib *pML, const AnimLib *pAL,
						GraphicsDevice *pGD, CBKeeper *pCBK);
