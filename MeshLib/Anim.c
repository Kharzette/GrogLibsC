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

UT_string	*Anim_GetName(Anim *pAnim)
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