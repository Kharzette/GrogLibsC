#include	<stdint.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<assert.h>
#include	"utstring.h"
#include	"../UtilityLib/StringStuff.h"
#include	"../UtilityLib/ListStuff.h"
#include	"../UtilityLib/DictionaryStuff.h"


DictSZ	*ReadEntryPoints(FILE *f)
{
	DictSZ	*pRet;

	DictSZ_New(&pRet);

	char	szLine[256];	//line buffer

	//current shader data with entries being worked on
	UT_string	*pCurShader;
	StringList	*pEntryPoints;

	utstring_new(pCurShader);
	pEntryPoints	=SZList_New();

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
			assert(utstring_len(pCurShader) > 0);

			//copy entry point minus tabs and \n and junx
			SZList_AddUTNoCopy(&pEntryPoints, SZ_Trim(szLine));
		}
		else
		{
			//must be a new shader name

			//is there a previous entry in cur?
			if(utstring_len(pCurShader) > 0)
			{
				//add data to pRet
				DictSZ_Add(pRet, pCurShader, pEntryPoints);

				//get ready for new data
				pEntryPoints	=SZList_New();
			}

			//strip extension
			pCurShader	=SZ_StripExtension(szLine);
		}

		if(feof(f))
		{
			break;
		}
	}

	//add last data to pRet
	DictSZ_Add(pRet, pCurShader, pEntryPoints);

	return	pRet;
}


//callback for the dictionary foreach below
void	PrintEntryPointsCB(const UT_string *pKey, const void *pValue)
{
	const StringList	*pList	=pValue;
	const StringList	*pCur	=SZList_Iterate(pList);

	while(pCur != NULL)
	{
		printf("\t%s\n", SZList_IteratorVal(pCur));

		pCur	=SZList_Next(pCur);
	}
}


int main(void)
{
	printf("StuffKeeper testing...\n");

	FILE	*f	=fopen("Shaders/VSEntryPoints.txt", "r");

	DictSZ	*pVSEP	=ReadEntryPoints(f);

	fclose(f);

	f	=fopen("Shaders/PSEntryPoints.txt", "r");

	DictSZ	*pPSEP	=ReadEntryPoints(f);

	fclose(f);

	DictSZ	*pCur, *pTmp;

	DictSZ_ForEach(pPSEP, PrintEntryPointsCB);

	//delete stuff
	DictSZ_Clear(&pVSEP);
	DictSZ_Clear(&pPSEP);
}
