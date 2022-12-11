#include	<stdint.h>
#include	<stdio.h>
#include	<ctype.h>
#include	"uthash.h"
#include	"utstring.h"
#include	"utlist.h"
#include	"../UtilityLib/StringStuff.h"
#include	"../UtilityLib/ListStuff.h"


//internal structs
typedef struct	ShaderEntryPoints_t
{
	UT_string	*mpShaderFile;	//blort.hlsl or whateva

	StringList	*mpEntryPoints;

	UT_hash_handle	hh;
}	ShaderEntryPoints;


ShaderEntryPoints	*ReadEntryPoints(FILE *f)
{
	ShaderEntryPoints	*pRet	=NULL;

	char	szLine[256];	//line buffer

	//current shader with entries being worked on
	ShaderEntryPoints	*pCur	=malloc(sizeof(ShaderEntryPoints));
	pCur->mpEntryPoints	=SZList_New();

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

			//copy entry point minus tabs and \n and junx
			SZList_AddUTNoCopy(&pCur->mpEntryPoints, SZ_Trim(szLine));
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
				pCur->mpEntryPoints	=SZList_New();
			}

			//strip extension
			pCur->mpShaderFile	=SZ_StripExtension(szLine);
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

		const StringList	*pSLCur	=SZList_Iterate(pCur->mpEntryPoints);
		while(pSLCur != NULL)
		{
			printf("\t%s\n", SZList_IteratorVal(pSLCur));
		}
	}

	//delete stuff
	HASH_ITER(hh, pVSEP, pCur, pTmp)
	{
		SZList_Clear(&pCur->mpEntryPoints);
		HASH_DEL(pVSEP, pCur);
		utstring_free(pCur->mpShaderFile);
		free(pCur);
	}
	
	//delete stuff
	HASH_ITER(hh, pPSEP, pCur, pTmp)
	{
		SZList_Clear(&pCur->mpEntryPoints);
		HASH_DEL(pPSEP, pCur);
		utstring_free(pCur->mpShaderFile);
		free(pCur);
	}
}
