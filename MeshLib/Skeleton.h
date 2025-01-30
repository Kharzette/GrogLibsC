#pragma once
#include	<stdio.h>
#include	<cglm/call.h>
#include	<utstring.h>

typedef struct	GSNode_t	GSNode;

//public structure
typedef struct	Skeleton_t
{
	GSNode	**mpRoots;
	int		mNumRoots;
}	Skeleton;

typedef struct	KeyFrame_t		KeyFrame;
typedef struct	StringList_t	StringList;

//file IO
Skeleton	*Skeleton_Read(FILE *f);
void		Skeleton_Write(const Skeleton *pSkel, FILE *f);

//searches
KeyFrame		*Skeleton_GetBoneKey(const Skeleton *pSkel, const char *szName);
const GSNode	*Skeleton_GetConstBoneByName(const Skeleton *pSkel, const char *szName);

void	Skeleton_FillBoneArray(const Skeleton *pSkel, mat4 *pBones);