#include	"d3d11.h"
#include	<cglm/call.h>
#include	<assert.h>
#include	"CBKeeper.h"
#include	"StuffKeeper.h"
#include	"GrogFont.h"
#include	"uthash.h"
#include	"../UtilityLib/GraphicsDevice.h"
#include	"../UtilityLib/MiscStuff.h"


//This is for stateless text drawn for UI
//The draw data is rebuilt every draw
//With my debug strings I'd have shader variables control
//stuff like scale and position and colour, but if I want
//to draw all text at once, I need to track that stuff
//per vert
//
//right before falling asleep I realized there will be zorder problems...
//might need multiple draw calls
//OR!... draw shapes and text together?
typedef struct	TextVert_t
{
	vec2		Position;
	uint16_t	Color[4];		//float16 xyzw
	uint16_t	TexCoord0[2];	//float16 xy
}	TextVert;

typedef struct	GrogText_t
{
	//D3D Stuff
	ID3D11Buffer				*mpVB;
	ID3D11InputLayout			*mpLayout;
	ID3D11VertexShader			*mpVS;
	ID3D11PixelShader			*mpPS;
	ID3D11DepthStencilState		*mpDSS;
	ID3D11BlendState			*mpBS;
	ID3D11SamplerState			*mpSS;

	//font
	GrogFont	*mpFont;

	int		mMaxCharacters;	//enough for all of this font onscreen
	int		mNumVerts;

	bool	mbDrawStage;

	TextVert	*mpTextBuf;

}	GrogText;

//statics
static void	MakeVBDesc(D3D11_BUFFER_DESC *pDesc, uint32_t byteSize);


//This creates the stuff needed to draw text.
//create one of these per font in use
GrogText	*GText_Create(GraphicsDevice *pGD, const StuffKeeper *pSK, GrogFont *pFont, int maxChars)
{
	GrogText	*pRet	=malloc(sizeof(GrogText));
	memset(pRet, 0, sizeof(GrogText));

	pRet->mMaxCharacters	=maxChars;

	pRet->mpLayout	=StuffKeeper_GetInputLayout(pSK, "VPos2Tex02");
	pRet->mpVS		=StuffKeeper_GetVertexShader(pSK, "TextVS");
	pRet->mpPS		=StuffKeeper_GetPixelShader(pSK, "TextPS");
	pRet->mpDSS		=StuffKeeper_GetDepthStencilState(pSK, "DisableDepth");
	pRet->mpBS		=StuffKeeper_GetBlendState(pSK, "AlphaBlending");
	pRet->mpSS		=StuffKeeper_GetSamplerState(pSK, "PointClamp");

	pRet->mpFont	=pFont;

	//alloc buf
	pRet->mpTextBuf	=malloc(sizeof(TextVert) * maxChars * 6);

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(TextVert) * maxChars * 6);
	pRet->mpVB	=GD_CreateBuffer(pGD, &bufDesc);

	return	pRet;
}

void	GText_FreeAll(GrogText *pGT)
{
	pGT->mpVB->lpVtbl->Release(pGT->mpVB);

	free(pGT->mpTextBuf);

	free(pGT);
}

void	GText_BeginDraw(GrogText *pGT)
{
	assert(!pGT->mbDrawStage);

	pGT->mNumVerts		=0;
	pGT->mbDrawStage	=true;
}

void	GText_EndDraw(GrogText *pGT, GraphicsDevice *pGD)
{
	assert(pGT->mbDrawStage);

	pGT->mbDrawStage	=false;

	D3D11_MAPPED_SUBRESOURCE	msr;

	memset(&msr, 0, sizeof(D3D11_MAPPED_SUBRESOURCE));

	GD_MapDiscard(pGD, (ID3D11Resource *)pGT->mpVB, &msr);

	memcpy(msr.pData, pGT->mpTextBuf, sizeof(TextVert) * pGT->mNumVerts);

	GD_UnMap(pGD, (ID3D11Resource *)pGT->mpVB);
}

