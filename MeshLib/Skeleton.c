#include	<stdint.h>
#include	<stdio.h>
#include	"GSNode.h"

//should match CommonFunctions.hlsli
#define	MAX_BONES			55


typedef struct	Skeleton_t
{
	GSNode	**mpRoots;
	int		mNumRoots;

	//indexed keyframes for shaderstuff
	KeyFrame	*mpKeys[MAX_BONES];

}	Skeleton;


Skeleton	*Skeleton_Read(FILE *f)
{
	Skeleton	*pRet	=malloc(sizeof(Skeleton));

	fread(&pRet->mNumRoots, sizeof(int), 1, f);

	pRet->mpRoots	=malloc(sizeof(GSNode *) * pRet->mNumRoots);

	//clear indexed pointers
	memset(pRet->mpKeys, 0, sizeof(KeyFrame *) * MAX_BONES);

	int	curIndex	=0;
	for(int i=0;i < pRet->mNumRoots;i++)
	{
		pRet->mpRoots[i]	=GSNode_Read(f);

		GSNode_SetBoneIndexes(pRet->mpRoots[i], &curIndex);

		//grab indexed bones
		for(int j=0;j < MAX_BONES;j++)
		{
			if(pRet->mpKeys[j] != NULL)
			{
				continue;	//already indexed
			}
			pRet->mpKeys[j]	=GSNode_GetKeyByIndex(pRet->mpRoots[i], j);
		}
	}
	return	pRet;
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


void	Skeleton_FillBoneArray(const Skeleton *pSkel, mat4 *pBones)
{
	for(int i=0;i < MAX_BONES;i++)
	{
		if(pSkel->mpKeys[i] == NULL)
		{
			continue;
		}
		KeyFrame_GetMatrix(pSkel->mpKeys[i], pBones[i]);
	}
}