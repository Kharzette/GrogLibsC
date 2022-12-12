#include	<stdint.h>
#include	<stdbool.h>
#include	"uthash.h"
#include	"utstring.h"


//an attempt at C#ish dictionaries with uthash as the base

//a string keyed key/value pair thing
typedef struct	DictSZ_t	DictSZ;

//foreach callback, calls this for every element
typedef void	(*DictSZ_ForEachCB)(const UT_string *pKey, const void *pValue);

//add a value, note that this is user allocated, but Dict frees it later
extern void	DictSZ_Add(DictSZ *pHead, const UT_string *pKey, const void *pValue);
extern void	DictSZ_Remove(DictSZ **ppHead, const UT_string *pKey);
extern void *DictSZ_GetValue(const DictSZ *pHead, const UT_string *pKey);
extern void	DictSZ_Clear(DictSZ **ppHead);
extern void	DictSZ_New(DictSZ **ppHead);
extern bool	DictSZ_ContainsKey(const DictSZ *pHead, UT_string *pKey);
extern int	DictSZ_Count(const DictSZ *pHead);
extern void	DictSZ_ForEach(const DictSZ *pHead, DictSZ_ForEachCB pCB);

