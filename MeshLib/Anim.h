#pragma once
#include	<stdio.h>
#include	<utstring.h>

typedef struct	Anim_t	Anim;


Anim		*Anim_Read(FILE *f);
UT_string	*Anim_GetName(Anim *pAnim);
void		Anim_Animate(Anim *pAnim, float time);