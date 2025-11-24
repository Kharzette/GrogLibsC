#include	<stdint.h>
#include	<stdbool.h>
#include	<cglm/call.h>
#include	"utlist.h"
#include	"utstring.h"
#include	"MiscStuff.h"


//an attempt at making more C# style lists using utlist behind the scenes


typedef struct	StringList_t
{
	UT_string	*mpSZ;

	struct StringList_t	*next;
}	StringList;

typedef struct	Vec4List_t
{
	vec4	mVec;

	struct Vec4List_t	*next;
}	Vec4List;


StringList	*SZList_New(void)
{
	return	NULL;
}


void	SZList_Add(StringList **ppSL, const char *pSZ)
{
	StringList	*pNew	=malloc(sizeof(StringList));

	//copy into structure
	utstring_new(pNew->mpSZ);
	utstring_printf(pNew->mpSZ, "%s", pSZ);

	pNew->next	=NULL;

	LL_APPEND(*ppSL, pNew);
}

void	SZList_AddUT(StringList **ppSL, const UT_string *pSZ)
{
	StringList	*pNew	=malloc(sizeof(StringList));

	//copy into structure
	utstring_new(pNew->mpSZ);
	utstring_concat(pNew->mpSZ, pSZ);

	pNew->next	=NULL;

	LL_APPEND(*ppSL, pNew);
}

void	SZList_AddUTNoCopy(StringList **ppSL, UT_string *pSZ)
{
	StringList	*pNew	=malloc(sizeof(StringList));

	//directly use the supplied string
	pNew->mpSZ	=pSZ;
	pNew->next	=NULL;

	LL_APPEND(*ppSL, pNew);
}


void	SZList_Clear(StringList **ppSL)
{
	StringList	*pTmp, *pElement	=NULL;

	LL_FOREACH_SAFE(*ppSL, pElement, pTmp)
	{
		LL_DELETE(*ppSL, pElement);

		//free data
		utstring_free(pElement->mpSZ);
		free(pElement);
	}
}


void	SZList_Remove(StringList **ppSL, const char *pSZ)
{
	StringList	*pTmp, *pElement	=NULL;

	LL_FOREACH_SAFE(*ppSL, pElement, pTmp)
	{
		int	cmp	=strcmp(utstring_body(pElement->mpSZ), pSZ);

		if(cmp == 0)
		{
			//remove from list
			LL_DELETE(*ppSL, pElement);

			//free data
			utstring_done(pElement->mpSZ);
			free(pElement);

			return;
		}
	}
}

void	SZList_RemoveUT(StringList **ppSL, const UT_string *pSZ)
{
	StringList	*pTmp, *pElement	=NULL;

	LL_FOREACH_SAFE(*ppSL, pElement, pTmp)
	{
		int	cmp	=strcmp(utstring_body(pElement->mpSZ), utstring_body(pSZ));

		if(cmp == 0)
		{
			//remove from list
			LL_DELETE(*ppSL, pElement);

			//free data
			utstring_done(pElement->mpSZ);
			free(pElement);

			return;
		}
	}
}


bool	SZList_Contains(const StringList *pSL, const char *pSZ)
{
	const StringList	*pElement	=NULL;

	LL_FOREACH(pSL, pElement)
	{
		int	cmp	=strcmp(utstring_body(pElement->mpSZ), pSZ);

		if(cmp == 0)
		{
			return	true;
		}
	}
	return	false;
}


int	SZList_Count(const StringList *pSL)
{
	int	cnt	=0;

	const StringList	*pElement	=NULL;

	LL_COUNT(pSL, pElement, cnt);

	return	cnt;
}


const StringList	*SZList_Iterate(const StringList *pList)
{
	const StringList	*pRet	=pList;

	return	pRet;
}

const StringList	*SZList_IteratorNext(const StringList *pIter)
{
	return	pIter->next;
}

const char	*SZList_IteratorVal(const StringList *pIter)
{
	return	utstring_body(pIter->mpSZ);
}

const UT_string	*SZList_IteratorValUT(const StringList *pIter)
{
	return	pIter->mpSZ;
}


Vec4List	*V4List_New(void)
{
	return	NULL;
}


void	V4List_Add(Vec4List **ppVL, const vec4 vec)
{
	Vec4List	*pNew	=malloc(sizeof(Vec4List));

	//copy into structure
	glm_vec4_copy(vec, pNew->mVec);

	pNew->next	=NULL;

	LL_APPEND(*ppVL, pNew);
}


void	V4List_Clear(Vec4List **ppVL)
{
	Vec4List	*pTmp, *pElement	=NULL;

	LL_FOREACH_SAFE(*ppVL, pElement, pTmp)
	{
		LL_DELETE(*ppVL, pElement);

		//free data
		free(pElement);
	}
}


void	V4List_Remove(Vec4List **ppVL, const vec4 vec)
{
	Vec4List	*pTmp, *pElement	=NULL;

	LL_FOREACH_SAFE(*ppVL, pElement, pTmp)
	{
		if(Misc_CompareVec4s(pElement->mVec, vec))
		{
			//remove from list
			LL_DELETE(*ppVL, pElement);

			//free data
			free(pElement);

			return;
		}
	}
}


bool	V4List_Contains(const Vec4List *pVL, const vec4 vec)
{
	const Vec4List	*pElement	=NULL;

	LL_FOREACH(pVL, pElement)
	{
		if(Misc_CompareVec4s(pElement->mVec, vec))
		{
			return	true;
		}
	}
	return	false;
}


int	V4List_Count(const Vec4List *pVL)
{
	int	cnt	=0;

	const Vec4List	*pElement	=NULL;

	LL_COUNT(pVL, pElement, cnt);

	return	cnt;
}


const Vec4List	*V4List_Iterate(const Vec4List *pList)
{
	const Vec4List	*pRet	=pList;

	return	pRet;
}

const Vec4List	*V4List_IteratorNext(const Vec4List *pIter)
{
	return	pIter->next;
}

const float *V4List_IteratorVal(const Vec4List *pIter)
{
	return	pIter->mVec;
}