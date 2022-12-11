#include	<stdint.h>
#include	<stdbool.h>
#include	<utstring.h>


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

bool	SZ_StartsWithCCUT(const char *pSZ, UT_string *pszThing)
{
	if(pSZ == NULL || pszThing == NULL)
	{
		return	false;
	}
	return	SZ_StartsWith(pSZ, utstring_body(pszThing));
}

bool	SZ_StartsWithUTUT(UT_string *pSZ, UT_string *pszThing)
{
	if(pSZ == NULL || pszThing == NULL)
	{
		return	false;
	}
	return	SZ_StartsWith(utstring_body(pSZ), utstring_body(pszThing));
}

bool	SZ_StartsWithUTCC(UT_string *pSZ, const char *pszThing)
{
	if(pSZ == NULL || pszThing == NULL)
	{
		return	false;
	}
	return	SZ_StartsWith(utstring_body(pSZ), pszThing);
}


//trim spaces, tabs, and junx from start and end
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

//trim spaces, tabs, and junx from start and end
UT_string	*SZ_TrimUT(UT_string *pSZ)
{
	char	*pBody	=utstring_body(pSZ);

	return	SZ_Trim(pBody);
}


//find the index of cThing in pSZ
int	SZ_IndexOf(const char *pSZ, char cThing)
{
	if(pSZ == NULL)
	{
		return	-1;
	}

	char	*pSpot	=strchr(pSZ, cThing);
	if(pSpot == NULL)
	{
		return	NULL;
	}
	return	(pSpot - pSZ);
}

int	SZ_IndexOfUT(UT_string *pSZ, char cThing)
{
	return	SZ_IndexOf(utstring_body(pSZ), cThing);
}


//find the last index of cThing in pSZ
int	SZ_LastIndexOf(const char *pSZ, char cThing)
{
	if(pSZ == NULL)
	{
		return	-1;
	}

	char	*pSpot	=strrchr(pSZ, cThing);
	if(pSpot == NULL)
	{
		return	NULL;
	}
	return	(pSpot - pSZ);
}

int	SZ_LastIndexOfUT(UT_string *pSZ, char cThing)
{
	return	SZ_LastIndexOf(utstring_body(pSZ), cThing);
}