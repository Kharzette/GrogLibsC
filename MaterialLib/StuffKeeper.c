#include	<stdint.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<assert.h>
#include	<dirent.h>
#include	<sys/stat.h>
#include	<errno.h>
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

	DictSZ	*mpVSCode;	//VS code bytes for entry point key
	DictSZ	*mpPSCode;	//PS code bytes for entry point key

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


//caller should free
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

//caller should free
static UT_string	*SMToString(ShaderModel sm)
{
	UT_string	*pRet;
	utstring_new(pRet);

	switch(sm)
	{
		case	SM2:
			utstring_printf(pRet, "%s", "SM2");
			return	pRet;
		case	SM4:
			utstring_printf(pRet, "%s", "SM4");
			return	pRet;
		case	SM41:
			utstring_printf(pRet, "%s", "SM41");
			return	pRet;
		case	SM5:
			utstring_printf(pRet, "%s", "SM5");
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


//load and store bytecode
static void LoadCompiledShader(DictSZ **ppStorage, const UT_string *pPath, const UT_string *pEntryPoint)
{
	FILE	*f	=fopen(utstring_body(pPath), "rb");

	//see how big the file is
	fseek(f, 0, SEEK_END);
	long	fileLen	=ftell(f);
	rewind(f);

	//alloc space
	uint8_t	*pCode	=malloc(fileLen);

	size_t	read	=fread(pCode, 1, fileLen, f);
	assert(read == fileLen);

	//store
	DictSZ_Add(ppStorage, pEntryPoint, pCode);

	fclose(f);
}


static void	LoadShaders(StuffKeeper *pSK, ShaderModel sm)
{
	//prepare bytecode storage
	DictSZ_New(&pSK->mpVSCode);
	DictSZ_New(&pSK->mpPSCode);

	//check dirs
	UT_string	*pCSDir;
	utstring_new(pCSDir);

	UT_string	*pVer	=SMToString(sm);

	utstring_printf(pCSDir, "%s", "CompiledShaders/");

	if(!FileStuff_DirExists(utstring_body(pCSDir)))
	{
		utstring_done(pVer);
		utstring_done(pCSDir);
		return;
	}

	utstring_concat(pCSDir, pVer);
	if(!FileStuff_DirExists(utstring_body(pCSDir)))
	{
		utstring_done(pVer);
		utstring_done(pCSDir);
		return;
	}

	UT_string	*pVSDir, *pPSDir;
	utstring_new(pVSDir);
	utstring_new(pPSDir);

	utstring_printf(pVSDir, "%s/VS/", utstring_body(pCSDir));
	utstring_printf(pPSDir, "%s/PS/", utstring_body(pCSDir));

	if(!FileStuff_DirExists(utstring_body(pVSDir)))
	{
		utstring_done(pVer);
		utstring_done(pCSDir);
		utstring_done(pVSDir);
		utstring_done(pPSDir);
		return;
	}
	if(!FileStuff_DirExists(utstring_body(pPSDir)))
	{
		utstring_done(pVer);
		utstring_done(pCSDir);
		utstring_done(pVSDir);
		utstring_done(pPSDir);
		return;
	}	

	UT_string	*pFilePath;
	utstring_new(pFilePath);

	//load all vertex
	DIR	*pDir	=opendir(utstring_body(pVSDir));
	for(;;)
	{
		struct dirent	*pDE	=readdir(pDir);
		if(pDE == NULL)
		{
			break;
		}

		UT_string	*pDName;
		utstring_new(pDName);
		utstring_printf(pDName, "%s", pDE->d_name);

		utstring_clear(pFilePath);

		utstring_concat(pFilePath, pVSDir);
		utstring_concat(pFilePath, pDName);

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
			UT_string	*pExtLess	=SZ_StripExtensionUT(pDName);

			LoadCompiledShader(&pSK->mpVSCode, pFilePath, pExtLess);
		}
	}
	closedir(pDir);

	//load pixel
	pDir	=opendir(utstring_body(pPSDir));
	for(;;)
	{
		struct dirent	*pDE	=readdir(pDir);
		if(pDE == NULL)
		{
			break;
		}

		UT_string	*pDName;
		utstring_new(pDName);
		utstring_printf(pDName, "%s", pDE->d_name);

		utstring_clear(pFilePath);

		utstring_concat(pFilePath, pPSDir);
		utstring_concat(pFilePath, pDName);

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
			UT_string	*pExtLess	=SZ_StripExtensionUT(pDName);

			LoadCompiledShader(&pSK->mpPSCode, pFilePath, pExtLess);
		}
	}
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
