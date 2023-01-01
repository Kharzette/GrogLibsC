#pragma once
#include	<stdint.h>
#include	<stdio.h>
#include	<utstring.h>
#include	"../UtilityLib/StringStuff.h"
#include	"KeyFrame.h"


typedef struct	GSNode_t	GSNode;


GSNode	*GSNode_Read(FILE *f);

KeyFrame	*GSNode_GetKeyByName(GSNode *pNode, const char *pName);
KeyFrame	*GSNode_GetKeyByIndex(GSNode *pNode, int index);
void		GSNode_SetBoneIndexes(GSNode *pNode, int *pIndex);
bool		GSNode_GetMatrixForBoneIndex(const GSNode *pNode, int index, mat4 mat);
