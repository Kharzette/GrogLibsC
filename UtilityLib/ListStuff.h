#pragma once
#include	<stdint.h>
#include	<stdbool.h>
#include	"utstring.h"


//an attempt at making more C# style lists using utlist behind the scenes
typedef struct	StringList_t	StringList;

//return a new stringlist
extern StringList *SZList_New(void);

//standard listy stuff
extern void	SZList_Add(StringList **ppSL, const char *pSZ);
extern void	SZList_AddUT(StringList **ppSL, const UT_string *pSZ);
extern void	SZList_AddUTNoCopy(StringList **ppSL, UT_string *pSZ);
extern void	SZList_Clear(StringList **ppSL);
extern void	SZList_Remove(StringList **ppSL, const char *pSZ);
extern void	SZList_RemoveUT(StringList **ppSL, const UT_string *pSZ);
extern bool	SZList_Contains(const StringList *pSL, const char *pSZ);
extern int	SZList_Count(const StringList *pSL);

//const iteration, return null when done?
extern const StringList *SZList_Iterate(const StringList *pList);
extern const StringList *SZList_IteratorNext(const StringList *pIter);

//get value
extern const char		*SZList_IteratorVal(const StringList *pIter);
extern const UT_string	*SZList_IteratorValUT(const StringList *pIter);