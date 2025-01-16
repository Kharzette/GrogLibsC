#include	"d3d11.h"
#include	<cglm/call.h>
#include	<assert.h>
#include	"CBKeeper.h"
#include	"StuffKeeper.h"
#include	"GrogFont.h"
#include	"uthash.h"
#include	"../UtilityLib/GraphicsDevice.h"
#include	"../UtilityLib/MiscStuff.h"


//This is mainly for debuggish text that goes on the screen
//and changes now and then.  Stuff like a FPS counter or
//player coordinates or something.
//This maintains state frame to frame.
typedef struct	UIVert_t
{
	vec2		Position;
	uint16_t	TexCoord0[2];
}	UIVert;

typedef struct	StringData_t
{
	int			id;			//dictionary id

	UIVert	*mpVerts;
	vec4		mColor;
	vec2		mPosition;
	vec2		mScale;
	int			mLen;		//strlen

	UT_hash_handle	hh;		//makes hashable
}	StringData;

typedef struct	ScreenText_t
{
	GrogFont	*mpFont;

	ID3D11Buffer				*mpVB;
	ID3D11InputLayout			*mpLayout;
	ID3D11ShaderResourceView	*mpFontSRV;
	ID3D11VertexShader			*mpVS;
	ID3D11PixelShader			*mpPS;
	ID3D11DepthStencilState		*mpDSS;
	ID3D11BlendState			*mpBS;
	ID3D11SamplerState			*mpSS;

	bool	mbDirty;
	int		mMaxCharacters;
	int		mNumVerts;

	UIVert	*mpTextBuf;
	StringData	*mpStrings;
}	ScreenText;


//statics
static void	CopyLetters(const GrogFont *pFont, UIVert *pTV, const char *pText);
static void	ReBuildVB(ScreenText *pST, GraphicsDevice *pGD);

static void	MakeVBDesc(D3D11_BUFFER_DESC *pDesc, uint32_t byteSize)
{
	memset(pDesc, 0, sizeof(D3D11_BUFFER_DESC));

	pDesc->BindFlags			=D3D11_BIND_VERTEX_BUFFER;
	pDesc->ByteWidth			=byteSize;
	pDesc->CPUAccessFlags		=DXGI_CPU_ACCESS_DYNAMIC;
	pDesc->MiscFlags			=0;
	pDesc->StructureByteStride	=0;
	pDesc->Usage				=D3D11_USAGE_DYNAMIC;
}


ScreenText	*ST_Create(GraphicsDevice *pGD, const StuffKeeper *pSK, int maxChars,
						const char *pFontName, const char *pFontTexName)
{
	ScreenText	*pRet	=malloc(sizeof(ScreenText));
	memset(pRet, 0, sizeof(ScreenText));

	pRet->mMaxCharacters	=maxChars;
	pRet->mpFontSRV			=StuffKeeper_GetFontSRV(pSK, pFontTexName);
	pRet->mpFont			=StuffKeeper_GetFont(pSK, pFontName);

	pRet->mpLayout	=StuffKeeper_GetInputLayout(pSK, "VPos2Tex02");
	pRet->mpVS		=StuffKeeper_GetVertexShader(pSK, "TextVS");
	pRet->mpPS		=StuffKeeper_GetPixelShader(pSK, "TextPS");
	pRet->mpDSS		=StuffKeeper_GetDepthStencilState(pSK, "DisableDepth");
	pRet->mpBS		=StuffKeeper_GetBlendState(pSK, "AlphaBlending");
	pRet->mpSS		=StuffKeeper_GetSamplerState(pSK, "PointClamp");

	//alloc buf
	pRet->mpTextBuf	=malloc(sizeof(UIVert) * maxChars * 6);

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(UIVert) * maxChars * 6);
	pRet->mpVB	=GD_CreateBuffer(pGD, &bufDesc);

	return	pRet;
}


