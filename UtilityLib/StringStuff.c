#include	"StringStuff.h"
#include	<ctype.h>


//an attempt to do C# style string manip stuffs
//want StartsWith, trim, IndexOf, GetExtension, StripExtension
//ConvertPathSlashes, ConvertPathBackSlashes, StringToVec234,
//StringToMatrix, FloatToString, FloatArrayToString, VecToString,

//does pSZ contain pszThing?
bool	SZ_ContainsUTCC(const UT_string *pSZ, const char *pszThing)
{
	if(pSZ == NULL || pszThing == NULL)
	{
		return	false;
	}

	int	len	=strlen(pszThing);

	int	res	=utstring_find(pSZ, 0, pszThing, len);

	return	(res != -1);
}

bool	SZ_EndsWithUT(const UT_string *pSZ, char c)
{
	int	len	=utstring_len(pSZ);

	int	lastIdx	=SZ_LastIndexOfUT(pSZ, c);

	if(lastIdx == -1)
	{
		return	false;
	}
	if(lastIdx < (len - 1))
	{
		return	false;
	}
	return	true;
}

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

	//set internal length var
	pRet->i	=strlen(utstring_body(pRet));

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


//return the substring beginning at startPos or NULL
UT_string	*SZ_SubStringStart(const char *pSZ, int startPos)
{
	if(pSZ == NULL)
	{
		return	NULL;
	}

	int	len	=strlen(pSZ);
	if(startPos >= len)
	{
		return	NULL;
	}

	UT_string	*pRet;
	utstring_new(pRet);

	utstring_printf(pRet, "%s", pSZ + startPos);

	return	pRet;
}

UT_string	*SZ_SubStringStartEnd(const char *pSZ, int startPos, int endPos)
{
	if(pSZ == NULL)
	{
		return	NULL;
	}

	int	len		=strlen(pSZ);
	int	newLen	=endPos - startPos;
	if(startPos >= len || endPos >= len || newLen >= len)
	{
		return	NULL;
	}


	UT_string	*pRet;
	utstring_new(pRet);
	utstring_reserve(pRet, newLen + 1);

	memcpy(utstring_body(pRet), pSZ + startPos, newLen);

	utstring_body(pRet)[newLen]	=0;	//null terminate
	
	//set internal length var
	pRet->i	=strlen(utstring_body(pRet));

	return	pRet;
}

UT_string	*SZ_SubStringUTStart(const UT_string *pSZ, int startPos)
{
	return	SZ_SubStringStart(utstring_body(pSZ), startPos);
}

UT_string	*SZ_SubStringUTStartEnd(const UT_string *pSZ, int startPos, int endPos)
{
	return	SZ_SubStringStartEnd(utstring_body(pSZ), startPos, endPos);
}

bool	SZ_ReplaceUTCCCC(UT_string *pSZ, const char *pszTarget, const char *pszReplace)
{
	if(pSZ == NULL || pszTarget == NULL || pszReplace == NULL)
	{
		return	false;
	}

	int	lenTarg	=strlen(pszTarget);

	int	res	=utstring_find(pSZ, 0, pszTarget, lenTarg);
	if(res == -1)
	{
		return	false;
	}

	//lazy
	UT_string	*pBeg	=SZ_SubStringUTStartEnd(pSZ, 0, res);
	UT_string	*pAfter	=SZ_SubStringUTStart(pSZ, res + lenTarg);

	utstring_clear(pSZ);

	if(pAfter == NULL)
	{
		utstring_printf(pSZ, "%s%s",
			utstring_body(pBeg),
			pszReplace);
	}
	else
	{
		utstring_printf(pSZ, "%s%s%s",
			utstring_body(pBeg),
			pszReplace,
			utstring_body(pAfter));
	}
	
	utstring_free(pBeg);
	if(pAfter != NULL)
	{
		utstring_free(pAfter);
	}

	return	true;
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

UT_string	*SZ_GetExtensionUT(const UT_string *pSZ)
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

	//set internal length var
	pRet->i	=strlen(utstring_body(pRet));

	return	pRet;
}

UT_string	*SZ_StripExtensionUT(const UT_string *pSZ)
{
	return	SZ_StripExtension(utstring_body(pSZ));
}

//return true if pSZ has the extension pExt
bool	SZ_IsExtension(const char *pSZ, const char *pExt)
{
	if(pSZ == NULL || pExt == NULL)
	{
		return	false;
	}

	int	dotPos	=SZ_LastIndexOf(pSZ, '.');
	if(dotPos == -1)
	{
		return	false;
	}

	//ensure it isn't a blank extension
	if(pSZ[dotPos + 1] == 0)
	{
		return	false;
	}

	//location to start compare
	const char	*pCompareSZPos	=pSZ + dotPos + 1;
	const char	*pCompareExtPos	=NULL;

	//does input extension have a dot?
	dotPos	=SZ_LastIndexOf(pExt, '.');
	if(dotPos == -1)
	{
		//no dot, this is probably the usual input
		pCompareExtPos	=pExt;
	}
	else
	{
		//there is a dot, should be position 0
		if(dotPos != 0)
		{
			//should probably consider this invalid input?
			return	false;
		}
		pCompareExtPos	=pExt + 1;
	}

	return	(strcmp(pCompareSZPos, pCompareExtPos) == 0);
}


//remove the filename from the path if there is one
//Returns a new string or NULL if the input was NULL
UT_string	*SZ_StripFileName(const char *pSZ)
{
	if(pSZ == NULL)
	{
		return	NULL;
	}

	UT_string	*pRet;
	utstring_new(pRet);

	//copy
	utstring_printf(pRet, "%s", pSZ);

	int	slashPos	=SZ_LastIndexOf(pSZ, '/');
	if(slashPos == -1)
	{
		return	pRet;
	}

	//null terminate at .
	utstring_body(pRet)[slashPos]	=0;

	//set internal length var
	pRet->i	=strlen(utstring_body(pRet));

	return	pRet;
}

UT_string	*SZ_StripFileNameUT(const UT_string *pSZ)
{
	return	SZ_StripFileName(utstring_body(pSZ));
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

//I really hate non ascii
//caller responsible for freeing
wchar_t	*SZ_ConvertToWCHAR(const UT_string *pSZ)
{
	int	len	=utstring_len(pSZ);

	wchar_t	*pBuf	=malloc(sizeof(wchar_t) * (len + 1));
	mbstowcs(pBuf, utstring_body(pSZ), len);

	return	pBuf;
}

UT_string	*SZ_ReadString(FILE *f)
{
	uint8_t	len;
	fread(&len, 1, 1, f);

	UT_string	*pRet	=malloc(sizeof(UT_string));
	pRet->d	=calloc(len + 1, 1);

	fread(pRet->d, 1, len, f);

	pRet->i	=len;
	pRet->n	=len + 1;

	return	pRet;
}

void	SZ_WriteString(FILE *f, const UT_string *pSZ)
{
	uint8_t	len	=utstring_len(pSZ);

	fwrite(&len, sizeof(uint8_t), 1, f);

	fwrite(utstring_body(pSZ), len, 1, f);
}