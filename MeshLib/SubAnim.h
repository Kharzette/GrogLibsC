#pragma once
#include	<stdio.h>
#include	"KeyFrame.h"


typedef struct	SubAnim_t	SubAnim;
typedef struct	Skeleton_t	Skeleton;


SubAnim	*SubAnim_Read(FILE *f, const Skeleton *pSkel);
void	SubAnim_Animate(SubAnim *pSA, float time, bool bLooping);
