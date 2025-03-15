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

	HASH_ADD_KEYPTR(hh, *ppHead, utstring_body(pAdd->mpKey), utstring_len(pAdd->mpKey), pAdd);
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

//does not free the value
void	DictSZ_Remove(DictSZ **ppHead, const UT_string *pKey)
{
	DictSZ	*pHash;

	HASH_FIND_STR(*ppHead, utstring_body(pKey), pHash);

	if(pHash == NULL)
	{
		return;	//not found
	}

	//free data
	utstring_done(pHash->mpKey);

	HASH_DELETE(hh, *ppHead, pHash);
}

//does not free the value
void	DictSZ_Removeccp(DictSZ **ppHead, const char *pKey)
{
	DictSZ	*pHash;

	HASH_FIND_STR(*ppHead, pKey, pHash);

	if(pHash == NULL)
	{
		return;	//not found
	}

	//free data
	utstring_done(pHash->mpKey);

	HASH_DELETE(hh, *ppHead, pHash);
}


void *DictSZ_GetValue(const DictSZ *pHead, const UT_string *pKey)
{
	DictSZ	*pHash;

	HASH_FIND_STR(pHead, utstring_body(pKey), pHash);
	if(pHash == NULL)
	{
		return	NULL;
	}
	return	pHash->pValue;
}

void *DictSZ_GetValueccp(const DictSZ *pHead, const char *pKey)
{
	DictSZ	*pHash;

	//HASH_FIND_STR(pHead, pKey, pHash);
	unsigned _uthash_hfstr_keylen = (unsigned)strlen(pKey);
	//HASH_FIND(hh, head, findstr, _uthash_hfstr_keylen, out);
	(pHash) = NULL;
	if (pHead) {
		unsigned _hf_hashv;
		HASH_VALUE(pKey, _uthash_hfstr_keylen, _hf_hashv);
		//HASH_FIND_BYHASHVALUE(hh, pHead, pKey, _uthash_hfstr_keylen, _hf_hashv, pHash);
		(pHash) = NULL;
		if (pHead) {
			unsigned _hf_bkt;
			HASH_TO_BKT(_hf_hashv, (pHead)->hh.tbl->num_buckets, _hf_bkt);
			if (HASH_BLOOM_TEST((pHead)->hh.tbl, _hf_hashv) != 0) {
				HASH_FIND_IN_BKT((pHead)->hh.tbl, hh, (pHead)->hh.tbl->buckets[ _hf_bkt ], pKey, _uthash_hfstr_keylen, _hf_hashv, pHash);
			}
		}
	}

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


//this used to just check for null from a get value,
//but one of the things I do with this sometimes is
//store an int instead of a pointer, and if there's
//a zero, it is the same as null so contains would fail
bool	DictSZ_ContainsKey(const DictSZ *pHead, const UT_string *pKey)
{
	return	DictSZ_ContainsKeyccp(pHead, utstring_body(pKey));
}


bool	DictSZ_ContainsKeyccp(const DictSZ *pHead, const char *pKey)
{
	DictSZ	*pHash;
	
	HASH_FIND_STR(pHead, pKey, pHash);
	if(pHash == NULL)
	{
		return	false;
	}
	return	true;
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