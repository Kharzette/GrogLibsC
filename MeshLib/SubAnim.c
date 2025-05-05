#include	<stdint.h>
#include	<stdio.h>
#include	<string.h>
#include	<assert.h>
#include	<utstring.h>
#include	"../UtilityLib/StringStuff.h"
#include	"KeyFrame.h"
#include	"Skeleton.h"


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
	int			mBoneIndex;

}	SubAnim;


//static forward decs
static void	sAddKeyAtTime(SubAnim *pSA, float t);
static void	sFillGaps(SubAnim *pSAT, SubAnim *pSAS, SubAnim *pSAR);
static void	sAnimateKey(SubAnim *pSA, float time, bool bLooping, KeyFrame *pDest);


SubAnim	*SubAnim_Create(const float *pTimes, KeyFrame *pKeys,
	int numKeys, KeyFrame *pBone, int boneIdx)
{
	SubAnim	*pRet	=malloc(sizeof(SubAnim));
	memset(pRet, 0, sizeof(SubAnim));

	//this is mallocated so just point to it
	pRet->mpKeys	=pKeys;

	//copy times, they come from bin
	pRet->mpTimes	=malloc(sizeof(float) * numKeys);
	memcpy(pRet->mpTimes, pTimes, sizeof(float) * numKeys);

	pRet->mNumKeys	=numKeys;

	pRet->mpBone		=pBone;
	pRet->mBoneIndex	=boneIdx;

	//compute totaltime
	for(int i=0;i < numKeys;i++)
	{
		if(pTimes[i] > pRet->mTotalTime)
		{
			pRet->mTotalTime	=pTimes[i];
		}
	}
	return	pRet;
}

