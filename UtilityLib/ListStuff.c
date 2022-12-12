#include	<stdint.h>
#include	<stdbool.h>
#include	"utlist.h"
#include	"utstring.h"


//an attempt at making more C# style lists using utlist behind the scenes


typedef struct	StringList_t
{
	UT_string	*mpSZ;

	struct StringList_t	*next;
}	StringList;


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
		utstring_done(pElement->mpSZ);
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

const StringList	*SZList_Next(const StringList *pIter)
{
	return	pIter->next;
}

const char	*SZList_IteratorVal(const StringList *pList)
{
	return	utstring_body(pList->mpSZ);
}