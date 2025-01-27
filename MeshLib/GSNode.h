#pragma once
#include	<stdint.h>
#include	<stdio.h>
#include	<utstring.h>
#include	"../UtilityLib/StringStuff.h"
#include	"KeyFrame.h"
#include	"Skeleton.h"


typedef struct	GSNode_t	GSNode;


GSNode	*GSNode_Read(FILE *f);
void	GSNode_Write(const GSNode *pNode, FILE *f);

KeyFrame	*GSNode_GetKeyByName(GSNode *pNode, const char *pName);
KeyFrame	*GSNode_GetKeyByIndex(GSNode *pNode, int index);
void		GSNode_SetBoneIndexes(GSNode *pNode, int *pIndex);
bool		GSNode_GetMatrixForBoneIndex(const GSNode *pNode, int index, mat4 mat);
void		GSNode_Iterate(const GSNode *pNode, Skeleton_IterateCB sicb, void *pContext);

const UT_string	*GSNode_GetName(const GSNode *pNode);