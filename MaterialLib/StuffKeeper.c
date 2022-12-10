#include	<stdint.h>
#include	<stdio.h>
#include	<ctype.h>
#include	"uthash.h"
#include	"utstring.h"
#include	"utlist.h"


//internal structs
typedef struct	StringList_t
{
	UT_string	*mpSZ;

	struct StringList_t	*next;
}	StringList;

typedef struct	ShaderEntryPoints_t
{
	UT_string	*mpShaderFile;	//blort.hlsl or whateva

	StringList	*mpEntryPoints;

	UT_hash_handle	hh;
}	ShaderEntryPoints;


//trim spaces, tabs, and junx from start and end
UT_string	*StringTrim(const char *pSZ)
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
UT_string	*StringTrimUT(UT_string *pSZ)
{
	char	*pBody	=utstring_body(pSZ);

	return	StringTrim(pBody);
}


ShaderEntryPoints	*ReadEntryPoints(FILE *f)
{
	ShaderEntryPoints	*pRet	=NULL;

	char	szLine[256];	//line buffer

	//current shader with entries being worked on
	ShaderEntryPoints	*pCur	=malloc(sizeof(ShaderEntryPoints));
	utstring_new(pCur->mpShaderFile);
	pCur->mpEntryPoints	=NULL;

	for(;;)
	{
		//TODO test super long line or otherwise malformed file
		if(fgets(szLine, 256, f) == NULL)
		{
			break;
		}

		if(szLine[0] == '/' && szLine[1] == '/')
		{
			continue;	//comment
		}

		//python style!
		if(szLine[0] == '\t')
		{
			//should have a shader name at this point
			assert(utstring_len(pCur->mpShaderFile) > 0);

			StringList	*pEntry	=malloc(sizeof(StringList));

			utstring_new(pEntry->mpSZ);

			//copy entry point minus tabs and \n and junx
			pEntry->mpSZ	=StringTrim(szLine);

			//add to list
			LL_APPEND(pCur->mpEntryPoints, pEntry);
		}
		else
		{
			//must be a new shader name

			//is there a previous entry in cur?
			if(utstring_len(pCur->mpShaderFile) > 0)
			{
				//add data to pRet
				HASH_ADD_KEYPTR(hh, pRet, utstring_body(pCur->mpShaderFile),
					utstring_len(pCur->mpShaderFile), pCur);

				//get cur ready for new data
				pCur	=malloc(sizeof(ShaderEntryPoints));
				utstring_new(pCur->mpShaderFile);
				pCur->mpEntryPoints	=NULL;
			}

			//strip extension
			char	*pDotPos	=strrchr(szLine, '.');
			if(pDotPos != NULL)
			{
				*pDotPos	=0;
			}

			utstring_printf(pCur->mpShaderFile, "%s", szLine);
		}

		if(feof(f))
		{
			break;
		}
	}

	//add last data to pRet
	HASH_ADD_KEYPTR(hh, pRet, utstring_body(pCur->mpShaderFile),
		utstring_len(pCur->mpShaderFile), pCur);

	return	pRet;
}


int main(void)
{
	printf("StuffKeeper testing...\n");

	FILE	*f	=fopen("Shaders/VSEntryPoints.txt", "r");

	ShaderEntryPoints	*pVSEP	=ReadEntryPoints(f);

	fclose(f);

	f	=fopen("Shaders/PSEntryPoints.txt", "r");

	ShaderEntryPoints	*pPSEP	=ReadEntryPoints(f);

	fclose(f);

	ShaderEntryPoints	*pCur, *pTmp;
	HASH_ITER(hh, pPSEP, pCur, pTmp)
	{
		printf("Shader name: %s\n", utstring_body(pCur->mpShaderFile));

		StringList	*pSLCur	=NULL;
		LL_FOREACH(pCur->mpEntryPoints, pSLCur)
		{
			printf("\t%s\n", utstring_body(pSLCur->mpSZ));
		}
	}

	//delete stuff
	HASH_ITER(hh, pVSEP, pCur, pTmp)
	{
		StringList	*pSLCur, *pSLTmp;
		LL_FOREACH_SAFE(pCur->mpEntryPoints, pSLCur, pSLTmp)
		{
			utstring_free(pSLCur->mpSZ);
			free(pSLCur);
		}
		HASH_DEL(pVSEP, pCur);
		utstring_free(pCur->mpShaderFile);
		free(pCur);
	}
	
	//delete stuff
	HASH_ITER(hh, pPSEP, pCur, pTmp)
	{
		StringList	*pSLCur, *pSLTmp;
		LL_FOREACH_SAFE(pCur->mpEntryPoints, pSLCur, pSLTmp)
		{
			utstring_free(pSLCur->mpSZ);
			free(pSLCur);
		}
		HASH_DEL(pPSEP, pCur);
		utstring_free(pCur->mpShaderFile);
		free(pCur);
	}
}
