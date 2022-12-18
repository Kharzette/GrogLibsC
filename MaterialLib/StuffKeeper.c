#include	<stdint.h>
#include	<stdio.h>
#include	<math.h>
#include	<ctype.h>
#include	<assert.h>
#include	<dirent.h>
#include	<sys/stat.h>
#include	<errno.h>
#include	"d3d11.h"
#include	"utstring.h"
#include	"png.h"
#include	"Font.h"
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

	DictSZ	*mpResources;
	DictSZ	*mpTextures;

	DictSZ	*mpVShaders;
	DictSZ	*mpPShaders;

	DictSZ	*mpFonts;
	DictSZ	*mpFontTextures;

	DictSZ	*mpSRVs;
	DictSZ	*mpFontSRVs;

}	StuffKeeper;

typedef struct	ShaderBytes_t
{
	uint8_t	*mpBytes;
	int		mLen;
}	ShaderBytes;

//context for iterators
typedef struct	StuffContext_t
{
	StuffKeeper		*mpSK;
	GraphicsDevice	*mpGD;
}	StuffContext;

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
void	PrintEntryPointsCB(const UT_string *pKey, const void *pValue, void *pContext)
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

	ShaderBytes	*sb	=malloc(sizeof(ShaderBytes));

	sb->mLen	=fileLen;

	//alloc space
	sb->mpBytes	=malloc(fileLen);

	size_t	read	=fread(sb->mpBytes, 1, fileLen, f);
	assert(read == fileLen);

	//store
	DictSZ_Add(ppStorage, pEntryPoint, sb);

	fclose(f);
}


