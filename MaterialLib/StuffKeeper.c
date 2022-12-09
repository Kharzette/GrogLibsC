#include	<stdint.h>
#include	<stdio.h>
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


//stuff data
static ShaderEntryPoints	*spVSEntryPoints	=NULL;


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

			//copy entry point minus the tab
			utstring_printf(pEntry->mpSZ, "%s", szLine + 1);

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

	ShaderEntryPoints	*pSEP	=ReadEntryPoints(f);

	ShaderEntryPoints	*pCur, *pTmp;
	HASH_ITER(hh, pSEP, pCur, pTmp)
	{
		printf("Shader name: %s\n", utstring_body(pCur->mpShaderFile));
	}
}