void	ST_AddString(ScreenText *pST, const char *pSZ, const int id,
					const vec4 color, const vec2 pos, const vec2 scale)
{
	StringData	*pDat	=malloc(sizeof(StringData));
	memset(pDat, 0, sizeof(StringData));

	pDat->mLen	=strlen(pSZ);

	pDat->id	=id;
	glm_vec4_copy(color, pDat->mColor);
	glm_vec2_copy(pos, pDat->mPosition);
	glm_vec2_copy(scale, pDat->mScale);

	pDat->mpVerts	=malloc(sizeof(UIVert) * pDat->mLen * 6);
	CopyLetters(pST->mpFont, pDat->mpVerts, pSZ);

	HASH_ADD_INT(pST->mpStrings, id, pDat);

	pST->mbDirty	=true;
}

void	ST_FreeAll(ScreenText *pST)
{
	pST->mpVB->lpVtbl->Release(pST->mpVB);

	StringData	*pSD, *pTmp;

	//clear stringdata
	HASH_ITER(hh, pST->mpStrings, pSD, pTmp)
	{
		HASH_DEL(pST->mpStrings, pSD);
		free(pSD);
	}

	free(pST);
}

//no linefeed/cr!
void	ST_MeasureString(ScreenText *pST, const char *pToMeasure, vec2 size)
{
	glm_vec2_zero(size);

	int	maxHeight	=0;
	int	len			=strlen(pToMeasure);
	for(int i=0;i < len;i++)
	{
		size[0]	+=GFont_GetCharacterWidth(pST->mpFont, pToMeasure[i]);

		int	height	=GFont_GetCharacterHeight(pST->mpFont);
		if(height > maxHeight)
		{
			maxHeight	=height;
			size[1]		=height;
		}
	}
}

void	ST_ModifyStringScale(ScreenText *pST, int id, const vec2 scale)
{
	StringData	*pSD	=NULL;
	HASH_FIND_INT(pST->mpStrings, &id, pSD);

	if(pSD != NULL)
	{
		glm_vec2_copy(scale, pSD->mScale);
	}
}

void	ST_ModifyStringPosition(ScreenText *pST, int id, const vec2 pos)
{
	StringData	*pSD	=NULL;
	HASH_FIND_INT(pST->mpStrings, &id, pSD);

	if(pSD != NULL)
	{
		glm_vec2_copy(pos, pSD->mPosition);
	}
}

void	ST_ModifyStringText(ScreenText *pST, int id, const char *pText)
{
	StringData	*pSD	=NULL;
	HASH_FIND_INT(pST->mpStrings, &id, pSD);

	if(pSD == NULL)
	{
		return;
	}

	int	len	=strlen(pText);

	free(pSD->mpVerts);

	pSD->mpVerts	=malloc(sizeof(UIVert) * len * 6);
	pSD->mLen		=len;

	CopyLetters(pST->mpFont, pSD->mpVerts, pText);

	pST->mbDirty	=true;
}

void	ST_ModifyStringColor(ScreenText *pST, int id, const vec4 color)
{
	StringData	*pSD	=NULL;
	HASH_FIND_INT(pST->mpStrings, &id, pSD);

	if(pSD != NULL)
	{
		glm_vec4_copy(color, pSD->mColor);
	}
}

void	ST_DeleteString(ScreenText *pST, int id)
{
	StringData	*pSD	=NULL;
	HASH_FIND_INT(pST->mpStrings, &id, pSD);

	if(pSD == NULL)
	{
		return;	//already gone?
	}

	HASH_DEL(pST->mpStrings, pSD);
	free(pSD);
}

void	ST_Update(ScreenText *pST, GraphicsDevice *pGD)
{
	if(!pST->mbDirty)
	{
		return;
	}

	ReBuildVB(pST, pGD);

	pST->mbDirty	=false;
}

