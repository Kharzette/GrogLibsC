#pragma once
#include	<stdint.h>
#include	<stdbool.h>
#include	<utstring.h>


//does pSZ start with Thing?
extern bool	SZ_StartsWith(const char *pSZ, const char *pszThing);
extern bool	SZ_StartsWithCCUT(const char *pSZ, UT_string *pszThing);
extern bool	SZ_StartsWithUTUT(UT_string *pSZ, UT_string *pszThing);
extern bool	SZ_StartsWithUTCC(UT_string *pSZ, const char *pszThing);

//trim spaces, tabs, and junx from start and end
extern UT_string	*SZ_Trim(const char *pSZ);
extern UT_string	*SZ_TrimUT(UT_string *pSZ);

//find first / last index of character cThing in pSZ
extern int	SZ_IndexOf(const char *pSZ, char cThing);
extern int	SZ_IndexOfUT(UT_string *pSZ, char cThing);
extern int	SZ_LastIndexOf(const char *pSZ, char cThing);
extern int	SZ_LastIndexOfUT(UT_string *pSZ, char cThing);
