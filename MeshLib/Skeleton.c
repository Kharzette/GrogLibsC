#include	<stdint.h>
#include	"GSNode.h"
#include	"Skeleton.h"
#include	"../UtilityLib/ListStuff.h"
#include	"../UtilityLib/StringStuff.h"


//should match CommonFunctions.hlsli
#define	MAX_BONES			55


Skeleton	*Skeleton_Read(FILE *f)
{
	Skeleton	*pRet	=malloc(sizeof(Skeleton));

	fread(&pRet->mNumRoots, sizeof(int), 1, f);

	pRet->mpRoots	=malloc(sizeof(GSNode *) * pRet->mNumRoots);

	int	curIndex	=0;
	for(int i=0;i < pRet->mNumRoots;i++)
	{
		pRet->mpRoots[i]	=GSNode_Read(f);

		GSNode_SetBoneIndexes(pRet->mpRoots[i], &curIndex);
	}
	return	pRet;
}

void	Skeleton_Write(const Skeleton *pSkel, FILE *f)
{
	fwrite(&pSkel->mNumRoots, sizeof(int), 1, f);

	for(int i=0;i < pSkel->mNumRoots;i++)
	{
		GSNode_Write(pSkel->mpRoots[i], f);
	}
}


const GSNode	*Skeleton_GetConstBoneByName(const Skeleton *pSkel, const char *szName)
{
	const GSNode	*pRet	=NULL;
	for(int i=0;i < pSkel->mNumRoots;i++)
	{
		pRet	=GSNode_GetConstNodeByName(pSkel->mpRoots[i], szName);

		if(pRet != NULL)
		{
			return	pRet;	//found!
		}
	}
	return	NULL;
}

KeyFrame	*Skeleton_GetBoneKey(const Skeleton *pSkel, const char *szName)
{
	KeyFrame	*pRet	=NULL;
	for(int i=0;i < pSkel->mNumRoots;i++)
	{
		pRet	=GSNode_GetKeyByName(pSkel->mpRoots[i], szName);

		if(pRet != NULL)
		{
			return	pRet;	//found!
		}
	}
	return	NULL;
}

//Warning: this does some allocations!
GSNode	*Skeleton_GetBoneMirror(const Skeleton *pSkel, const UT_string *pName)
{
	//see if the bone exists
	GSNode	*pNode	=NULL;
	for(int i=0;i < pSkel->mNumRoots;i++)
	{
		pNode	=GSNode_GetNodeByName(pSkel->mpRoots[i], utstring_body(pName));
		if(pNode != NULL)
		{
			break;
		}
	}

	if(pNode == NULL)
	{
		return	NULL;
	}

	UT_string	*pMirror;
	utstring_new(pMirror);

	utstring_concat(pMirror, pName);
	int	len	=utstring_len(pMirror);

	if(SZ_ContainsUTCC(pMirror, "Left"))
	{
		SZ_ReplaceUTCCCC(pMirror, "Left", "Right");
	}
	else if(SZ_ContainsUTCC(pMirror, "Right"))
	{
		SZ_ReplaceUTCCCC(pMirror, "Right", "Left");
	}
	else if(SZ_EndsWithUT(pMirror, 'L'))
	{
		UT_string	*pBeg	=SZ_SubStringUTStartEnd(pMirror, 0, len - 1);
		utstring_clear(pMirror);
		utstring_printf(pMirror, "%sR", utstring_body(pBeg));
		utstring_done(pBeg);
	}
	else if(SZ_EndsWithUT(pMirror, 'R'))
	{
		UT_string	*pBeg	=SZ_SubStringUTStartEnd(pMirror, 0, len - 1);
		utstring_clear(pMirror);
		utstring_printf(pMirror, "%sL", utstring_body(pBeg));
		utstring_done(pBeg);
	}
	else if(SZ_ContainsUTCC(pMirror, "_L_"))
	{
		SZ_ReplaceUTCCCC(pMirror, "_L_", "_R_");
	}
	else if(SZ_ContainsUTCC(pMirror, "_R_"))
	{
		SZ_ReplaceUTCCCC(pMirror, "_R_", "_L_");
	}
	else
	{
		//no leftish rightish stuff found
		utstring_done(pMirror);
		return	NULL;
	}

	//see if the mirror exists
	pNode	=NULL;
	for(int i=0;i < pSkel->mNumRoots;i++)
	{
		pNode	=GSNode_GetNodeByName(pSkel->mpRoots[i], utstring_body(pMirror));
		if(pNode != NULL)
		{
			break;
		}
	}

	utstring_done(pMirror);

	if(pNode == NULL)
	{
		return	NULL;
	}
	return	pNode;
}


bool	Skeleton_GetMatrixForBoneIndex(const Skeleton *pSkel, int idx, mat4 mat)
{
	for(int i=0;i < pSkel->mNumRoots;i++)
	{
		if(GSNode_GetMatrixForBoneIndex(pSkel->mpRoots[i], idx, mat))
		{
//			printf("of index %d\n", idx);
			return	true;
		}
	}
	return	false;
}

void	Skeleton_FillBoneArray(const Skeleton *pSkel, mat4 *pBones)
{
	for(int i=0;i < MAX_BONES;i++)
	{
		Skeleton_GetMatrixForBoneIndex(pSkel, i, pBones[i]);
	}
}