static void	LoadShaders(StuffKeeper *pSK, ShaderModel sm)
{
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


static void	PreMultAndLinearRGB(uint8_t **pRows, int width, int height)
{
	float	oo255	=1.0f / 255.0f;

	for(int y=0;y < height;y++)
	{
		for(int x=0;x < width;x++)
		{
			int	ofsX	=(x * 3);

			uint8_t	cR	=pRows[y][ofsX];
			uint8_t	cG	=pRows[y][ofsX + 1];
			uint8_t	cB	=pRows[y][ofsX + 2];

			float	xc	=cR * 00255;
			float	yc	=cG * 00255;
			float	zc	=cB * 00255;

			//convert to linear
			xc	=powf(xc, 2.2);
			yc	=powf(yc, 2.2);
			zc	=powf(zc, 2.2);

			pRows[y][ofsX]		=(uint8_t)(xc * 255.0f);
			pRows[y][ofsX + 1]	=(uint8_t)(yc * 255.0f);
			pRows[y][ofsX + 2]	=(uint8_t)(zc * 255.0f);
		}
	}
}

static void	PreMultAndLinearRGBA(uint8_t **pRows, int width, int height)
{
	float	oo255	=1.0f / 255.0f;

	for(int y=0;y < height;y++)
	{
		for(int x=0;x < width;x++)
		{
			int	ofsX	=(x * 4);

			uint8_t	cR	=pRows[y][ofsX];
			uint8_t	cG	=pRows[y][ofsX + 1];
			uint8_t	cB	=pRows[y][ofsX + 2];
			uint8_t	cA	=pRows[y][ofsX + 3];

			float	xc	=cR * 00255;
			float	yc	=cG * 00255;
			float	zc	=cB * 00255;
			float	wc	=cA * 00255;

			//convert to linear
			xc	=powf(xc, 2.2);
			yc	=powf(yc, 2.2);
			zc	=powf(zc, 2.2);

			//premultiply alpha
			xc	*=wc;
			yc	*=wc;
			zc	*=wc;

			pRows[y][ofsX]		=(uint8_t)(xc * 255.0f);
			pRows[y][ofsX + 1]	=(uint8_t)(yc * 255.0f);
			pRows[y][ofsX + 2]	=(uint8_t)(zc * 255.0f);
		}
	}
}


static ID3D11Texture2D *LoadTexture(GraphicsDevice *pGD, const UT_string *pPath)
{
	FILE	*f	=fopen(utstring_body(pPath), "rb");
	if(f == NULL)
	{
		return	NULL;
	}

	png_structp	pPng	=png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(pPng == NULL)
	{
		return	NULL;
	}

	png_infop	pInfo	=png_create_info_struct(pPng);
	if(pInfo == NULL)
	{
		return	NULL;
	}

	if(setjmp(png_jmpbuf(pPng)))
	{
		return	NULL;
	}

	png_init_io(pPng, f);
	png_read_info(pPng, pInfo);

	png_uint_32	width	=png_get_image_width(pPng, pInfo);
	png_uint_32	height	=png_get_image_height(pPng, pInfo);

	png_byte	colType		=png_get_color_type(pPng, pInfo);
	png_byte	bitDepth	=png_get_bit_depth(pPng, pInfo);

	int	numPasses	=png_set_interlace_handling(pPng);
	
	if(bitDepth == 16)
	{
		png_set_strip_16(pPng);
	}
	
	if(colType == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_palette_to_rgb(pPng);
	}
	
	//PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
	if(colType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
	{
		png_set_expand_gray_1_2_4_to_8(pPng);
	}
	
	if(png_get_valid(pPng, pInfo, PNG_INFO_tRNS))
	{
		png_set_tRNS_to_alpha(pPng);
	}
	
	if(colType == PNG_COLOR_TYPE_GRAY ||
		colType == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		png_set_gray_to_rgb(pPng);
	}

	png_read_update_info(pPng, pInfo);

	if(setjmp(png_jmpbuf(pPng)))
	{
		return	NULL;
	}

	int	rowPitch	=png_get_rowbytes(pPng, pInfo);

	png_bytep	*pRows	=malloc(sizeof(png_bytep) * height);
	for(int y=0;y < height;y++)
	{
		pRows[y]	=malloc(rowPitch);
	}

	png_read_image(pPng, pRows);

	fclose(f);

	if(colType == PNG_COLOR_TYPE_RGB)
	{
		PreMultAndLinearRGB(pRows, width, height);
	}
	else if(colType == PNG_COLOR_TYPE_RGBA)
	{
		PreMultAndLinearRGBA(pRows, width, height);
	}
	else if(colType == PNG_COLOR_TYPE_PALETTE)
	{

	}
	else
	{
		printf("png %s color type %d unsupported!\n", utstring_body(pPath), colType);

		png_destroy_read_struct(&pPng, &pInfo, NULL);
		for(int y=0;y < height;y++)
		{
			free(pRows[y]);
		}
		free(pRows);
		return	NULL;
	}

	png_destroy_read_struct(&pPng, &pInfo, NULL);

	ID3D11Texture2D	*pTex	=GraphicsDevice_MakeTexture(pGD, pRows, width, height, rowPitch);

	//free data
	for(int y=0;y < height;y++)
	{
		free(pRows[y]);
	}
	free(pRows);

	return	pTex;
}


static void LoadResources(GraphicsDevice *pGD, StuffKeeper *pSK)
{
	if(!FileStuff_DirExists("Textures"))
	{
		return;
	}

	UT_string	*pTexDir, *pFilePath;
	utstring_new(pTexDir);
	utstring_new(pFilePath);

	utstring_printf(pTexDir, "%s", "Textures");
	
	DIR	*pDir	=opendir(utstring_body(pTexDir));
	for(;;)
	{
		struct dirent	*pDE	=readdir(pDir);
		if(pDE == NULL)
		{
			break;
		}

		int	len	=strlen(pDE->d_name);
		if(len < 3)
		{
			continue;	//probably . or ..
		}

		UT_string	*pExt	=SZ_GetExtension(pDE->d_name);
		if(pExt == NULL)
		{
			continue;
		}

		//png?
		int	upper	=strncmp(utstring_body(pExt), ".PNG", utstring_len(pExt));
		int	lower	=strncmp(utstring_body(pExt), ".png", utstring_len(pExt));

		utstring_done(pExt);

		if(upper && lower)
		{
			continue;
		}

		utstring_clear(pFilePath);
		utstring_printf(pFilePath, "%s/%s", utstring_body(pTexDir), pDE->d_name);

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
			//isolate the filename without the extension
			UT_string	*pExtLess	=SZ_StripExtensionUT(pFilePath);
			UT_string	*pJustName	=SZ_SubStringUTStart(pExtLess, 9);

			ID3D11Texture2D	*pTex	=LoadTexture(pGD, pFilePath);
			if(pTex != NULL)
			{
				DictSZ_Add(&pSK->mpTextures, pJustName, pTex);

				ID3D11Resource	*pRes;
				pTex->lpVtbl->QueryInterface(pTex, &IID_ID3D11Resource, (void **)&pRes);
				if(pRes == NULL)
				{
					printf("Error getting resource interface from a texture!\n");					
				}
				else
				{
					DictSZ_Add(&pSK->mpResources, pJustName, pRes);
				}
			}

			utstring_done(pExtLess);
			utstring_done(pJustName);
		}
	}
	closedir(pDir);

	utstring_done(pTexDir);
	utstring_done(pFilePath);
}


static void	CreateVShaderCB(const UT_string *pKey, const void *pValue, void *pContext)
{
	StuffContext	*pCon	=pContext;

	const ShaderBytes	*pSB	=pValue;

	ID3D11VertexShader	*pVS	=GraphicsDevice_CreateVertexShader(
									pCon->mpGD, pSB->mpBytes, pSB->mLen);
	if(pVS != NULL)
	{
		DictSZ_Add(&pCon->mpSK->mpVShaders, pKey, pVS);
	}
}

static void	CreatePShaderCB(const UT_string *pKey, const void *pValue, void *pContext)
{
	StuffContext	*pCon	=pContext;

	const ShaderBytes	*pSB	=pValue;

	ID3D11VertexShader	*pVS	=GraphicsDevice_CreatePixelShader(
									pCon->mpGD, pSB->mpBytes, pSB->mLen);
	if(pVS != NULL)
	{
		DictSZ_Add(&pCon->mpSK->mpPShaders, pKey, pVS);
	}
}


static void CreateShadersFromCode(StuffKeeper *pSK, GraphicsDevice *pGD)
{
	StuffContext	sc;

	sc.mpGD	=pGD;
	sc.mpSK	=pSK;
	
	DictSZ_ForEach(pSK->mpVSCode, CreateVShaderCB, &sc);
	DictSZ_ForEach(pSK->mpPSCode, CreatePShaderCB, &sc);
}


void LoadFonts(GraphicsDevice *pGD, StuffKeeper *pSK)
{
	if(!FileStuff_DirExists("Fonts"))
	{
		return;
	}

	UT_string	*pFontDir, *pFilePath;
	utstring_new(pFontDir);
	utstring_new(pFilePath);

	utstring_printf(pFontDir, "%s", "Fonts");
	
	DIR	*pDir	=opendir(utstring_body(pFontDir));
	for(;;)
	{
		struct dirent	*pDE	=readdir(pDir);
		if(pDE == NULL)
		{
			break;
		}

		int	len	=strlen(pDE->d_name);
		if(len < 3)
		{
			continue;	//probably . or ..
		}

		UT_string	*pExt	=SZ_GetExtension(pDE->d_name);
		if(pExt == NULL)
		{
			continue;
		}

		//png?
		int	upper	=strncmp(utstring_body(pExt), ".PNG", utstring_len(pExt));
		int	lower	=strncmp(utstring_body(pExt), ".png", utstring_len(pExt));

		utstring_done(pExt);

		if(upper && lower)
		{
			continue;
		}

		utstring_clear(pFilePath);
		utstring_printf(pFilePath, "%s/%s", utstring_body(pFontDir), pDE->d_name);

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
			//isolate the filename without the extension
			UT_string	*pExtLess	=SZ_StripExtensionUT(pFilePath);
			UT_string	*pJustName	=SZ_SubStringUTStart(pExtLess, 6);

			ID3D11Texture2D	*pTex	=LoadTexture(pGD, pFilePath);
			if(pTex != NULL)
			{
				DictSZ_Add(&pSK->mpFontTextures, pJustName, pTex);

				ID3D11Resource	*pRes;
				pTex->lpVtbl->QueryInterface(pTex, &IID_ID3D11Resource, (void **)&pRes);
				if(pRes == NULL)
				{
					printf("Error getting resource interface from a font texture!\n");					
				}
				else
				{
					DictSZ_Add(&pSK->mpResources, pJustName, pRes);
				}

				//make the actual font
				utstring_clear(pFilePath);
				utstring_printf(pFilePath, "%s.dat", utstring_body(pExtLess));

				Font	*pFont	=Font_Create(pFilePath);
				if(pFont == NULL)
				{
					printf("Error reading font %s\n", utstring_body(pFilePath));
				}
				else
				{
					DictSZ_Add(&pSK->mpFonts, pJustName, pFont);
				}
			}
			utstring_done(pExtLess);
			utstring_done(pJustName);
		}
	}
	closedir(pDir);

	utstring_done(pFontDir);
	utstring_done(pFilePath);
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

	//prepare all dictionaries for new data
	DictSZ_New(&pRet->mpVSCode);
	DictSZ_New(&pRet->mpPSCode);
	DictSZ_New(&pRet->mpVShaders);
	DictSZ_New(&pRet->mpPShaders);
	DictSZ_New(&pRet->mpResources);
	DictSZ_New(&pRet->mpTextures);
	DictSZ_New(&pRet->mpFonts);
	DictSZ_New(&pRet->mpFontTextures);
	DictSZ_New(&pRet->mpSRVs);
	DictSZ_New(&pRet->mpFontSRVs);

	LoadShaders(pRet, sm);
	CreateShadersFromCode(pRet, pGD);
	LoadResources(pGD, pRet);
	LoadFonts(pGD, pRet);

	int	numShaderData	=DictSZ_Count(pRet->mpVSCode);
	numShaderData		+=DictSZ_Count(pRet->mpPSCode);
	int	numTex			=DictSZ_Count(pRet->mpTextures);
	int	numRes			=DictSZ_Count(pRet->mpResources);
	int	numVS			=DictSZ_Count(pRet->mpVShaders);
	int	numPS			=DictSZ_Count(pRet->mpPShaders);
	int	numFonts		=DictSZ_Count(pRet->mpFonts);

	printf("Loaded %d shader data, %d textures, with %d resources, %d vertex shaders, %d pixel shaders, and %d fonts.\n",
		numShaderData, numTex, numRes, numVS, numPS, numFonts);

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

	DictSZ_ForEach(pPSEP, PrintEntryPointsCB, NULL);

	//delete stuff, note that because our void * in the dictionary
	//is a complicated SZList type, additional work needs to be done
	//to clean it up beyond just a free()

	//delete stuff
	DictSZ_ClearCB(&pVSEP, NukeSZListCB);
	DictSZ_ClearCB(&pPSEP, NukeSZListCB);
}
