#pragma once
#include	<cglm/call.h>

typedef struct	AnimLib_t		AnimLib;
typedef struct	Skeleton_t		Skeleton;
typedef struct	StringList_t	StringList;

extern AnimLib	*AnimLib_Read(const char *fileName);
extern void		AnimLib_Animate(AnimLib *pAL, const char *szAnimName, float time);
extern void		AnimLib_FillBoneArray(const AnimLib *pAL, mat4 *pBones);
const Skeleton	*AnimLib_GetSkeleton(const AnimLib *pAL);
int				AnimLib_GetNumAnims(const AnimLib *pAL);
StringList		*AnimLib_GetAnimList(const AnimLib *pAL);
void			AnimLib_ReName(AnimLib *pAL, const char *szOld, const char *szNew);
void			AnimLib_Delete(AnimLib *pAL, const char *szAnim);