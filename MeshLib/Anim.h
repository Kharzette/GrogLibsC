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

void			Anim_Animate(Anim *pAnim, float time);
void			Anim_SetNameccp(Anim *pAnim, const char *szNew);
const UT_string	*Anim_GetName(const Anim *pAnim);
