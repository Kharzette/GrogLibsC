#pragma once
#include	<stdio.h>
#include	<cglm/call.h>
#include	<utstring.h>

typedef struct	GSNode_t	GSNode;
typedef struct	DictSZ_t	DictSZ;

//public structure
typedef struct	Skeleton_t
{
	GSNode	*mpRoot;

	DictSZ	*mpNameToIndex;
}	Skeleton;

typedef struct	KeyFrame_t		KeyFrame;
typedef struct	StringList_t	StringList;

Skeleton	*Skeleton_Create(GSNode *pRoot);
void		Skeleton_Destroy(Skeleton *pSkel);

//file IO
Skeleton	*Skeleton_Read(FILE *f);
void		Skeleton_Write(const Skeleton *pSkel, FILE *f);

//searches
KeyFrame		*Skeleton_GetBoneKeyByIndex(const Skeleton *pSkel, int index);
KeyFrame		*Skeleton_GetBoneKey(const Skeleton *pSkel, const char *szName);
const GSNode	*Skeleton_GetConstBoneByName(const Skeleton *pSkel, const char *szName);
GSNode			*Skeleton_GetBoneMirror(const Skeleton *pSkel, const UT_string *pName);

bool	Skeleton_GetMatrixForBoneIndex(const Skeleton *pSkel, int idx, mat4 mat);
void	Skeleton_FillBoneArray(const Skeleton *pSkel,
				const uint8_t joints[], mat4 *pBones, int numBones);
