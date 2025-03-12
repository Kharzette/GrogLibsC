#pragma once
#include	<cglm/call.h>

//should match CommonFunctions.hlsli
#define	MAX_BONES		55

typedef struct	AnimLib_t		AnimLib;
typedef struct	Anim_t			Anim;
typedef struct	Skeleton_t		Skeleton;
typedef struct	StringList_t	StringList;

AnimLib	*AnimLib_Create(Skeleton *pSkel);
AnimLib	*AnimLib_Read(const char *fileName);
void	AnimLib_Write(const AnimLib *pAL, const char *szFileName);

void			AnimLib_FillBoneArray(const AnimLib *pAL, mat4 *pBones);
const Skeleton	*AnimLib_GetSkeleton(const AnimLib *pAL);
int				AnimLib_GetNumAnims(const AnimLib *pAL);
StringList		*AnimLib_GetAnimList(const AnimLib *pAL);

void	AnimLib_ReName(AnimLib *pAL, const char *szOld, const char *szNew);
void	AnimLib_Delete(AnimLib *pAL, const char *szAnim);
void	AnimLib_Animate(AnimLib *pAL, const char *szAnimName, float time);
void	AnimLib_Add(AnimLib *pALib, Anim *pAnim);
void	AnimLib_AddForeign(AnimLib *pALib, Anim *pAnim, Skeleton *pForeignSkel);
void	AnimLib_SetBoneRefs(AnimLib *pAL);