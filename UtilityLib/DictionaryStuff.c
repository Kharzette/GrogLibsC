#include	<stdint.h>
#include	<stdbool.h>
#include	"uthash.h"
#include	"utstring.h"
#include	"DictionaryStuff.h"


//an attempt at C#ish dictionaries with uthash as the base

//a string keyed key/value pair thing
typedef struct	DictSZ_t
{
	UT_string		*mpKey;
	void			*pValue;	//generic value
	UT_hash_handle	hh;
}	DictSZ;


void	DictSZ_Add(DictSZ **ppHead, const UT_string *pKey, void *pValue)
{
	DictSZ	*pAdd	=malloc(sizeof(DictSZ));

	utstring_new(pAdd->mpKey);
	utstring_concat(pAdd->mpKey, pKey);

	pAdd->pValue	=pValue;

	HASH_ADD_KEYPTR(hh, *ppHead, utstring_body(pKey), utstring_len(pKey), pAdd);
}

//char * ver
void	DictSZ_Addccp(DictSZ **ppHead, const char *pKey, void *pValue)
{
	DictSZ	*pAdd	=malloc(sizeof(DictSZ));

	utstring_new(pAdd->mpKey);
	utstring_printf(pAdd->mpKey, "%s", pKey);

	pAdd->pValue	=pValue;

	HASH_ADD_KEYPTR(hh, *ppHead, utstring_body(pAdd->mpKey), utstring_len(pAdd->mpKey), pAdd);
}


void	DictSZ_Remove(DictSZ **ppHead, const UT_string *pKey)
{
	DictSZ	*pHash;

	HASH_FIND(hh, *ppHead, pKey, utstring_len(pKey), pHash);

	if(pHash == NULL)
	{
		return;	//not found
	}

	//free data
	utstring_done(pHash->mpKey);
	free(pHash->pValue);

	HASH_DELETE(hh, *ppHead, pHash);
}


void *DictSZ_GetValue(const DictSZ *pHead, const UT_string *pKey)
{
	void	*pRet	=NULL;
	DictSZ	*pHash;

	HASH_FIND(hh, pHead, pKey, utstring_len(pKey), pHash);
	if(pHash == NULL)
	{
		return	NULL;
	}
	return	pHash->pValue;
}


void	DictSZ_Clear(DictSZ **ppHead)
{
	DictSZ	*pIt, *pTmp;

	HASH_ITER(hh, *ppHead, pIt, pTmp)
	{
		//nuke data
		utstring_done(pIt->mpKey);
		free(pIt->pValue);

		HASH_DEL(*ppHead, pIt);
	}
}


void	DictSZ_ClearNoFree(DictSZ **ppHead)
{
	DictSZ	*pIt, *pTmp;

	HASH_ITER(hh, *ppHead, pIt, pTmp)
	{
		//nuke data, user will nuke value
		utstring_done(pIt->mpKey);

		HASH_DEL(*ppHead, pIt);
	}
}


void	DictSZ_ClearCB(DictSZ **ppHead, DictSZ_ValueNukeCB pCB)
{
	DictSZ	*pIt, *pTmp;

	HASH_ITER(hh, *ppHead, pIt, pTmp)
	{
		//call callback so the user can clean up this void *
		pCB(pIt->pValue);

		//nuke key
		utstring_done(pIt->mpKey);

		HASH_DEL(*ppHead, pIt);
	}
}


void	DictSZ_New(DictSZ **ppHead)
{
	*ppHead	=NULL;
}


bool	DictSZ_ContainsKey(const DictSZ *pHead, UT_string *pKey)
{
	return	(DictSZ_GetValue(pHead, pKey) != NULL);
}


int	DictSZ_Count(const DictSZ *pHead)
{
	return	HASH_COUNT(pHead);
}


void	DictSZ_ForEach(const DictSZ *pHead, DictSZ_ForEachCB pCB, void *pContext)
{
	const DictSZ	*pCur, *pTmp;

	HASH_ITER(hh, pHead, pCur, pTmp)
	{
		pCB(pCur->mpKey, pCur->pValue, pContext);
	}
}