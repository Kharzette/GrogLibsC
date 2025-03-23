#pragma once
#include	<cglm/call.h>

//should match CommonFunctions.hlsli
#define	MAX_BONES		55

typedef struct	AnimLib_t		AnimLib;
typedef struct	Anim_t			Anim;
typedef struct	Skeleton_t		Skeleton;
typedef struct	StringList_t	StringList;


AnimLib	*AnimLib_Create(Skeleton *pSkel);
void	AnimLib_Destroy(AnimLib **ppAL);
AnimLib	*AnimLib_Read(const char *fileName);
void	AnimLib_Write(const AnimLib *pAL, const char *szFileName);

const Skeleton	*AnimLib_GetSkeleton(const AnimLib *pAL);
int				AnimLib_GetNumAnims(const AnimLib *pAL);
StringList		*AnimLib_GetAnimList(const AnimLib *pAL);
bool			AnimLib_CheckSkeleton(const AnimLib *pALib, const Skeleton *pForeignSkel);

void	AnimLib_ReName(AnimLib *pAL, const char *szOld, const char *szNew);
void	AnimLib_Delete(AnimLib *pAL, const char *szAnim);
void	AnimLib_Add(AnimLib *pALib, Anim *pAnim);
void	AnimLib_AddForeign(AnimLib *pALib, Anim *pAnim, const Skeleton *pForeignSkel);
void	AnimLib_SetBoneRefs(AnimLib *pAL);

void	AnimLib_Animate(AnimLib *pAL, const char *szAnimName, float time);
void	AnimLib_Blend(AnimLib *pAL, const char *szAnim1, const char *szAnim2,
	float anTime1, float anTime2, float percentage);