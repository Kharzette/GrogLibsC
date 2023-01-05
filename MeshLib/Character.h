#pragma once
#include	<cglm/call.h>

typedef struct	Character_t	Character;
typedef struct	Skeleton_t	Skeleton;

Character	*Character_Read(const char *szFileName);
void	Character_FillBoneArray(const Character *pChar, const Skeleton *pSkel, mat4 *pBones);