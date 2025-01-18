#include	"d3d11.h"
#include	"UIStuff.h"
#include	<assert.h>
#include	"CBKeeper.h"
#include	"StuffKeeper.h"
#include	"GrogFont.h"
#include	"uthash.h"
#include	"../UtilityLib/GraphicsDevice.h"
#include	"../UtilityLib/MiscStuff.h"


//This is for stateless text/shapes drawn for UI
//The draw data is rebuilt every frame
typedef struct	UIVert_t
{
	vec2		Position;
	uint16_t	Color[4];		//float16 xyzw
	uint16_t	TexCoord0[4];	//float16 xyzw
}	UIVert;

typedef struct	UIStuff_t
{
	//D3D Stuff
	ID3D11Buffer				*mpVB;	//dynamic
	ID3D11InputLayout			*mpLayout;
	ID3D11VertexShader			*mpVS;
	ID3D11PixelShader			*mpPS;
	ID3D11DepthStencilState		*mpDSS;
	ID3D11BlendState			*mpBS;
	ID3D11SamplerState			*mpSS;

	//device
	GraphicsDevice	*mpGD;

	//font
	GrogFont	*mpFont;

	int		mMaxVerts, mNumVerts;

	bool	mbDrawStage;	//begin/end

	//store up multiple user draws in this
	UIVert	*mpUIBuf;

}	UIStuff;

//statics
static void	MakeVBDesc(D3D11_BUFFER_DESC *pDesc, uint32_t byteSize);
static void	sRender(UIStuff *pUI);

//combined text and shape buffer
UIStuff	*UI_Create(GraphicsDevice *pGD, const StuffKeeper *pSK,
					GrogFont *pFont, int maxVerts)
{
	UIStuff	*pRet	=malloc(sizeof(UIStuff));
	memset(pRet, 0, sizeof(UIStuff));

	pRet->mMaxVerts	=maxVerts;
	pRet->mpGD		=pGD;

	pRet->mpLayout	=StuffKeeper_GetInputLayout(pSK, "VPos2Col0Tex04");
	pRet->mpVS		=StuffKeeper_GetVertexShader(pSK, "UIStuffVS");
	pRet->mpPS		=StuffKeeper_GetPixelShader(pSK, "UIStuffPS");
	pRet->mpDSS		=StuffKeeper_GetDepthStencilState(pSK, "DisableDepth");
	pRet->mpBS		=StuffKeeper_GetBlendState(pSK, "AlphaBlending");
	pRet->mpSS		=StuffKeeper_GetSamplerState(pSK, "PointClamp");

	pRet->mpFont	=pFont;

	//alloc buf
	pRet->mpUIBuf	=malloc(sizeof(UIVert) * maxVerts);

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(UIVert) * maxVerts);
	pRet->mpVB	=GD_CreateBuffer(pGD, &bufDesc);

	return	pRet;
}

void	UI_FreeAll(UIStuff *pUI)
{
	pUI->mpVB->lpVtbl->Release(pUI->mpVB);

	free(pUI->mpUIBuf);

	free(pUI);
}

void	UI_BeginDraw(UIStuff *pUI)
{
	assert(!pUI->mbDrawStage);

	pUI->mNumVerts		=0;
	pUI->mbDrawStage	=true;
}

void	UI_EndDraw(UIStuff *pUI)
{
	assert(pUI->mbDrawStage);

	pUI->mbDrawStage	=false;

	D3D11_MAPPED_SUBRESOURCE	msr;

	memset(&msr, 0, sizeof(D3D11_MAPPED_SUBRESOURCE));

	GD_MapDiscard(pUI->mpGD, (ID3D11Resource *)pUI->mpVB, &msr);

	memcpy(msr.pData, pUI->mpUIBuf, sizeof(UIVert) * pUI->mNumVerts);

	GD_UnMap(pUI->mpGD, (ID3D11Resource *)pUI->mpVB);

	sRender(pUI);
}