SubAnim	*SubAnim_Read(FILE *f, const Skeleton *pSkel)
{
	SubAnim	*pRet	=malloc(sizeof(SubAnim));
	memset(pRet, 0, sizeof(SubAnim));

	fread(&pRet->mBoneIndex, sizeof(int), 1, f);

	pRet->mpBone	=Skeleton_GetBoneKeyByIndex(pSkel, pRet->mBoneIndex);
	if(pRet->mpBone == NULL)
	{
		printf("Warning: Bone %d not found in skeleton.\n", pRet->mBoneIndex);
	}

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

void	SubAnim_Write(const SubAnim *pSA, FILE *f)
{	
	fwrite(&pSA->mBoneIndex, sizeof(int), 1, f);

	fwrite(&pSA->mNumKeys, sizeof(int), 1, f);

	fwrite(pSA->mpTimes, sizeof(float), pSA->mNumKeys, f);

	fwrite(&pSA->mNumKeys, sizeof(int), 1, f);

	for(int i=0;i < pSA->mNumKeys;i++)
	{
		KeyFrame_Write(&pSA->mpKeys[i], f);
	}

	fwrite(&pSA->mTotalTime, sizeof(float), 1, f);
}

void	SubAnim_Animate(SubAnim *pSA, float time, bool bLooping)
{	
	if(pSA->mpBone == NULL)
	{
		return;
	}

	sAnimateKey(pSA, time, bLooping, pSA->mpBone);
}

void	SubAnim_Blend(SubAnim *pSA0, SubAnim *pSA1,
	float time0, float time1, float percentage)
{	
	if(pSA0->mpBone == NULL)
	{
		return;
	}

	assert(pSA0->mpBone == pSA1->mpBone);
	assert(pSA0->mBoneIndex == pSA1->mBoneIndex);

	KeyFrame	blend0, blend1;

	//animate into temp keys
	sAnimateKey(pSA0, time0, true, &blend0);
	sAnimateKey(pSA1, time1, true, &blend1);

	//lerp into original bone
	KeyFrame_Lerp(&blend0, &blend1, percentage, pSA0->mpBone);
}

void	SubAnim_Destroy(SubAnim *pSA)
{
	free(pSA->mpKeys);
	free(pSA->mpTimes);

	free(pSA);
}


const KeyFrame	*SubAnim_GetBone(const SubAnim *pSA)
{
	return	pSA->mpBone;
}

int	SubAnim_GetBoneIndex(const SubAnim *pSA)
{
	return	pSA->mBoneIndex;
}


void	SubAnim_SetBoneRef(SubAnim *pSA, Skeleton *pSkel)
{
	pSA->mpBone	=Skeleton_GetBoneKeyByIndex(pSkel, pSA->mBoneIndex);
}

void	SubAnim_SetBone(SubAnim *pSA, KeyFrame *pBoneRef, int boneIdx)
{
	pSA->mpBone		=pBoneRef;
	pSA->mBoneIndex	=boneIdx;
}

void	SubAnim_ReMapBoneIndex(SubAnim *pSA, int boneMap[])
{
	pSA->mBoneIndex	=boneMap[pSA->mBoneIndex];
}

//take 3 sub anims that operate only on T S R
//and combine them into a single SubAnim
SubAnim	*SubAnim_Merge(SubAnim *pSAT, SubAnim *pSAS, SubAnim *pSAR)
{
	SubAnim	*pRet	=malloc(sizeof(SubAnim));
	memset(pRet, 0, sizeof(SubAnim));

	sFillGaps(pSAT, pSAS, pSAR);

	assert(pSAT->mNumKeys == pSAS->mNumKeys);
	assert(pSAT->mNumKeys == pSAR->mNumKeys);

	pRet->mBoneIndex	=pSAT->mBoneIndex;
	pRet->mNumKeys		=pSAT->mNumKeys;
	pRet->mpBone		=pSAT->mpBone;
	pRet->mTotalTime	=pSAT->mTotalTime;

	pRet->mpKeys	=malloc(sizeof(KeyFrame) * pRet->mNumKeys);
	pRet->mpTimes	=malloc(sizeof(float) * pRet->mNumKeys);

	//copy times
	memcpy(pRet->mpTimes, pSAT->mpTimes, sizeof(float) * pRet->mNumKeys);

	//merge keys
	for(int i=0;i < pRet->mNumKeys;i++)
	{
		glm_vec3_copy(pSAT->mpKeys[i].mPosition, pRet->mpKeys[i].mPosition);
		glm_vec3_copy(pSAS->mpKeys[i].mScale, pRet->mpKeys[i].mScale);
		glm_vec4_copy(pSAR->mpKeys[i].mRotation, pRet->mpKeys[i].mRotation);
	}

	return	pRet;
}


static void	sAddKeyAtTime(SubAnim *pSA, float t)
{
	//ensure the key isn't already there
	for(int i=0;i < pSA->mNumKeys;i++)
	{
		//danger!  comparing floats
		if(pSA->mpTimes[i] == t)
		{
			return;
		}
	}

	//animate to time t
	SubAnim_Animate(pSA, t, true);

	//make new times and keys storage
	float		*pNewT	=malloc(sizeof(float) * (pSA->mNumKeys + 1));
	KeyFrame	*pNewKF	=malloc(sizeof(KeyFrame) * (pSA->mNumKeys + 1));

	//search for time
	int	insertIndex	=-1;
	for(int i=0;i < pSA->mNumKeys;i++)
	{
		if(pSA->mpTimes[i] > t)
		{
			insertIndex	=i;
			break;
		}
	}

	if(insertIndex == -1)
	{
		//on the far end...
		//animated key might look strange?
		insertIndex	=pSA->mNumKeys;
	}

	//examples
	//0 0.1 0.3 0.6 0.7 1.2 1.4
	//0.5 t
	//insertIndex is 3
	//
	//0.6 0.7 1.2 1.4 1.8 2.5
	//0.5 t
	//insertIndex is 0
	//
	//0 0.1 0.3 0.6 0.7 1.2 1.4
	//1.5 t
	//insertIndex is 6
	//

	if(insertIndex != 0)
	{
		memcpy(pNewT, pSA->mpTimes, sizeof(float) * insertIndex);
		memcpy(pNewKF, pSA->mpKeys, sizeof(KeyFrame) * insertIndex);
	}

	pNewT[insertIndex]	=t;

	//copy animated values
	glm_vec3_copy(pSA->mpBone->mPosition, pNewKF[insertIndex].mPosition);
	glm_vec3_copy(pSA->mpBone->mScale, pNewKF[insertIndex].mScale);
	glm_vec4_copy(pSA->mpBone->mRotation, pNewKF[insertIndex].mRotation);

	int	remaining	=pSA->mNumKeys - insertIndex;
	if(remaining > 0)
	{
		//copy rest of stuff
		memcpy(&pNewT[insertIndex + 1], &pSA->mpTimes[insertIndex], sizeof(float) * remaining);
		memcpy(&pNewKF[insertIndex + 1], &pSA->mpKeys[insertIndex], sizeof(KeyFrame) * remaining);
	}

	//free old
	free(pSA->mpTimes);
	free(pSA->mpKeys);

	//assign new
	pSA->mpTimes	=pNewT;
	pSA->mpKeys		=pNewKF;

	pSA->mNumKeys++;
}

//take 3 sub anims that operate only on T S R
//and fill any keyframe gaps so they can be merged
static void	sFillGaps(SubAnim *pSAT,
	SubAnim *pSAS, SubAnim *pSAR)
{
	//if all have the same number of keys, done
	if(pSAT->mNumKeys == pSAS->mNumKeys
		&& pSAT->mNumKeys == pSAR->mNumKeys)
	{
		return;
	}

	if(pSAT->mNumKeys >= pSAS->mNumKeys
		&& pSAT->mNumKeys >= pSAR->mNumKeys)
	{
		//use T keys to fill in the others
		for(int i=0;i < pSAT->mNumKeys;i++)
		{
			float	t	=pSAT->mpTimes[i];

			sAddKeyAtTime(pSAS, t);
			sAddKeyAtTime(pSAR, t);
		}
	}
	else if(pSAS->mNumKeys >= pSAT->mNumKeys
		&& pSAS->mNumKeys >= pSAR->mNumKeys)
	{
		//use S keys to fill in the others
		for(int i=0;i < pSAS->mNumKeys;i++)
		{
			float	t	=pSAS->mpTimes[i];

			sAddKeyAtTime(pSAT, t);
			sAddKeyAtTime(pSAR, t);
		}
	}
	else
	{
		//use R keys to fill in the others
		assert(pSAR->mNumKeys >= pSAT->mNumKeys
			&& pSAR->mNumKeys >= pSAS->mNumKeys);

		for(int i=0;i < pSAR->mNumKeys;i++)
		{
			float	t	=pSAR->mpTimes[i];

			sAddKeyAtTime(pSAS, t);
			sAddKeyAtTime(pSAT, t);
		}
	}
}

void	sAnimateKey(SubAnim *pSA, float time, bool bLooping, KeyFrame *pDest)
{	
	if(pDest == NULL)
	{
		return;
	}

	//make sure the time is in range
	float	animTime;
	if(bLooping)
	{
		animTime	=fmodf(time, pSA->mTotalTime);
	}
	else
	{
		animTime	=time;
	}

	if(animTime < pSA->mpTimes[0])
	{
		//bring into range
		animTime	=pSA->mpTimes[0];
	}
	else if(animTime > pSA->mpTimes[pSA->mNumKeys - 1])
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
				startIndex			=GLM_MAX(startIndex - 1, 0);
				pSA->mLastTimeIndex	=startIndex;
				break;	//found
			}
		}
		else
		{
			if(animTime <= pSA->mpTimes[startIndex])
			{
				//back up one
				startIndex			=GLM_MAX(startIndex - 1, 0);
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
				startIndex			=GLM_MAX(startIndex - 1, 0);
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

	KeyFrame_Lerp(&pSA->mpKeys[startIndex], &pSA->mpKeys[startIndex + 1], percentage, pDest);
}