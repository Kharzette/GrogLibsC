#include	<stdint.h>
#include	<stdbool.h>
#include	<ctype.h>
#include	"utstring.h"


//an attempt to do C# style string manip stuffs
//want StartsWith, trim, IndexOf, GetExtension, StripExtension
//ConvertPathSlashes, ConvertPathBackSlashes, StringToVec234,
//StringToMatrix, FloatToString, FloatArrayToString, VecToString,


//does pSZ start with Thing?
bool	SZ_StartsWith(const char *pSZ, const char *pszThing)
{
	if(pSZ == NULL || pszThing == NULL)
	{
		return	false;
	}

	int	len	=strlen(pszThing);

	if(len <= 0)
	{
		return	false;
	}

	int	cmp	=strncmp(pSZ, pszThing, len);

	return	(cmp == 0);
}

bool	SZ_StartsWithCCUT(const char *pSZ, const UT_string *pszThing)
{
	if(pSZ == NULL || pszThing == NULL)
	{
		return	false;
	}
	return	SZ_StartsWith(pSZ, utstring_body(pszThing));
}

bool	SZ_StartsWithUTUT(const UT_string *pSZ, const UT_string *pszThing)
{
	if(pSZ == NULL || pszThing == NULL)
	{
		return	false;
	}
	return	SZ_StartsWith(utstring_body(pSZ), utstring_body(pszThing));
}

bool	SZ_StartsWithUTCC(const UT_string *pSZ, const char *pszThing)
{
	if(pSZ == NULL || pszThing == NULL)
	{
		return	false;
	}
	return	SZ_StartsWith(utstring_body(pSZ), pszThing);
}


//trim spaces, tabs, and junx from start and end, allocs a new copy
UT_string	*SZ_Trim(const char *pSZ)
{
	int	len	=strlen(pSZ);

	//trim start
	int	newStart	=0;
	for(int i=0;i < len;i++)
	{
		if(isgraph(pSZ[i]))
		{
			newStart	=i;
			break;
		}
	}

	//trim end
	int	newEnd	=len - 1;
	for(int i=len-1;i >= 0;i--)
	{
		if(isgraph(pSZ[i]))
		{
			newEnd	=i;
			break;
		}
	}

	UT_string	*pRet;
	utstring_new(pRet);

	//length of trimmed segment + space for null terminator
	int		newLen	=newEnd - newStart + 2;

	utstring_reserve(pRet, newLen);
	char	*pRetBody	=utstring_body(pRet);

	//copy trimmed contents
	memcpy(pRetBody, pSZ + newStart, newLen - 1);

	//null terminate
	pRetBody[newLen - 1]	=0;

	return	pRet;
}

UT_string	*SZ_TrimUT(const UT_string *pSZ)
{
	char	*pBody	=utstring_body(pSZ);

	return	SZ_Trim(pBody);
}


//find the index of cThing in pSZ, -1 if not found
int	SZ_IndexOf(const char *pSZ, char cThing)
{
	if(pSZ == NULL)
	{
		return	-1;
	}

	char	*pSpot	=strchr(pSZ, cThing);
	if(pSpot == NULL)
	{
		return	-1;
	}
	return	(pSpot - pSZ);
}

int	SZ_IndexOfUT(const UT_string *pSZ, char cThing)
{
	return	SZ_IndexOf(utstring_body(pSZ), cThing);
}


//find the last index of cThing in pSZ, -1 if not found
int	SZ_LastIndexOf(const char *pSZ, char cThing)
{
	if(pSZ == NULL)
	{
		return	-1;
	}

	char	*pSpot	=strrchr(pSZ, cThing);
	if(pSpot == NULL)
	{
		return	-1;
	}
	return	(pSpot - pSZ);
}

int	SZ_LastIndexOfUT(const UT_string *pSZ, char cThing)
{
	return	SZ_LastIndexOf(utstring_body(pSZ), cThing);
}


//return the extension of a filename or NULL if none
UT_string	*SZ_GetExtension(const char *pSZ)
{
	if(pSZ == NULL)
	{
		return	NULL;
	}

	int	dotPos	=SZ_LastIndexOf(pSZ, '.');
	if(dotPos == -1)
	{
		return	NULL;
	}

	UT_string	*pRet;
	utstring_new(pRet);

	utstring_printf(pRet, "%s", pSZ + dotPos);

	return	pRet;
}

UT_string	*SZ_GetExtensionUT(UT_string *pSZ)
{
	return	SZ_GetExtension(utstring_body(pSZ));
}


//remove the extension from the filename if there is one
//Returns a new string or NULL if the input was NULL
UT_string	*SZ_StripExtension(const char *pSZ)
{
	if(pSZ == NULL)
	{
		return	NULL;
	}

	UT_string	*pRet;
	utstring_new(pRet);

	//copy
	utstring_printf(pRet, "%s", pSZ);

	int	dotPos	=SZ_LastIndexOf(pSZ, '.');
	if(dotPos == -1)
	{
		return	pRet;
	}

	//null terminate at .
	utstring_body(pRet)[dotPos]	=0;

	return	pRet;
}

UT_string	*SZ_StripExtensionUT(const UT_string *pSZ)
{
	return	SZ_StripExtension(utstring_body(pSZ));
}


//convert a path's slashes from \ to /, returns a new utstring or NULL if pSZ is NULL
UT_string	*SZ_ConvertPathSlashes(const char *pSZ)
{
	if(pSZ == NULL)
	{
		return	NULL;
	}

	UT_string	*pRet;
	utstring_new(pRet);

	//copy
	utstring_printf(pRet, "%s", pSZ);

	for(;;)
	{
		int	idx	=SZ_IndexOfUT(pRet, '\\');
		if(idx == -1)
		{
			return	pRet;
		}

		//replace
		utstring_body(pRet)[idx]	='/';
	}
	return	pRet;
}

UT_string	*SZ_ConvertPathSlashesUT(const UT_string *pSZ)
{
	return	SZ_ConvertPathSlashes(utstring_body(pSZ));
}

//convert a path's slashes from / to \, returns a new utstring or NULL if pSZ is NULL
UT_string	*SZ_ConvertPathBackSlashes(const char *pSZ)
{
	if(pSZ == NULL)
	{
		return	NULL;
	}

	UT_string	*pRet;
	utstring_new(pRet);

	//copy
	utstring_printf(pRet, "%s", pSZ);

	for(;;)
	{
		int	idx	=SZ_IndexOfUT(pRet, '/');
		if(idx == -1)
		{
			return	pRet;
		}

		//replace
		utstring_body(pRet)[idx]	='\\';
	}
	return	pRet;
}

UT_string	*SZ_ConvertPathBackSlashesUT(const UT_string *pSZ)
{
	return	SZ_ConvertPathBackSlashes(utstring_body(pSZ));
}