void	ST_Draw(ScreenText *pST, GraphicsDevice *pGD, CBKeeper *pCBK)
{
	//if this assert fires, make sure
	//all text modification stuff happens
	//before the call to update
	assert(!pST->mbDirty);

	if(pST->mNumVerts <= 0)
	{
		return;
	}

	GD_IASetPrimitiveTopology(pGD, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	GD_IASetVertexBuffers(pGD, pST->mpVB, 12, 0);
	GD_IASetIndexBuffers(pGD, NULL, DXGI_FORMAT_UNKNOWN, 0);
	GD_IASetInputLayout(pGD, pST->mpLayout);

	GD_VSSetShader(pGD, pST->mpVS);
	GD_PSSetShader(pGD, pST->mpPS);
	GD_PSSetSRV(pGD, pST->mpFontSRV, 0);

	GD_PSSetSampler(pGD, pST->mpSS, 0);

	GD_OMSetDepthStencilState(pGD, pST->mpDSS);
	GD_OMSetBlendState(pGD, pST->mpBS);

	CBK_Set2DCBToShaders(pCBK, pGD);

	int			offset	=0;
	StringData	*pSD, *pTmp;
	HASH_ITER(hh, pST->mpStrings, pSD, pTmp)
	{
		int	len	=pSD->mLen * 6;

		CBK_SetTextTransform(pCBK, pSD->mPosition, pSD->mScale);
		CBK_SetTextColor(pCBK, pSD->mColor);

		CBK_UpdateTwoD(pCBK, pGD);

		GD_Draw(pGD, len, offset);

		offset	+=len;
	}	
}


static void	CopyLetters(const GrogFont *pFont, UIVert *pTV, const char *pText)
{
	int	curWidth	=0;
	int	szLen		=strlen(pText);

	vec2	unitX	={	1.0f, 0.0f	};
	vec2	unitY	={	0.0f, 1.0f	};

	for(int i=0;i < szLen;i++)
	{
		int	nextWidth	=curWidth + GFont_GetCharacterWidth(pFont, pText[i]);
		int	sixi		=i * 6;

		vec2	xCoord, xCoord2, yCoord, yCoord2, uv;

		glm_vec2_scale(unitX, curWidth, xCoord);
		glm_vec2_scale(unitX, nextWidth, xCoord2);
		glm_vec2_zero(yCoord);
		glm_vec2_scale(unitY, GFont_GetCharacterHeight(pFont), yCoord2);

		//note the winding order reversal here
		glm_vec2_copy(xCoord, pTV[sixi].Position);
		GFont_GetUV(pFont, pText[i], 0, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi].TexCoord0);

		glm_vec2_copy(xCoord2, pTV[sixi + 2].Position);
		GFont_GetUV(pFont, pText[i], 1, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi + 2].TexCoord0);

		glm_vec2_add(xCoord2, yCoord2, pTV[sixi + 1].Position);
		GFont_GetUV(pFont, pText[i], 2, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi + 1].TexCoord0);

		glm_vec2_copy(xCoord, pTV[sixi + 3].Position);
		GFont_GetUV(pFont, pText[i], 3, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi + 3].TexCoord0);

		glm_vec2_add(xCoord2, yCoord2, pTV[sixi + 5].Position);
		GFont_GetUV(pFont, pText[i], 4, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi + 5].TexCoord0);

		glm_vec2_add(xCoord, yCoord2, pTV[sixi + 4].Position);
		GFont_GetUV(pFont, pText[i], 5, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi + 4].TexCoord0);

		curWidth	=nextWidth;
	}
}

static void	ReBuildVB(ScreenText *pST, GraphicsDevice *pGD)
{
	pST->mNumVerts	=0;

	D3D11_MAPPED_SUBRESOURCE	msr;

	memset(&msr, 0, sizeof(D3D11_MAPPED_SUBRESOURCE));

	GD_MapDiscard(pGD, (ID3D11Resource *)pST->mpVB, &msr);

	StringData	*pSD, *pTmp;

	int	twoTriSize	=sizeof(UIVert) * 6;

	HASH_ITER(hh, pST->mpStrings, pSD, pTmp)
	{
		int	nextSize	=pST->mNumVerts + pSD->mLen;

		if(nextSize > pST->mMaxCharacters)
		{
			//TODO: warn
			continue;
		}

		memcpy(msr.pData + (pST->mNumVerts * twoTriSize), pSD->mpVerts, pSD->mLen * twoTriSize);

		pST->mNumVerts	+=pSD->mLen;
	}

	GD_UnMap(pGD, (ID3D11Resource *)pST->mpVB);
}