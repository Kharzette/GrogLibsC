#pragma once
#include	<stdio.h>
#include	<cglm/call.h>

typedef struct	Skeleton_t	Skeleton;
typedef struct	KeyFrame_t	KeyFrame;

Skeleton	*Skeleton_Read(FILE *f);
void		Skeleton_Write(const Skeleton *pSkel, FILE *f);
KeyFrame	*Skeleton_GetBoneKey(const Skeleton *pSkel, const char *szName);
void		Skeleton_FillBoneArray(const Skeleton *pSkel, mat4 *pBones);