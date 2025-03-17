#pragma once
#include	<stdio.h>
#include	"KeyFrame.h"


typedef struct	SubAnim_t	SubAnim;
typedef struct	Skeleton_t	Skeleton;


SubAnim	*SubAnim_Create(const float *pTimes, KeyFrame *pKeys,
						int numKeys, KeyFrame *pBone, int boneIdx);

SubAnim	*SubAnim_Read(FILE *f, const Skeleton *pSkel);
void	SubAnim_Write(const SubAnim *pSA, FILE *f);

const KeyFrame	*SubAnim_GetBone(const SubAnim *pSA);
int				SubAnim_GetBoneIndex(const SubAnim *pSA);

void	SubAnim_Animate(SubAnim *pSA, float time, bool bLooping);
void	SubAnim_Destroy(SubAnim *pSA);
void	SubAnim_SetBone(SubAnim *pSA, KeyFrame *pBoneRef, int boneIdx);
SubAnim	*SubAnim_Merge(SubAnim *pSAT, SubAnim *pSAS, SubAnim *pSAR);
void	SubAnim_ReMapBoneIndex(SubAnim *pSA, int boneMap[]);
void	SubAnim_SetBoneRef(SubAnim *pSA, Skeleton *pSkel);