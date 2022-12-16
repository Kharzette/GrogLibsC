#include	<stdint.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<assert.h>
#include	<dirent.h>
#include	<sys/stat.h>
#include	<errno.h>
#include	<d3dcompiler.h>
#include	"utstring.h"
#include	"../UtilityLib/StringStuff.h"
#include	"../UtilityLib/ListStuff.h"
#include	"../UtilityLib/DictionaryStuff.h"
#include	"../UtilityLib/GraphicsDevice.h"
#include	"../UtilityLib/FileStuff.h"

//shaders easier to step through
#define	SHADER_DEBUG


//data for games
//shaders, textures, fonts, etc
typedef struct	StuffKeeper_t
{
	DictSZ	*mpVSEntryPoints;	//entry points for vertex
	DictSZ	*mpPSEntryPoints;	//entry points for pixel

}	StuffKeeper;

//shader models
typedef enum ShaderModel_t
{
	SM5, SM41, SM4, SM2
}	ShaderModel;

//shader types
typedef enum	ShaderEntryType_t
{
	None,
	Vertex		=1,
	Pixel		=2,
	Compute		=4,
	Geometry	=8,
	Hull		=16,
	Domain		=32
}	ShaderEntryType;


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
				DictSZ_Add(&pRet, pCurShader, pEntryPoints);

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
	DictSZ_Add(&pRet, pCurShader, pEntryPoints);

	return	pRet;
}


static UT_string	*VersionString(ShaderModel sm)
{
	UT_string	*pRet;
	utstring_new(pRet);

	switch(sm)
	{
		case	SM2:
			utstring_printf(pRet, "%s", "2_0");
			return	pRet;
		case	SM4:
			utstring_printf(pRet, "%s", "4_0");
			return	pRet;
		case	SM41:
			utstring_printf(pRet, "%s", "4_1");
			return	pRet;
		case	SM5:
			utstring_printf(pRet, "%s", "5_0");
			return	pRet;
		default:
			assert(false);
	}
}

static UT_string	*ProfileFromSM(ShaderModel sm, ShaderEntryType set)
{
	UT_string	*pRet;
	utstring_new(pRet);

	UT_string	*pVer	=VersionString(sm);

	switch(set)
	{
		case	Compute:
			utstring_printf(pRet, "cs_%s", utstring_body(pVer));
			break;
		case	Geometry:
			utstring_printf(pRet, "gs_%s", utstring_body(pVer));
			break;
		case	Pixel:
			utstring_printf(pRet, "ps_%s", utstring_body(pVer));
			break;
		case	Vertex:
			utstring_printf(pRet, "vs_%s", utstring_body(pVer));
			break;
		case	Domain:
			utstring_printf(pRet, "ds_%s", utstring_body(pVer));
			break;
		case	Hull:
			utstring_printf(pRet, "hs_%s", utstring_body(pVer));
			break;
		default:
			assert(false);
	}

	utstring_done(pVer);

	return	pRet;
}


static void LoadShader(const UT_string *pFilePath, const UT_string *pEntryPoint,
	ShaderModel sm, ShaderEntryType set)
{
	//annoying wide crap
	wchar_t	*pFatPath	=SZ_ConvertToWCHAR(pFilePath);

	UT_string	*pProfile	=ProfileFromSM(sm, set);

	UINT	flags	=D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS
#ifdef SHADER_DEBUG
		| D3DCOMPILE_OPTIMIZATION_LEVEL0 | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#else
		| D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	ID3DBlob	*pCode, *pErrors;

	char	szSM2[4]	="SM2\0";
	char	szSM4[4]	="SM4\0";
	char	szSM41[5]	="SM41\0";
	char	szSM5[4]	="SM5\0";
	char	szOne[2]	="1\0";

	D3D_SHADER_MACRO	macz[2];
	macz[0].Definition	=szOne;

	switch(sm)
	{
		case	SM2:
			macz[0].Name	=szSM2;
			break;
		case	SM4:
			macz[0].Name	=szSM4;
			break;
		case	SM41:
			macz[0].Name	=szSM41;
			break;
		case	SM5:
			macz[0].Name	=szSM5;
			break;
		default:
			assert(false);
	}

	HRESULT	res	=D3DCompileFromFile(pFatPath, macz, NULL, utstring_body(pEntryPoint),
		utstring_body(pProfile), flags, 0, &pCode, &pErrors);
	if(res != S_OK)
	{
		printf("Shader: %s compile failed.  Errors: %s\n",
			utstring_body(pFilePath),
			(char *)pErrors->lpVtbl->GetBufferPointer(pErrors));
	}
}


