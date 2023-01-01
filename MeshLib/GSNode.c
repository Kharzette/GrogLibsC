#include	"GSNode.h"


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