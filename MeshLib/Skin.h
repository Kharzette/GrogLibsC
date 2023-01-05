#pragma once
#include	<stdio.h>
#include	<cglm/call.h>


typedef struct	Skin_t		Skin;
typedef struct	Skeleton_t	Skeleton;

Skin	*Skin_Read(FILE *f);
void	Skin_FillBoneArray(const Skin *pSkin, const Skeleton *pSkel, mat4 *pBones);