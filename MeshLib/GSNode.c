#include	"GSNode.h"
#include	<cglm/call.h>


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

void	GSNode_Write(const GSNode *pNode, FILE *f)
{
	SZ_WriteString(f, pNode->szName);

	KeyFrame_Write(&pNode->mKeyValue, f);

	fwrite(&pNode->mNumChildren, sizeof(int), 1, f);
	if(pNode->mNumChildren > 0)
	{
		for(int i=0;i < pNode->mNumChildren;i++)
		{
			GSNode_Write(pNode->mpChildren[i], f);
		}
	}
}

void	GSNode_Destroy(GSNode *pNode)
{
	utstring_done(pNode->szName);

	for(int i=0;i < pNode->mNumChildren;i++)
	{
		GSNode_Destroy(pNode->mpChildren[i]);
	}

	if(pNode->mNumChildren > 0)
	{
		free(pNode->mpChildren);
	}

	free(pNode);
}


const GSNode	*GSNode_GetConstNodeByName(const GSNode *pNode, const char * pName)
{
	if(strncmp(utstring_body(pNode->szName), pName, pNode->szName->i) == 0)
	{
		return	pNode;
	}

	for(int i=0;i < pNode->mNumChildren;i++)
	{
		const GSNode	*pN	=GSNode_GetConstNodeByName(pNode->mpChildren[i], pName);

		if(pN != NULL)
		{
			return	pN;	//found!
		}
	}
	return	NULL;
}

GSNode	*GSNode_GetNodeByName(GSNode *pNode, const char * pName)
{
	if(strncmp(utstring_body(pNode->szName), pName, pNode->szName->i) == 0)
	{
		return	pNode;
	}

	for(int i=0;i < pNode->mNumChildren;i++)
	{
		GSNode	*pN	=GSNode_GetNodeByName(pNode->mpChildren[i], pName);

		if(pN != NULL)
		{
			return	pN;	//found!
		}
	}
	return	NULL;
}

KeyFrame	*GSNode_GetKeyByName(GSNode *pNode, const char *pName)
{
	GSNode	*pN	=GSNode_GetNodeByName(pNode, pName);
	if(pN == NULL)
	{
		return	NULL;
	}

	return	&pN->mKeyValue;
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

			glm_mat4_mul(parent, mat, mat);

			return	true;	//found!
		}
	}
	glm_mat4_identity(mat);
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