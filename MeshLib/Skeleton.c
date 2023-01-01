#include	<stdint.h>
#include	<stdio.h>
#include	"GSNode.h"


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

	for(int i=0;i < pRet->mNumRoots;i++)
	{
		pRet->mpRoots[i]	=GSNode_Read(f);
	}
	return	pRet;
}