static void	sRender(UIStuff *pUI)
{
	if(pUI->mNumVerts <= 0)
	{
		return;
	}

	if(pUI == NULL)
	{
		return;
	}

	if(pUI->mNumVerts <= 0)
	{
		return;
	}

	GD_IASetPrimitiveTopology(pUI->mpGD, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	GD_IASetVertexBuffers(pUI->mpGD, pUI->mpVB, sizeof(UIVert), 0);
	GD_IASetIndexBuffers(pUI->mpGD, NULL, DXGI_FORMAT_UNKNOWN, 0);
	GD_IASetInputLayout(pUI->mpGD, pUI->mpLayout);

	GD_VSSetShader(pUI->mpGD, pUI->mpVS);
	GD_PSSetShader(pUI->mpGD, pUI->mpPS);
	GD_PSSetSRV(pUI->mpGD, GFont_GetSRV(pUI->mpFont), 0);

	GD_PSSetSampler(pUI->mpGD, pUI->mpSS, 0);

	GD_OMSetDepthStencilState(pUI->mpGD, pUI->mpDSS);
	GD_OMSetBlendState(pUI->mpGD, pUI->mpBS);

	GD_Draw(pUI->mpGD, pUI->mNumVerts, 0);
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


void	UI_DrawString(UIStuff *pUI, const char *pText, int len, GrogFont *pFont,
						const vec2 pos, const vec4 colour)
{
	if(pUI == NULL || !pUI->mbDrawStage)
	{
		return;
	}

	vec4	c;
	Misc_SRGBToLinear(colour, c);

	//see if there is a change of font
	if(pUI->mpFont != pFont)
	{
		//font changed, flush what we have
		if(pUI->mNumVerts > 0)
		{
//			printf("Font change: %d\n", pUI->mNumVerts);
			UI_EndDraw(pUI);
			UI_BeginDraw(pUI);
		}
		pUI->mpFont	=pFont;
	}

	int	curWidth	=0;
	int	szLen		=len;

	//check bounds
	if(((szLen * 6) + pUI->mNumVerts) > pUI->mMaxVerts)
	{
		printf("Overflow of UIStuff buffer...\n");
		return;
	}

	vec2	unitX	={	1.0f, 0.0f	};
	vec2	unitY	={	0.0f, 1.0f	};

	vec2	yCoord, yCoord2;

	glm_vec2_zero(yCoord);
	glm_vec2_scale(unitY, GFont_GetCharacterHeight(pUI->mpFont), yCoord2);

	UIVert	*pTV	=pUI->mpUIBuf;

	int	sixi	=pUI->mNumVerts;
	for(int i=0;i < szLen;i++)
	{
		int	nextWidth	=curWidth + GFont_GetCharacterWidth(pUI->mpFont, pText[i]);

		vec2	xCoord, xCoord2;
		vec4	uv	={ 1, 1, 1, 0 };

		glm_vec2_scale(unitX, curWidth, xCoord);
		glm_vec2_scale(unitX, nextWidth, xCoord2);

		char	letter	=pText[i];

		//check for \n
		//TODO: handle \r and dos CR
		if(letter == '\n')
		{
			curWidth	=0;
			glm_vec2_add(yCoord2, yCoord, yCoord);			
			continue;
		}

		//note the winding order reversal here
		UIVert	*pV	=&pTV[sixi];
		glm_vec2_copy(pos, pV->Position);
		glm_vec2_add(xCoord, pV->Position, pV->Position);
		GFont_GetUV(pUI->mpFont, letter, 0, uv);
		Misc_ConvertVec4ToF16(uv, pV->TexCoord0);
		Misc_ConvertVec4ToF16(c, pV->Color);

		pV	=&pTV[sixi + 2];
		glm_vec2_copy(pos, pV->Position);
		glm_vec2_add(xCoord2, pV->Position, pV->Position);
		GFont_GetUV(pUI->mpFont, letter, 1, uv);
		Misc_ConvertVec4ToF16(uv, pV->TexCoord0);
		Misc_ConvertVec4ToF16(c, pV->Color);

		pV	=&pTV[sixi + 1];
		glm_vec2_copy(pos, pV->Position);
		glm_vec2_add(xCoord2, pV->Position, pV->Position);
		glm_vec2_add(yCoord2, pV->Position, pV->Position);
		GFont_GetUV(pUI->mpFont, letter, 2, uv);
		Misc_ConvertVec4ToF16(uv, pV->TexCoord0);
		Misc_ConvertVec4ToF16(c, pV->Color);

		pV	=&pTV[sixi + 3];
		glm_vec2_copy(pos, pV->Position);
		glm_vec2_add(xCoord, pV->Position, pV->Position);
		GFont_GetUV(pUI->mpFont, letter, 3, uv);
		Misc_ConvertVec4ToF16(uv, pV->TexCoord0);
		Misc_ConvertVec4ToF16(c, pV->Color);

		pV	=&pTV[sixi + 5];
		glm_vec2_copy(pos, pV->Position);
		glm_vec2_add(xCoord2, pV->Position, pV->Position);
		glm_vec2_add(yCoord2, pV->Position, pV->Position);
		GFont_GetUV(pUI->mpFont, letter, 4, uv);
		Misc_ConvertVec4ToF16(uv, pV->TexCoord0);
		Misc_ConvertVec4ToF16(c, pV->Color);

		pV	=&pTV[sixi + 4];
		glm_vec2_copy(pos, pV->Position);
		glm_vec2_add(xCoord, pV->Position, pV->Position);
		glm_vec2_add(yCoord2, pV->Position, pV->Position);
		GFont_GetUV(pUI->mpFont, letter, 5, uv);
		Misc_ConvertVec4ToF16(uv, pV->TexCoord0);
		Misc_ConvertVec4ToF16(c, pV->Color);

		curWidth		=nextWidth;
		pUI->mNumVerts	+=6;
		sixi			=pUI->mNumVerts;
	}
}

void	UI_DrawRect(UIStuff *pUI, const UIRect r, const vec4 color)
{
	if(pUI == NULL || !pUI->mbDrawStage)
	{
		return;
	}

	//in my noggin, this should be the other way around
	//this seems counterclockwise which should be culled

	//tri 0
	//top left corner
	pUI->mpUIBuf[pUI->mNumVerts].Position[0]	=r.x;
	pUI->mpUIBuf[pUI->mNumVerts].Position[1]	=r.y;
	pUI->mNumVerts++;

	//bottom left corner
	pUI->mpUIBuf[pUI->mNumVerts].Position[0]	=r.x;
	pUI->mpUIBuf[pUI->mNumVerts].Position[1]	=r.y + r.height;
	pUI->mNumVerts++;

	//top right corner
	pUI->mpUIBuf[pUI->mNumVerts].Position[0]	=r.x + r.width;
	pUI->mpUIBuf[pUI->mNumVerts].Position[1]	=r.y;
	pUI->mNumVerts++;


	//tri1
	//top right corner
	pUI->mpUIBuf[pUI->mNumVerts].Position[0]	=r.x + r.width;
	pUI->mpUIBuf[pUI->mNumVerts].Position[1]	=r.y;
	pUI->mNumVerts++;

	//bottom left corner
	pUI->mpUIBuf[pUI->mNumVerts].Position[0]	=r.x;
	pUI->mpUIBuf[pUI->mNumVerts].Position[1]	=r.y + r.height;
	pUI->mNumVerts++;

	//bottom right corner
	pUI->mpUIBuf[pUI->mNumVerts].Position[0]	=r.x + r.width;
	pUI->mpUIBuf[pUI->mNumVerts].Position[1]	=r.y + r.height;
	pUI->mNumVerts++;

	vec4	c;
	Misc_SRGBToLinear(color, c);

	//zero out the texture with z of 0
	//set w to 1 to boost to white
	vec4	uv	={	0, 0, 0, 1	};

	//color and UV
	for(int i=0;i < 6;i++)
	{
		int	idx	=(pUI->mNumVerts - 6) + i;

		Misc_ConvertVec4ToF16(uv, pUI->mpUIBuf[idx].TexCoord0);
		Misc_ConvertVec4ToF16(c, pUI->mpUIBuf[idx].Color);
	}

	assert(pUI->mNumVerts < pUI->mMaxVerts);
}

void	UI_DrawRectRounded(UIStuff *pUI, const UIRect r, float roundNess, const vec4 color)
{
	if(pUI == NULL || !pUI->mbDrawStage)
	{
		return;
	}

	if(roundNess <= 0.0f)
	{
		UI_DrawRect(pUI, r, color);
		return;
	}

	//roundness should be >0 and <=1
	glm_clamp(roundNess, 0.0f, 1.0f);

	//for rounded, there are points along the rect where the straight
	//lines stop and the arcs begin.
	//        A          B
	//        ************
	//    C*                *D
	//    E*                *F
	//        ************
	//        G          H
	//
	//            A  B
	//            ****
	//           *    *
	//         C*      *D
	//          *      *
	//          *      *
	//          *      *
	//         E*      *F
	//           *    *
	//            ****
	//            G  H
	//
	//Could start with right triangles like CA EG BD FH
	//and then draw ABHG
	//
	//With a roundess of 1 some of those points go away:
	//
	//        A          B
	//        ************
	//    C*                *D
	//        ************
	//        G          H
	//
	//             A  
	//             **
	//           *    *
	//         C*      *D
	//          *      *
	//          *      *
	//          *      *
	//         E*      *F
	//           *    *
	//             **
	//             G
	//

	//find length of sides
	float	xLen	=r.width;
	float	yLen	=r.height;

	//at full roundness, the shortest side forms
	//the diameter of a circle
	float	rad	=0.5f * ((xLen > yLen)? yLen : xLen);

	//scale rad by roundNess factor
	rad	*=roundNess;

	//the middle bit unaffected by roundness
	//assumes wider than taller for now
	vec2	topLeft		={	r.x + rad, r.y	};
	vec2	topRight	={	(r.x + r.width) - rad, r.y	};
	vec2	bottomLeft	={	topLeft[0], (r.y + r.height)	};
	vec2	bottomRight	={	topRight[0], (r.y + r.height)	};

	//in my noggin, this should be the other way around
	//this seems counterclockwise which should be culled
	int	vCnt	=0;
	//tri 0
	//top left corner
	glm_vec2_copy(topLeft, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	//bottom left corner
	glm_vec2_copy(bottomLeft, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	//top right corner
	glm_vec2_copy(topRight, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;


	//tri1
	//top right corner
	glm_vec2_copy(topRight, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	//bottom left corner
	glm_vec2_copy(bottomLeft, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	//bottom right corner
	glm_vec2_copy(bottomRight, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	//draw the corners for debuggery
	//topleft
	vec2	rectTLC	={	r.x, r.y	};				//top left corner
	vec2	rectLM	={	r.x, r.y + rad	};			//rect left middle
	vec2	midLeft	={	r.x + rad, r.y + rad	};	//left middle

	//topRight
	vec2	rectTRC		={	r.x + r.width, r.y	};					//top left corner
	vec2	rectRM		={	(r.x + r.width), r.y + rad	};			//rect left middle
	vec2	midRight	={	(r.x + r.width) - rad, r.y + rad	};	//left middle

	//bottomLeft
	vec2	rectBLC		={	r.x, r.y + r.height	};					//top left corner
	vec2	rectBLM		={	r.x + rad, (r.y + r.height) - rad	};	//rect left middle
	vec2	midBLeft	={	r.x, (r.y + r.height) - rad	};			//left middle

	//bottomRight
	vec2	rectBRC		={	r.x + r.width, r.y + r.height	};					//top left corner
	vec2	rectBRM		={	(r.x + r.width) - rad, (r.y + r.height) - rad	};	//rect left middle
	vec2	midBRight	={	r.x + r.width, (r.y + r.height) - rad	};			//left middle

	//square between corners right tri 0
	glm_vec2_copy(rectRM, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	glm_vec2_copy(midRight, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	glm_vec2_copy(midBRight, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	//square between corners right tri 1
	glm_vec2_copy(midRight, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	glm_vec2_copy(rectBRM, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	glm_vec2_copy(midBRight, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;
	
	//square between corners left tri 0
	glm_vec2_copy(rectLM, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	glm_vec2_copy(midBLeft, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	glm_vec2_copy(midLeft, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	//square between corners left tri 1
	glm_vec2_copy(midLeft, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	glm_vec2_copy(midBLeft, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	glm_vec2_copy(rectBLM, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	//top right corner
	glm_vec2_copy(bottomRight, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	glm_vec2_copy(midBRight, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	glm_vec2_copy(rectBRM, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	//bottom left corner
	glm_vec2_copy(bottomLeft, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	glm_vec2_copy(rectBLM, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	glm_vec2_copy(midBLeft, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	//top left corner
	glm_vec2_copy(topLeft, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	glm_vec2_copy(rectLM, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	glm_vec2_copy(midLeft, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	//top right corner
	glm_vec2_copy(topRight, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	glm_vec2_copy(midRight, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	glm_vec2_copy(rectRM, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;	vCnt++;

	vec4	c;
	Misc_SRGBToLinear(color, c);

	//zero out the texture with z of 0
	//set w to 1 to boost to white
	vec4	uv	={	0, 0, 0, 1	};

	//color and UV
	for(int i=0;i < vCnt;i++)
	{
		int	idx	=(pUI->mNumVerts - vCnt) + i;

		Misc_ConvertVec4ToF16(uv, pUI->mpUIBuf[idx].TexCoord0);
		Misc_ConvertVec4ToF16(c, pUI->mpUIBuf[idx].Color);
	}

	assert(pUI->mNumVerts < pUI->mMaxVerts);
}