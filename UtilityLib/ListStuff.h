#pragma once
#include	<stdint.h>
#include	<stdbool.h>
#include	"utstring.h"


//an attempt at making more C# style lists using utlist behind the scenes
typedef struct	StringList_t	StringList;

//return a new stringlist
extern StringList *SZList_New(void);

extern void	SZList_Add(StringList **ppSL, const char *pSZ);
extern void	SZList_AddUT(StringList **ppSL, const UT_string *pSZ);
extern void	SZList_Remove(StringList **ppSL, const char *pSZ);
extern void	SZList_RemoveUT(StringList **ppSL, const UT_string *pSZ);
extern bool	SZList_Contains(const StringList *pSL, const char *pSZ);
extern int	SZList_Count(const StringList *pSL);
