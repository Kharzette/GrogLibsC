#pragma once
#include	<stdint.h>
#include	<stdbool.h>
#include	<utstring.h>


//does pSZ start with Thing?
extern bool	SZ_StartsWith(const char *pSZ, const char *pszThing);
extern bool	SZ_StartsWithCCUT(const char *pSZ, UT_string *pszThing);
extern bool	SZ_StartsWithUTUT(UT_string *pSZ, UT_string *pszThing);
extern bool	SZ_StartsWithUTCC(UT_string *pSZ, const char *pszThing);

//trim spaces, tabs, and junx from start and end, allocs a new copy
extern UT_string	*SZ_Trim(const char *pSZ);
extern UT_string	*SZ_TrimUT(UT_string *pSZ);

//find first / last index of character cThing in pSZ, -1 if not found
extern int	SZ_IndexOf(const char *pSZ, char cThing);
extern int	SZ_IndexOfUT(UT_string *pSZ, char cThing);
extern int	SZ_LastIndexOf(const char *pSZ, char cThing);
extern int	SZ_LastIndexOfUT(UT_string *pSZ, char cThing);

//return the extension of a filename or NULL if none
extern UT_string	*SZ_GetExtension(const char *pSZ);
extern UT_string	*SZ_GetExtensionUT(UT_string *pSZ);

//remove the extension from the filename if there is one
//Returns a new string or NULL if the input was NULL
extern UT_string	*SZ_StripExtension(const char *pSZ);
extern UT_string	*SZ_StripExtensionUT(UT_string *pSZ);
