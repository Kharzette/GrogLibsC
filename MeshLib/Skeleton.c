#include	<stdint.h>
#include	"GSNode.h"
#include	"Skeleton.h"
#include	"../UtilityLib/ListStuff.h"


//should match CommonFunctions.hlsli
#define	MAX_BONES			55


typedef struct	Skeleton_t
{
	GSNode	**mpRoots;
	int		mNumRoots;
}	Skeleton;


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


static	bool	GetMatrixForBoneIndex(const Skeleton *pSkel, int idx, mat4 mat)
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
		GetMatrixForBoneIndex(pSkel, i, pBones[i]);
	}
}


const StringList	*Skeleton_GetRootNames(const Skeleton *pSkel)
{
	StringList	*pRet	=SZList_New();

	for(int i=0;i < pSkel->mNumRoots;i++)
	{
		SZList_AddUT(&pRet,	GSNode_GetName(pSkel->mpRoots[i]));
	}
	return	pRet;
}


void	Skeleton_Iterate(const Skeleton *pSkel, Skeleton_IterateCB sicb, void *pContext)
{
	if(pSkel == NULL)
	{
		return;
	}

	for(int i=0;i < pSkel->mNumRoots;i++)
	{
		GSNode_Iterate(pSkel->mpRoots[i], sicb, pContext);
	}
}