//callback for the dictionary foreach below
void	PrintEntryPointsCB(const UT_string *pKey, const void *pValue)
{
	const StringList	*pList	=pValue;
	const StringList	*pCur	=SZList_Iterate(pList);

	while(pCur != NULL)
	{
		printf("\t%s\n", SZList_IteratorVal(pCur));

		pCur	=SZList_IteratorNext(pCur);
	}
}

//callback for deleting the StringList in every entry
void	NukeSZListCB(void *pValue)
{
	StringList	*pList	=pValue;

	SZList_Clear(&pList);
}


static void	LoadShaders(StuffKeeper *pSK, ShaderModel sm)
{
	if(!FileStuff_DirExists("Shaders"))
	{
		return;
	}

	UT_string	*pFilePath;
	utstring_new(pFilePath);

	int	count	=0;
	DIR	*pDir	=opendir("Shaders");
	for(;;)
	{
		struct dirent	*pDE	=readdir(pDir);
		if(pDE == NULL)
		{
			break;
		}

		utstring_printf(pFilePath, "Shaders/%s", pDE->d_name);

		struct stat	fileStuff;
		int	res	=stat(utstring_body(pFilePath), &fileStuff);
		if(res)
		{
			FileStuff_PrintErrno(res);
			continue;
		}

		//regular file?
		if(S_ISREG(fileStuff.st_mode))
		{
			UT_string	*pExtLess	=SZ_StripExtensionUT(pFilePath);

			//check entry points
			if(DictSZ_ContainsKey(pSK->mpVSEntryPoints, pExtLess))
			{
				StringList	*pEntries	=DictSZ_GetValue(pSK->mpVSEntryPoints, pExtLess);

				ShaderEntryType	set	=Vertex;

				for(const StringList *pIter	=SZList_Iterate(pEntries);pIter != NULL;pIter=SZList_IteratorNext(pIter))
				{
					LoadShader(pFilePath, SZList_IteratorValUT(pIter), sm, set);
				}
			}

		}
	}

	closedir(pDir);
}


StuffKeeper	*StuffKeeper_Create(GraphicsDevice *pGD)
{
	StuffKeeper	*pRet	=malloc(sizeof(StuffKeeper));

	FILE	*f	=fopen("Shaders/VSEntryPoints.txt", "r");

	pRet->mpVSEntryPoints	=ReadEntryPoints(f);

	fclose(f);

	f	=fopen("Shaders/PSEntryPoints.txt", "r");

	pRet->mpPSEntryPoints	=ReadEntryPoints(f);

	fclose(f);

	D3D_FEATURE_LEVEL	deviceFL	=GraphicsDevice_GetFeatureLevel(pGD);

	ShaderModel	sm;

	switch(deviceFL)
	{
		case	D3D_FEATURE_LEVEL_9_1:
		case	D3D_FEATURE_LEVEL_9_2:
		case	D3D_FEATURE_LEVEL_9_3:
					sm	=SM2;
					break;
		case	D3D_FEATURE_LEVEL_10_0:
		case	D3D_FEATURE_LEVEL_10_1:
					sm	=SM4;
					break;
		case	D3D_FEATURE_LEVEL_11_0:
		case	D3D_FEATURE_LEVEL_11_1:
		case	D3D_FEATURE_LEVEL_12_0:
		case	D3D_FEATURE_LEVEL_12_1:
					sm	=SM5;
					break;
		default:
			assert(0);
	}

	LoadShaders(pRet, sm);

	return	pRet;
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

	//delete stuff, note that because our void * in the dictionary
	//is a complicated SZList type, additional work needs to be done
	//to clean it up beyond just a free()

	//delete stuff
	DictSZ_ClearCB(&pVSEP, NukeSZListCB);
	DictSZ_ClearCB(&pPSEP, NukeSZListCB);
}
