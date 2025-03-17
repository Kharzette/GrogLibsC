#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<utstring.h>
#include	"../UtilityLib/StringStuff.h"
#include	"SubAnim.h"
#include	"Skeleton.h"


//animation!
typedef struct	Anim_t
{
	SubAnim	**mpSubAnims;
	int		mNumSubAnims;

	UT_string	*szName;	//name in the animation library
	bool		mbLooping;
	bool		mbPingPong;
}	Anim;


Anim	*Anim_Create(const UT_string *pName, SubAnim **ppSubs, int numSubs)
{
	Anim	*pRet	=malloc(sizeof(Anim));

	utstring_new(pRet->szName);
	utstring_printf(pRet->szName, "%s", utstring_body(pName));

	//this is mallocated so can just grab
	pRet->mpSubAnims	=ppSubs;

	pRet->mNumSubAnims	=numSubs;

	pRet->mbLooping		=true;
	pRet->mbPingPong	=false;

	return	pRet;
}

Anim	*Anim_Read(FILE *f, const Skeleton *pSkel)
{
	Anim	*pRet	=malloc(sizeof(Anim));

	pRet->szName	=SZ_ReadString(f);

	fread(&pRet->mbLooping, sizeof(bool), 1, f);
	fread(&pRet->mbPingPong, sizeof(bool), 1, f);

	int	numSA;
	fread(&numSA, sizeof(int), 1, f);

	pRet->mpSubAnims	=malloc(sizeof(SubAnim *) * numSA);

	for(int i=0;i < numSA;i++)
	{
		pRet->mpSubAnims[i]	=SubAnim_Read(f, pSkel);
	}
	pRet->mNumSubAnims	=numSA;

	return	pRet;
}

void	Anim_Write(const Anim *pAnim, FILE *f)
{
	SZ_WriteString(f, pAnim->szName);

	fwrite(&pAnim->mbLooping, sizeof(bool), 1, f);
	fwrite(&pAnim->mbPingPong, sizeof(bool), 1, f);

	fwrite(&pAnim->mNumSubAnims, sizeof(int), 1, f);

	for(int i=0;i < pAnim->mNumSubAnims;i++)
	{
		SubAnim_Write(pAnim->mpSubAnims[i], f);
	}
}


void	Anim_Animate(Anim *pAnim, float time)
{
	for(int i=0;i < pAnim->mNumSubAnims;i++)
	{
		SubAnim_Animate(pAnim->mpSubAnims[i], time, true);//pAnim->mbLooping);
	}
}

void	Anim_Blend(Anim *pAnim1, Anim *pAnim2,
	float anTime1, float anTime2, float percentage)
{
	assert(pAnim1->mNumSubAnims == pAnim2->mNumSubAnims);

	for(int i=0;i < pAnim1->mNumSubAnims;i++)
	{
		SubAnim	*pSA1	=pAnim1->mpSubAnims[i];
		SubAnim	*pSA2	=pAnim2->mpSubAnims[i];

		//save the subanim's referenced bone
		const KeyFrame	*pBone1	=SubAnim_GetBone(pSA1);
		const KeyFrame	*pBone2	=SubAnim_GetBone(pSA2);

		assert(pBone1 == pBone2);

		int	idx1	=SubAnim_GetBoneIndex(pSA1);
		int	idx2	=SubAnim_GetBoneIndex(pSA2);

		assert(idx1 == idx2);

		KeyFrame	blendKey1;
		KeyFrame	blendKey2;

		//set blend key temporarily
		SubAnim_SetBone(pSA1, &blendKey1, idx1);
		SubAnim_SetBone(pSA2, &blendKey2, idx2);

		//animate to the blend keys
		SubAnim_Animate(pSA1, anTime1, true);
		SubAnim_Animate(pSA2, anTime2, true);

		//blend to original bone reference
		KeyFrame_Lerp(&blendKey1, &blendKey2, percentage, pBone1);

		//put the bones back
		SubAnim_SetBone(pSA1, pBone1, idx1);
		SubAnim_SetBone(pSA2, pBone2, idx2);
	}
}

const UT_string	*Anim_GetName(const Anim *pAnim)
{
	return	pAnim->szName;
}

void	Anim_SetNameccp(Anim *pAnim, const char *szNew)
{
	utstring_clear(pAnim->szName);
	utstring_printf(pAnim->szName, "%s", szNew);
}

void	Anim_Destroy(Anim *pAnim)
{
	utstring_done(pAnim->szName);

	for(int i=0;i < pAnim->mNumSubAnims;i++)
	{
		SubAnim_Destroy(pAnim->mpSubAnims[i]);
	}

	free(pAnim);
}

void	Anim_ReMapBoneIndexes(Anim *pAnim, int boneMap[])
{
	for(int i=0;i < pAnim->mNumSubAnims;i++)
	{
		SubAnim_ReMapBoneIndex(pAnim->mpSubAnims[i], boneMap);
	}
}

void	Anim_SetBoneRefs(Anim *pAnim, Skeleton *pSkel)
{
	for(int i=0;i < pAnim->mNumSubAnims;i++)
	{
		SubAnim_SetBoneRef(pAnim->mpSubAnims[i], pSkel);
	}
}