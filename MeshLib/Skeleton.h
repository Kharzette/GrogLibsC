#pragma once
#include	<stdio.h>
#include	<cglm/call.h>
#include	<utstring.h>

typedef struct	Skeleton_t		Skeleton;
typedef struct	KeyFrame_t		KeyFrame;
typedef struct	StringList_t	StringList;

//iterate skeletal structure callback
typedef void (*Skeleton_IterateCB)(const UT_string *szName, const UT_string *szParent, void *pContext);


Skeleton	*Skeleton_Read(FILE *f);
void		Skeleton_Write(const Skeleton *pSkel, FILE *f);
KeyFrame	*Skeleton_GetBoneKey(const Skeleton *pSkel, const char *szName);
void		Skeleton_FillBoneArray(const Skeleton *pSkel, mat4 *pBones);
void		Skeleton_Iterate(const Skeleton *pSkel, Skeleton_IterateCB sicb, void *pContext);

const StringList	*Skeleton_GetRootNames(const Skeleton *pSkel);