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

Character			*Character_Read(const char *szFileName);
void				Character_FillBoneArray(const Character *pChar, const Skeleton *pSkel, mat4 *pBones);
int					Character_GetNumParts(const Character *pChar);
const StringList	*Character_GetPartList(const Character *pChar);

void	Character_Draw(const Character *pChar, const DictSZ *pMeshes,
						const MaterialLib *pML, const AnimLib *pAL,
						GraphicsDevice *pGD, CBKeeper *pCBK);