//Returns the longest line width in the string
float	GText_MeasureText(const GrogText *pGT, const char *pText)
{
	int	len	=strlen(pText);
	if(len <= 0)
	{
		return	0.0f;
	}

	float	maxWidth	=0.0f;
	float	lineWidth	=0.0f;

	for(int i=0;i < len;i++)
	{
		char	c	=pText[i];

		if(c == '\n')
		{
			maxWidth	=fmax(maxWidth, lineWidth);
			lineWidth	=0;
			continue;
		}
		lineWidth	+=Font_GetCharacterWidth(pGT->mpFont, c);
	}

	return	fmax(maxWidth, lineWidth);
}


void	GText_Render(GrogText *pGT, GraphicsDevice *pGD)
{
	if(pGT->mNumVerts <= 0)
	{
		return;
	}

	if(pGT == NULL || pGD == NULL)
	{
		return;
	}

	if(pGT->mNumVerts <= 0)
	{
		return;
	}

	GD_IASetPrimitiveTopology(pGD, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	GD_IASetVertexBuffers(pGD, pGT->mpVB, sizeof(TextVert), 0);
	GD_IASetIndexBuffers(pGD, NULL, DXGI_FORMAT_UNKNOWN, 0);
	GD_IASetInputLayout(pGD, pGT->mpLayout);

	GD_VSSetShader(pGD, pGT->mpVS);
	GD_PSSetShader(pGD, pGT->mpPS);
	GD_PSSetSRV(pGD, Font_GetSRV(pGT->mpFont), 0);

	GD_PSSetSampler(pGD, pGT->mpSS, 0);

	GD_OMSetDepthStencilState(pGD, pGT->mpDSS);
	GD_OMSetBlendState(pGD, pGT->mpBS);

	GD_Draw(pGD, pGT->mNumVerts, 0);
}


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

static int	CopyLetters(const GrogText *pGT, TextVert *pTV, const char *pText)
{
	int	curWidth	=0;
	int	szLen		=strlen(pText);
	int	copied		=0;

	//check bounds
	if(szLen + (pGT->mNumVerts / 6) > pGT->mMaxCharacters)
	{
		printf("Overflow of GrogText buffer...\n");
		return	0;
	}

	vec2	unitX	={	1.0f, 0.0f	};
	vec2	unitY	={	0.0f, 1.0f	};

	vec2	yCoord, yCoord2;

	glm_vec2_zero(yCoord);
	glm_vec2_scale(unitY, Font_GetCharacterHeight(pGT->mpFont), yCoord2);

	int	sixi	=0;
	for(int i=0;i < szLen;i++)
	{
		int	nextWidth	=curWidth + Font_GetCharacterWidth(pGT->mpFont, pText[i]);

		vec2	xCoord, xCoord2, yCoord, yCoord2, uv;

		glm_vec2_scale(unitX, curWidth, xCoord);
		glm_vec2_scale(unitX, nextWidth, xCoord2);

		//check for \n
		//TODO: handle \r and dos CR
		if(pText[i] == '\n')
		{
			curWidth	=0;
			glm_vec2_add(yCoord2, yCoord, yCoord);			
			continue;
		}

		//note the winding order reversal here
		glm_vec2_copy(xCoord, pTV[sixi].Position);
		Font_GetUV(pGT->mpFont, pText[i], 0, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi].TexCoord0);

		glm_vec2_copy(xCoord2, pTV[sixi + 2].Position);
		Font_GetUV(pGT->mpFont, pText[i], 1, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi + 2].TexCoord0);

		glm_vec2_add(xCoord2, yCoord2, pTV[sixi + 1].Position);
		Font_GetUV(pGT->mpFont, pText[i], 2, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi + 1].TexCoord0);

		glm_vec2_copy(xCoord, pTV[sixi + 3].Position);
		Font_GetUV(pGT->mpFont, pText[i], 3, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi + 3].TexCoord0);

		glm_vec2_add(xCoord2, yCoord2, pTV[sixi + 5].Position);
		Font_GetUV(pGT->mpFont, pText[i], 4, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi + 5].TexCoord0);

		glm_vec2_add(xCoord, yCoord2, pTV[sixi + 4].Position);
		Font_GetUV(pGT->mpFont, pText[i], 5, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi + 4].TexCoord0);

		curWidth	=nextWidth;
		sixi		=(i + 1) * 6;
		copied++;
	}
	return	copied;
}


