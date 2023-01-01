#include	<stdint.h>
#include	<stdio.h>
#include	<string.h>
#include	<assert.h>
#include	<utstring.h>
#include	"../UtilityLib/StringStuff.h"
#include	"KeyFrame.h"


//this is a set of keys and times for a single bone
typedef struct	SubAnim_t
{
	KeyFrame	*mpKeys;
	float		*mpTimes;
	int			mNumKeys;

	float		mTotalTime;
	int			mLastTimeIndex;	//cache the last time used to
								//quicken key finding
	
	KeyFrame	*mpBone;	//pointer to affected bone
	UT_string	*szBoneName;
}	SubAnim;


SubAnim	*SubAnim_Read(FILE *f)
{
	SubAnim	*pRet	=malloc(sizeof(SubAnim));
	memset(pRet, 0, sizeof(SubAnim));

	pRet->szBoneName	=SZ_ReadString(f);

	int	numTimes;
	fread(&numTimes, sizeof(int), 1, f);

	pRet->mpTimes	=malloc(sizeof(float) * numTimes);
	fread(pRet->mpTimes, sizeof(float), numTimes, f);

	fread(&pRet->mNumKeys, sizeof(int), 1, f);

	assert(pRet->mNumKeys == numTimes);

	pRet->mpKeys	=malloc(sizeof(KeyFrame) * pRet->mNumKeys);
	for(int i=0;i < pRet->mNumKeys;i++)
	{
		KeyFrame_Read(f, &pRet->mpKeys[i]);
	}

	fread(&pRet->mTotalTime, sizeof(float), 1, f);

	return	pRet;
}


void	SubAnim_Animate(SubAnim *pSA, float time)
{	
	if(pSA->mpBone == NULL)
	{
		return;
	}

	//make sure the time is in range
	float	animTime	=time;
	if(time < pSA->mpTimes[0])
	{
		//bring into range
		animTime	=pSA->mpTimes[0];
	}
	else if(time > pSA->mpTimes[pSA->mNumKeys - 1])
	{
		animTime	=pSA->mpTimes[pSA->mNumKeys - 1];
	}

	//locate the key index to start with
	int	startIndex;
	for(startIndex = pSA->mLastTimeIndex;startIndex < pSA->mNumKeys;startIndex++)
	{
		if(startIndex > 0)
		{
			if(animTime < pSA->mpTimes[startIndex] && animTime >= pSA->mpTimes[startIndex - 1])
			{
				//back up one
				startIndex			=fmaxf(startIndex - 1, 0);
				pSA->mLastTimeIndex	=startIndex;
				break;	//found
			}
		}
		else
		{
			if(animTime <= pSA->mpTimes[startIndex])
			{
				//back up one
				startIndex			=fmaxf(startIndex - 1, 0);
				pSA->mLastTimeIndex	=startIndex;
				break;	//found
			}
		}
	}

	if(startIndex >= pSA->mNumKeys)
	{
		//wasn't found, search all
		for(startIndex = 0;startIndex < pSA->mNumKeys;startIndex++)
		{
			if(animTime <= pSA->mpTimes[startIndex])
			{
				//back up one
				startIndex			=fmaxf(startIndex - 1, 0);
				pSA->mLastTimeIndex	=startIndex;
				break;	//found
			}
		}
	}

	assert(startIndex < pSA->mNumKeys);

	//figure out the percentage between pos1 and pos2
	//get the deltatime
	float	percentage	=pSA->mpTimes[startIndex + 1] - pSA->mpTimes[startIndex];

	//convert to percentage
	percentage	=1.0f / percentage;

	//multiply by amount beyond p1
	percentage	*=(animTime - pSA->mpTimes[startIndex]);

	assert(percentage >= 0.0f && percentage <= 1.0f);

	KeyFrame_Lerp(&pSA->mpKeys[startIndex], &pSA->mpKeys[startIndex + 1], percentage, pSA->mpBone);
}