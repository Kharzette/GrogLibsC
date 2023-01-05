#pragma once
#include	<cglm/call.h>

typedef struct	AnimLib_t	AnimLib;
typedef struct	Skeleton_t	Skeleton;

extern AnimLib	*AnimLib_Read(const char *fileName);
extern void		AnimLib_Animate(AnimLib *pAL, const char *szAnimName, float time);
extern void		AnimLib_FillBoneArray(const AnimLib *pAL, mat4 *pBones);
Skeleton		*AnimLib_GetSkeleton(AnimLib *pAL);