#include	"GSNode.h"
#include	<cglm/call.h>


typedef struct	GSNode_t
{
	UT_string	*szName;
	int			mIndex;

	struct GSNode_t	**mpChildren;	//list of children
	int				mNumChildren;

	//current pos / rot / scale
	KeyFrame	mKeyValue;
}	GSNode;


GSNode	*GSNode_Read(FILE *f)
{
	GSNode	*pRet	=malloc(sizeof(GSNode));

	pRet->szName	=SZ_ReadString(f);

	KeyFrame_Read(f, &pRet->mKeyValue);

	int	numKids;
	fread(&numKids, sizeof(int), 1, f);

	if(numKids > 0)
	{
		pRet->mpChildren	=malloc(sizeof(GSNode *) * numKids);

		for(int i=0;i < numKids;i++)
		{
			pRet->mpChildren[i]	=GSNode_Read(f);
		}
	}
	else
	{
		pRet->mpChildren	=NULL;
	}
	pRet->mNumChildren	=numKids;

	return	pRet;
}


KeyFrame	*GSNode_GetKeyByName(GSNode *pNode, const char *pName)
{
	if(strncmp(utstring_body(pNode->szName), pName, pNode->szName->i) == 0)
	{
		return	&pNode->mKeyValue;
	}

	for(int i=0;i < pNode->mNumChildren;i++)
	{
		KeyFrame	*pKF	=GSNode_GetKeyByName(pNode->mpChildren[i], pName);

		if(pKF != NULL)
		{
			return	pKF;	//found!
		}
	}
	return	NULL;
}


bool	GSNode_GetMatrixForBoneIndex(const GSNode *pNode, int index, mat4 mat)
{
	if(pNode->mIndex == index)
	{
		KeyFrame_GetMatrix(&pNode->mKeyValue, mat);
		return	true;
	}

	for(int i=0;i < pNode->mNumChildren;i++)
	{
		bool	bFound	=GSNode_GetMatrixForBoneIndex(pNode->mpChildren[i], index, mat);
		if(bFound)
		{
			//mul by parent
			mat4	parent;
			KeyFrame_GetMatrix(&pNode->mKeyValue, parent);

			glmc_mat4_mul(parent, mat, mat);

			return	true;	//found!
		}
	}
	glmc_mat4_identity(mat);
	return	false;
}


KeyFrame	*GSNode_GetKeyByIndex(GSNode *pNode, int index)
{
	if(pNode->mIndex == index)
	{
		return	&pNode->mKeyValue;
	}

	for(int i=0;i < pNode->mNumChildren;i++)
	{
		KeyFrame	*pKF	=GSNode_GetKeyByIndex(pNode->mpChildren[i], index);
		if(pKF != NULL)
		{
			return	pKF;	//found!
		}
	}
	return	NULL;
}


//just sets in recursion order
void	GSNode_SetBoneIndexes(GSNode *pNode, int *pIndex)
{
	for(int i=0;i < pNode->mNumChildren;i++)
	{
		GSNode_SetBoneIndexes(pNode->mpChildren[i], pIndex);
	}

	pNode->mIndex	=*pIndex;
	(*pIndex)++;
}