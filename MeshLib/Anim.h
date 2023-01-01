#pragma once
#include	<stdio.h>
#include	<utstring.h>

typedef struct	Anim_t		Anim;
typedef struct	Skeleton_t	Skeleton;


Anim		*Anim_Read(FILE *f, const Skeleton *pSkel);
UT_string	*Anim_GetName(Anim *pAnim);
void		Anim_Animate(Anim *pAnim, float time);