void	GText_DrawString(GrogText *pGT, const char *pText, vec2 pos, vec4 colour)
{
	if(pGT == NULL || !pGT->mbDrawStage)
	{
		return;
	}

	int	curWidth	=0;
	int	szLen		=strlen(pText);

	//check bounds
	if(szLen + (pGT->mNumVerts / 6) > pGT->mMaxCharacters)
	{
		printf("Overflow of GrogText buffer...\n");
		return	0;
	}

	vec2	unitX	={	1.0f, 0.0f	};
	vec2	unitY	={	0.0f, 1.0f	};

	vec2	yCoord, yCoord2;

	glm_vec2_zero(yCoord);
	glm_vec2_scale(unitY, Font_GetCharacterHeight(pGT->mpFont), yCoord2);

	//add in position
	glm_vec2_add(pos, yCoord, yCoord);
	glm_vec2_add(pos, yCoord2, yCoord2);

	TextVert	*pTV	=pGT->mpTextBuf;

	int	sixi	=pGT->mNumVerts / 6;
	for(int i=0;i < szLen;i++)
	{
		int	nextWidth	=curWidth + Font_GetCharacterWidth(pGT->mpFont, pText[i]);

		vec2	xCoord, xCoord2, yCoord, yCoord2, uv;

		glm_vec2_scale(unitX, curWidth, xCoord);
		glm_vec2_scale(unitX, nextWidth, xCoord2);

		//add in position
		glm_vec2_add(pos, xCoord, xCoord);
		glm_vec2_add(pos, xCoord2, xCoord2);

		//check for \n
		//TODO: handle \r and dos CR
		if(pText[i] == '\n')
		{
			curWidth	=0;
			glm_vec2_add(yCoord2, yCoord, yCoord);			
			continue;
		}

		//note the winding order reversal here
		glm_vec2_copy(xCoord, pTV[sixi].Position);
		Font_GetUV(pGT->mpFont, pText[i], 0, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi].TexCoord0);
		Misc_ConvertVec4ToF16(colour, pTV[sixi].Color);

		glm_vec2_copy(xCoord2, pTV[sixi + 2].Position);
		Font_GetUV(pGT->mpFont, pText[i], 1, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi + 2].TexCoord0);
		Misc_ConvertVec4ToF16(colour, pTV[sixi + 2].Color);

		glm_vec2_add(xCoord2, yCoord2, pTV[sixi + 1].Position);
		Font_GetUV(pGT->mpFont, pText[i], 2, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi + 1].TexCoord0);
		Misc_ConvertVec4ToF16(colour, pTV[sixi + 1].Color);

		glm_vec2_copy(xCoord, pTV[sixi + 3].Position);
		Font_GetUV(pGT->mpFont, pText[i], 3, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi + 3].TexCoord0);
		Misc_ConvertVec4ToF16(colour, pTV[sixi + 3].Color);

		glm_vec2_add(xCoord2, yCoord2, pTV[sixi + 5].Position);
		Font_GetUV(pGT->mpFont, pText[i], 4, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi + 5].TexCoord0);
		Misc_ConvertVec4ToF16(colour, pTV[sixi + 5].Color);

		glm_vec2_add(xCoord, yCoord2, pTV[sixi + 4].Position);
		Font_GetUV(pGT->mpFont, pText[i], 5, uv);
		Misc_ConvertVec2ToF16(uv, pTV[sixi + 4].TexCoord0);
		Misc_ConvertVec4ToF16(colour, pTV[sixi + 4].Color);

		curWidth		=nextWidth;
		sixi			=(i + 1) * 6;
		pGT->mNumVerts	+=6;
	}
}