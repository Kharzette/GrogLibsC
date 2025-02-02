#pragma once
#include	<stdio.h>
#include	<cglm/call.h>

#define	BONE_COL_SHAPE_BOX		0
#define	BONE_COL_SHAPE_SPHERE	1
#define	BONE_COL_SHAPE_CAPSULE	2
#define	BONE_COL_SHAPE_INVALID	3

typedef struct	Skin_t		Skin;
typedef struct	Skeleton_t	Skeleton;

Skin	*Skin_Read(FILE *f);
void	Skin_Write(const Skin *pSkin, FILE *f);
void	Skin_Destroy(Skin *pSkin);
void	Skin_FillBoneArray(const Skin *pSkin, const Skeleton *pSkel, mat4 *pBones);
int		Skin_GetBoundChoice(const Skin *pSkin, int boneIdx);