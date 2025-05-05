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

	//defaults
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
		SubAnim_Animate(pAnim->mpSubAnims[i], time,
			pAnim->mbLooping, pAnim->mbPingPong);
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

		SubAnim_Blend(pSA1, pSA2, anTime1, anTime2, percentage);
	}
}


const UT_string	*Anim_GetName(const Anim *pAnim)
{
	return	pAnim->szName;
}

bool	Anim_GetLooping(const Anim *pAnim)
{
	return	pAnim->mbLooping;
}

bool	Anim_GetPingPong(const Anim *pAnim)
{
	return	pAnim->mbPingPong;
}


void	Anim_SetNameccp(Anim *pAnim, const char *szNew)
{
	utstring_clear(pAnim->szName);
	utstring_printf(pAnim->szName, "%s", szNew);
}

void	Anim_SetLooping(Anim *pAnim, bool bLooping)
{
	pAnim->mbLooping	=bLooping;
}

void	Anim_SetPingPong(Anim *pAnim, bool bPingPong)
{
	pAnim->mbPingPong	=bPingPong;
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