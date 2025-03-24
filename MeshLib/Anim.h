#pragma once
#include	<stdio.h>
#include	<utstring.h>

typedef struct	Anim_t		Anim;
typedef struct	SubAnim_t	SubAnim;
typedef struct	Skeleton_t	Skeleton;


Anim	*Anim_Create(const UT_string *pName,
				SubAnim **ppSubs, int numSubs);

Anim	*Anim_Read(FILE *f, const Skeleton *pSkel);
void	Anim_Write(const Anim *pAnim, FILE *f);

void	Anim_Destroy(Anim *pAnim);

void	Anim_Animate(Anim *pAnim, float time);
void	Anim_Blend(Anim *pAnim1, Anim *pAnim2,
	float anTime1, float anTime2, float percentage);

void	Anim_SetNameccp(Anim *pAnim, const char *szNew);
void	Anim_SetLooping(Anim *pAnim, bool bLooping);
void	Anim_SetPingPong(Anim *pAnim, bool bPingPong);
void	Anim_ReMapBoneIndexes(Anim *pAnim, int boneMap[]);
void	Anim_SetBoneRefs(Anim *pAnim, Skeleton *pSkel);

const UT_string	*Anim_GetName(const Anim *pAnim);
bool			Anim_GetLooping(const Anim *pAnim);
bool			Anim_GetPingPong(const Anim *pAnim);