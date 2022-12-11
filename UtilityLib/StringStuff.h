#pragma once
#include	<stdint.h>
#include	<stdbool.h>
#include	<utstring.h>


//does pSZ start with Thing?
extern bool	SZ_StartsWith(const char *pSZ, const char *pszThing);
extern bool	SZ_StartsWithCCUT(const char *pSZ, const UT_string *pszThing);
extern bool	SZ_StartsWithUTUT(const UT_string *pSZ, const UT_string *pszThing);
extern bool	SZ_StartsWithUTCC(const UT_string *pSZ, const char *pszThing);

//trim spaces, tabs, and junx from start and end, allocs a new copy
extern UT_string	*SZ_Trim(const char *pSZ);
extern UT_string	*SZ_TrimUT(const UT_string *pSZ);

//find first / last index of character cThing in pSZ, -1 if not found
extern int	SZ_IndexOf(const char *pSZ, char cThing);
extern int	SZ_IndexOfUT(const UT_string *pSZ, char cThing);
extern int	SZ_LastIndexOf(const char *pSZ, char cThing);
extern int	SZ_LastIndexOfUT(const UT_string *pSZ, char cThing);

//return the extension of a filename or NULL if none
extern UT_string	*SZ_GetExtension(const char *pSZ);
extern UT_string	*SZ_GetExtensionUT(const UT_string *pSZ);

//remove the extension from the filename if there is one
//Returns a new string or NULL if the input was NULL
extern UT_string	*SZ_StripExtension(const char *pSZ);
extern UT_string	*SZ_StripExtensionUT(const UT_string *pSZ);

//convert a path's slashes from \ to / or the other way around
//returns a new utstring or NULL if pSZ is NULL
extern UT_string	*SZ_ConvertPathSlashes(const char *pSZ);
extern UT_string	*SZ_ConvertPathSlashesUT(const UT_string *pSZ);
extern UT_string	*SZ_ConvertPathBackSlashes(const char *pSZ);
extern UT_string	*SZ_ConvertPathBackSlashesUT(const UT_string *pSZ);
