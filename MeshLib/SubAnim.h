#pragma once
#include	<stdio.h>
#include	"KeyFrame.h"


typedef struct	SubAnim_t	SubAnim;

SubAnim	*SubAnim_Read(FILE *f);
void	SubAnim_Animate(SubAnim *pSA, float time);
