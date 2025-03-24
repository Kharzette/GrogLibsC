#include	<stdint.h>
#include	"GSNode.h"
#include	"Skeleton.h"
#include	"../UtilityLib/DictionaryStuff.h"
#include	"../UtilityLib/StringStuff.h"


//should match CommonFunctions.hlsli
#define	MAX_BONES			55

//static forward decs
static void	srMakeNameDict(Skeleton *pSkel, GSNode *pNode);


Skeleton	*Skeleton_Create(GSNode *pRoot)
{
	Skeleton	*pRet	=malloc(sizeof(Skeleton));

	pRet->mpRoot	=pRoot;

	DictSZ_New(&pRet->mpNameToIndex);

	srMakeNameDict(pRet, pRoot);

	return	pRet;
}

Skeleton	*Skeleton_Read(FILE *f)
{
	Skeleton	*pRet	=malloc(sizeof(Skeleton));

	int	curIndex	=0;

	pRet->mpRoot	=GSNode_Read(f);

	GSNode_SetBoneIndexes(pRet->mpRoot, &curIndex);

	srMakeNameDict(pRet, pRet->mpRoot);
	
	return	pRet;
}

void	Skeleton_Write(const Skeleton *pSkel, FILE *f)
{
	GSNode_Write(pSkel->mpRoot, f);
}


const GSNode	*Skeleton_GetConstBoneByName(const Skeleton *pSkel, const char *szName)
{
	return	GSNode_GetConstNodeByName(pSkel->mpRoot, szName);
}

KeyFrame	*Skeleton_GetBoneKey(const Skeleton *pSkel, const char *szName)
{
	return	GSNode_GetKeyByName(pSkel->mpRoot, szName);
}

KeyFrame	*Skeleton_GetBoneKeyByIndex(const Skeleton *pSkel, int index)
{
	return	GSNode_GetKeyByIndex(pSkel->mpRoot, index);
}

//Warning: this does some allocations!
GSNode	*Skeleton_GetBoneMirror(const Skeleton *pSkel, const UT_string *pName)
{
	//see if the bone exists
	GSNode	*pNode	=GSNode_GetNodeByName(pSkel->mpRoot, utstring_body(pName));
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
	pNode	=GSNode_GetNodeByName(pSkel->mpRoot, utstring_body(pMirror));

	utstring_done(pMirror);

	return	pNode;
}

void	Skeleton_Destroy(Skeleton *pSkel)
{
	//the pointers in this are just ints, no need to free
	DictSZ_ClearNoFree(&pSkel->mpNameToIndex);

	GSNode_Destroy(pSkel->mpRoot);

	free(pSkel);
}


bool	Skeleton_GetMatrixForBoneIndex(const Skeleton *pSkel, int idx, mat4 mat)
{
	if(GSNode_GetMatrixForBoneIndex(pSkel->mpRoot, idx, mat))
	{
//		printf("of index %d\n", idx);
		return	true;
	}
	return	false;
}

void	Skeleton_FillBoneArray(const Skeleton *pSkel,
	const uint8_t joints[], mat4 *pBones, int numBones)
{
	assert(numBones < MAX_BONES);

	for(int i=0;i < numBones;i++)
	{
		Skeleton_GetMatrixForBoneIndex(pSkel, joints[i], pBones[i]);
	}
}


static void	srMakeNameDict(Skeleton *pSkel, GSNode *pNode)
{
	printf("srMakeNameDict: Adding to name dictionary %s index %d\n", utstring_body(pNode->szName), pNode->mIndex);
	DictSZ_Add(&pSkel->mpNameToIndex, pNode->szName, (void *)pNode->mIndex);

	for(int i=0;i < pNode->mNumChildren;i++)
	{
		srMakeNameDict(pSkel, pNode->mpChildren[i]);
	}
}