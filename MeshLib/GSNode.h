#pragma once
#include	<stdint.h>
#include	<stdio.h>
#include	<utstring.h>
#include	"../UtilityLib/StringStuff.h"
#include	"KeyFrame.h"
#include	"Skeleton.h"

//public structure
typedef struct	GSNode_t
{
	UT_string	*szName;
	int			mIndex;

	struct GSNode_t	**mpChildren;	//list of children
	int				mNumChildren;

	//current pos / rot / scale
	KeyFrame	mKeyValue;
}	GSNode;


//file IO
GSNode	*GSNode_Read(FILE *f);
void	GSNode_Write(const GSNode *pNode, FILE *f);

//searches
const GSNode	*GSNode_GetConstNodeByName(const GSNode *pNode, const char * pName);
GSNode			*GSNode_GetNodeByName(GSNode *pNode, const char * pName);
KeyFrame		*GSNode_GetKeyByName(GSNode *pNode, const char *pName);
KeyFrame		*GSNode_GetKeyByIndex(GSNode *pNode, int index);
bool			GSNode_GetMatrixForBoneIndex(const GSNode *pNode, int index, mat4 mat);

void	GSNode_SetBoneIndexes(GSNode *pNode, int *pIndex);
void	GSNode_ConvertToLeftHanded(GSNode *pNode);