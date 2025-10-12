#include	"d3d11.h"
#include	"UIStuff.h"
#include	<dirent.h>
#include	<errno.h>
#include	<sys/stat.h>
#include	<assert.h>
#include	"../MaterialLib/CBKeeper.h"
#include	"../MaterialLib/StuffKeeper.h"
#include	"GrogFont.h"
#include	"uthash.h"
#include	"../UtilityLib/GraphicsDevice.h"
#include	"../UtilityLib/MiscStuff.h"
#include	"../UtilityLib/StringStuff.h"
#include	"../UtilityLib/FileStuff.h"


//clay uses uint16 ids for fonts
//but I'm storing the key in int
typedef struct	UIFonts_t
{
	int	id;

	GrogFont	*mpFont;

	UT_hash_handle	hh;
}	UIFonts;

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
	ID3D11ShaderResourceView	*mpSRV;	//for tex drawing

	//scissory stuff
	ID3D11RasterizerState	*mpNormalRast, *mpScissorRast;

	//device & stuff
	GraphicsDevice	*mpGD;
	StuffKeeper		*mpSK;

	//font for text draw
	GrogFont	*mpFont;

	//hashy list of fonts
	UIFonts	*mpFonts;

	int		mMaxVerts, mNumVerts;

	bool	mbDrawStage;	//begin/end

	//store up multiple user draws in this
	UIVert	*mpUIBuf;

}	UIStuff;

//statics
static void	sMakeVBDesc(D3D11_BUFFER_DESC *pDesc, uint32_t byteSize);
static void	sRender(UIStuff *pUI);
static int	sAddTri(UIStuff *pUI, const vec2 A, const vec2 B, const vec2 C);
static void	sMakeCornerArcPoints(const vec2 A, const vec2 B, const vec2 C,
								 int segments, vec2 *pPoints);
static int	sMakeCornerTris(UIStuff *pUI, int segments,
							const vec2 A, const vec2 B, const vec2 C,
							const vec2 *pPoints);
static int	sAddTriUVC(UIStuff *pUI, const vec2 A, const vec2 B, const vec2 C,
						const vec4 uvA, const vec4 uvB, const vec4 uvC,
						const vec4 color);
static void sComputeAtoL(const UIRect r, float roundNess, vec2 A, vec2 B, vec2 C, vec2 D, vec2 E, vec2 F, vec2 G, vec2 H, vec2 I, vec2 J, vec2 K, vec2 L);
static int	sMakeCornerTrisHull(UIStuff *pUI, int segments, int hullSize,
								const vec2 A, const vec2 B, const vec2 C,
								const vec2 a, const vec2 b, const vec2 c,
								const vec2 *pPoints, const vec2 *pPointsInner);

//combined text and shape buffer
UIStuff	*UI_Create(GraphicsDevice *pGD, StuffKeeper *pSK, int maxVerts)
{
	UIStuff	*pRet	=malloc(sizeof(UIStuff));
	memset(pRet, 0, sizeof(UIStuff));

	pRet->mMaxVerts	=maxVerts;
	pRet->mpGD		=pGD;
	pRet->mpSK		=pSK;

	pRet->mpLayout	=StuffKeeper_GetInputLayout(pSK, "VPos2Col0Tex04");
	pRet->mpVS		=StuffKeeper_GetVertexShader(pSK, "UIStuffVS");
	pRet->mpPS		=StuffKeeper_GetPixelShader(pSK, "UIStuffPS");
	pRet->mpDSS		=StuffKeeper_GetDepthStencilState(pSK, "DisableDepth");
	pRet->mpBS		=StuffKeeper_GetBlendState(pSK, "AlphaBlending");
	pRet->mpSS		=StuffKeeper_GetSamplerState(pSK, "LinearClamp");

	//make scissor rast states
	D3D11_RASTERIZER_DESC	rastDesc;
	rastDesc.AntialiasedLineEnable	=false;
	rastDesc.CullMode				=D3D11_CULL_BACK;
	rastDesc.FillMode				=D3D11_FILL_SOLID;
	rastDesc.FrontCounterClockwise	=true;
	rastDesc.MultisampleEnable		=false;
	rastDesc.DepthBias				=0;
	rastDesc.DepthBiasClamp			=0;
	rastDesc.DepthClipEnable		=true;
	rastDesc.ScissorEnable			=false;
	rastDesc.SlopeScaledDepthBias	=0;

	//create the usual
	pRet->mpNormalRast	=GD_CreateRasterizerState(pGD, &rastDesc);

	//this one is for scissory
	rastDesc.ScissorEnable	=true;
	pRet->mpScissorRast	=GD_CreateRasterizerState(pGD, &rastDesc);

	//alloc buf
	pRet->mpUIBuf	=malloc(sizeof(UIVert) * maxVerts);

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	sMakeVBDesc(&bufDesc, sizeof(UIVert) * maxVerts);
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


void	UI_DrawString(UIStuff *pUI, const char *pText, int len, GrogFont *pFont,
						const vec2 pos, const vec4 colour)
{
	if(pUI == NULL || !pUI->mbDrawStage)
	{
		return;
	}

	vec4	c;
	Misc_SRGBToLinear(colour, c);

	//see if there is a change of srv
	if(pUI->mpFont != pFont || pUI->mpSRV != NULL)
	{
		//font changed, flush what we have
		if(pUI->mNumVerts > 0)
		{
//			printf("Font change: %d\n", pUI->mNumVerts);
			UI_EndDraw(pUI);
			UI_BeginDraw(pUI);
		}
		pUI->mpFont	=pFont;
		pUI->mpSRV	=NULL;
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

	for(int i=0;i < szLen;i++)
	{
		int	nextWidth	=curWidth + GFont_GetCharacterWidth(pUI->mpFont, pText[i]);

		vec2	xCoord, xCoord2;

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

		vec2	A, B, C;
		vec4	aUV	={ 1, 1, 1, 0 }, bUV ={ 1, 1, 1, 0 }, cUV	={ 1, 1, 1, 0 };
		glm_vec2_add(pos, xCoord, A);
		glm_vec2_add(pos, xCoord2, C);
		glm_vec2_add(C, yCoord2, B);

		GFont_GetUV(pUI->mpFont, letter, 0, aUV);
		GFont_GetUV(pUI->mpFont, letter, 2, bUV);
		GFont_GetUV(pUI->mpFont, letter, 1, cUV);

		sAddTriUVC(pUI, A, B, C, aUV, bUV, cUV, colour);

		glm_vec2_add(pos, xCoord, A);
		glm_vec2_add(A, yCoord2, B);
		glm_vec2_add(pos, xCoord2, C);
		glm_vec2_add(C, yCoord2, C);

		GFont_GetUV(pUI->mpFont, letter, 3, aUV);
		GFont_GetUV(pUI->mpFont, letter, 5, bUV);
		GFont_GetUV(pUI->mpFont, letter, 4, cUV);

		sAddTriUVC(pUI, A, B, C, aUV, bUV, cUV, colour);

		curWidth	=nextWidth;
	}
}

void	UI_DrawImage(UIStuff *pUI, const char *szTex, const vec2 pos,
					 float rotation, const vec4 color)
{
	if(pUI == NULL || !pUI->mbDrawStage)
	{
		return;
	}

	ID3D11ShaderResourceView	*pSRV	=StuffKeeper_GetSRV(pUI->mpSK, szTex);
	if(pSRV == NULL)
	{
		printf("No SRV found for %s", szTex);
		return;
	}

	//see if a flush is needed, usually will be
	if(pUI->mpSRV != pSRV)
	{
		if(pUI->mNumVerts > 0)
		{
			UI_EndDraw(pUI);
			UI_BeginDraw(pUI);
		}
	}

	int	*dims	=StuffKeeper_GetResourceDimensions(pUI->mpSK, szTex);

	//set ui srv
	pUI->mpSRV	=pSRV;

	//keep the texture with z of 1
	//set w to 0 for no color boost
	vec4	uv0	={	0, 0, 1, 0	};
	vec4	uv1	={	1, 0, 1, 0	};
	vec4	uv2	={	0, 1, 1, 0	};
	vec4	uv3	={	1, 1, 1, 0	};

	vec4	c;
	Misc_SRGBToLinear(color, c);

	//tri 0
	sAddTriUVC(pUI,
		(vec2) { pos[0], 			pos[1] 				},		//top left corner
		(vec2) { pos[0], 			pos[1] + dims[1]	},		//bottom left corner 
		(vec2) { pos[0] + dims[0],	pos[1]				},		//top right corner
		uv0, uv2, uv1, c);

	//tri1
	sAddTriUVC(pUI,
		(vec2) { pos[0] + dims[0],	pos[1]				},		//top right corner
		(vec2) { pos[0], 			pos[1] + dims[1]	},		//bottom left corner 
		(vec2) { pos[0] + dims[0],	pos[1] + dims[1]	},		//bottom right corner
		uv1, uv2, uv3, c);

	assert(pUI->mNumVerts < pUI->mMaxVerts);
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
	sAddTri(pUI,
		(vec2) { r.x, 			r.y 			},		//top left corner
		(vec2) { r.x, 			r.y + r.height	},		//bottom left corner 
		(vec2) { r.x + r.width,	r.y				});		//top right corner


	//tri1
	sAddTri(pUI,
		(vec2) { r.x + r.width,	r.y 			},		//top right corner
		(vec2) { r.x, 			r.y + r.height	},		//bottom left corner 
		(vec2) { r.x + r.width,	r.y + r.height	});		//bottom right corner

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

void	UI_DrawRectHollow(UIStuff *pUI, const UIRect r, int hullSize, const vec4 color)
{
	if(pUI == NULL || !pUI->mbDrawStage)
	{
		return;
	}

	if(hullSize < 1.0f)
	{
		return;
	}

	//if the hull will meet in the middle, just draw a rect
	if(hullSize >= (r.width * 0.5f))
	{
		UI_DrawRect(pUI, r, color);
		return;
	}
	else if(hullSize >= (r.height * 0.5f))
	{
		UI_DrawRect(pUI, r, color);
		return;
	}

	//  A--------------C
	//  |a            c|
	//  |              |
	//  |b            d|
	//  B--------------D
	vec2	A	={	r.x,			r.y				};	//top left corner
	vec2	B	={	r.x,			r.y + r.height	};	//bottom left corner 
	vec2	C	={	r.x + r.width,	r.y				};	//top right corner
	vec2	D	={	r.x + r.width,	r.y + r.height	};	//bottom right corner

	//shrink the rect for the inner verts
	UIRect	shk	={	r.x + hullSize, r.y + hullSize,
		r.width - (hullSize * 2), r.height - (hullSize * 2)	};

	vec2	a	={	shk.x,				shk.y				};	//top left corner
	vec2	b	={	shk.x,				shk.y + shk.height	};	//bottom left corner 
	vec2	c	={	shk.x + shk.width,	shk.y				};	//top right corner
	vec2	d	={	shk.x + shk.width,	shk.y + shk.height	};	//bottom right corner

	int	vCnt	=0;	
	vCnt	+=sAddTri(pUI, A, a, C);	//tri 0 AaC
	vCnt	+=sAddTri(pUI, c, C, a);	//tri 0 cCa
	vCnt	+=sAddTri(pUI, a, A, B);	//tri 0 aAB
	vCnt	+=sAddTri(pUI, B, b, a);	//tri 0 Bba
	vCnt	+=sAddTri(pUI, b, B, D);	//tri 0 bBD
	vCnt	+=sAddTri(pUI, d, b, D);	//tri 0 dbD
	vCnt	+=sAddTri(pUI, C, c, d);	//tri 0 Ccd
	vCnt	+=sAddTri(pUI, d, D, C);	//tri 0 dDC

	vec4	col;
	Misc_SRGBToLinear(color, col);

	//zero out the texture with z of 0
	//set w to 1 to boost to white
	vec4	uv	={	0, 0, 0, 1	};

	//color and UV
	for(int i=0;i < vCnt;i++)
	{
		int	idx	=(pUI->mNumVerts - vCnt) + i;

		Misc_ConvertVec4ToF16(uv, pUI->mpUIBuf[idx].TexCoord0);
		Misc_ConvertVec4ToF16(col, pUI->mpUIBuf[idx].Color);
	}

	assert(pUI->mNumVerts < pUI->mMaxVerts);
}

//roundness 0 to 1 makes the corners more roundy
//segments makes the curve less polygony
void	UI_DrawRectRounded(UIStuff *pUI, const UIRect r, float roundNess,
							int segments, const vec4 color)
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

	//segments should be >=1 and <=20
	int	seggz	=(segments < 1)? 1 : segments;
	seggz		=(seggz > 20)? 20 : seggz;

	//for rounded, there are points along the rect where the straight
	//lines stop and the arcs begin.  (A B C D E F G H)
	//I J K L are the right angle corners of the corner triangles
	//
	//        A          B
	//        ************
	//    C*  I          J  *D
	//    E*  K          L  *F
	//        ************
	//        G          H
	//

	//compute above points
	vec2	A, B, C, D, E, F, G, H, I, J, K, L;
	sComputeAtoL(r, roundNess, A, B, C, D, E, F, G, H, I, J, K, L);


	//in my noggin, this should be the other way around
	//this seems counterclockwise which should be culled
	//Might be caused by that negative Z in the shader
	int	vCnt	=0;
	vCnt	+=sAddTri(pUI, A, G, B);	//tri 0 AGB
	vCnt	+=sAddTri(pUI, B, G, H);	//tri1 BGH
	vCnt	+=sAddTri(pUI, C, E, I);	//tri2 CEI
	vCnt	+=sAddTri(pUI, I, E, K);	//tri3 IEK
	vCnt	+=sAddTri(pUI, D, J, L);	//tri4 DJL
	vCnt	+=sAddTri(pUI, D, L, F);	//tri5 DLF

	//that is all of the square bits
	//find arc points for the 4 corners
	vec2	points[20];

	//ACI on the diagram above, top left
	sMakeCornerArcPoints(A, C, I, seggz, points);
	vCnt	+=sMakeCornerTris(pUI, seggz, A, C, I, points);

	//DBJ, top right
	sMakeCornerArcPoints(D, B, J, seggz, points);
	vCnt	+=sMakeCornerTris(pUI, seggz, D, B, J, points);

	//EGK, bottom left
	sMakeCornerArcPoints(E, G, K, seggz, points);
	vCnt	+=sMakeCornerTris(pUI, seggz, E, G, K, points);

	//HFL, bottom right
	sMakeCornerArcPoints(H, F, L, seggz, points);
	vCnt	+=sMakeCornerTris(pUI, seggz, H, F, L, points);

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

//like rectround above, but hollow
void	UI_DrawRRHollow(UIStuff *pUI, const UIRect r, float hullSize,
						float roundNess, int segments, const vec4 color)
{
	if(pUI == NULL || !pUI->mbDrawStage)
	{
		return;
	}

	if(hullSize < 1.0f)
	{
		return;
	}

	if(roundNess <= 0.0f)
	{
		UI_DrawRectHollow(pUI, r, hullSize, color);
		return;
	}

	//if the hull will meet in the middle, just draw a rect
	if(hullSize >= (r.width * 0.5f))
	{
		UI_DrawRectRounded(pUI, r, roundNess, segments, color);
		return;
	}
	else if(hullSize >= (r.height * 0.5f))
	{
		UI_DrawRectRounded(pUI, r, roundNess, segments, color);
		return;
	}

	//roundness should be >0 and <=1
	glm_clamp(roundNess, 0.0f, 1.0f);

	//segments should be >=1 and <=20
	int	seggz	=(segments < 1)? 1 : segments;
	seggz		=(seggz > 20)? 20 : seggz;

	//for rounded, there are points along the rect where the straight
	//lines stop and the arcs begin.  (A B C D E F G H)
	//I J K L are the right angle corners of the corner triangles
	//
	//        A          B
	//        ************
	//    C*  I          J  *D
	//    E*  K          L  *F
	//        ************
	//        G          H
	//

	//compute above points
	vec2	A, B, C, D, E, F, G, H, I, J, K, L;
	sComputeAtoL(r, roundNess, A, B, C, D, E, F, G, H, I, J, K, L);

	//shrink the rect and compute more
	UIRect	shrunk	={	r.x + hullSize, r.y + hullSize,
		r.width - (hullSize * 2), r.height - (hullSize * 2)	};

	//lower case for the inner verts
	vec2	a, b, c, d, e, f, g, h, i, j, k, l;
	sComputeAtoL(shrunk, roundNess, a, b, c, d, e, f, g, h, i, j, k, l);

	//top		AaB Bab
	int	vCnt	=sAddTri(pUI, A, a, B);
	vCnt	+=sAddTri(pUI, B, a, b);
	//bottom	gGh hGH
	vCnt	+=sAddTri(pUI, g, G, h);
	vCnt	+=sAddTri(pUI, h, G, H);
	//left		CEc cEe
	vCnt	+=sAddTri(pUI, C, E, c);
	vCnt	+=sAddTri(pUI, c, E, e);
	//right		Ddf DfF
	vCnt	+=sAddTri(pUI, D, d, f);
	vCnt	+=sAddTri(pUI, D, f, F);


	//that is all of the square bits
	//find arc points for the 4 corners
	vec2	points[20];
	vec2	pointsInner[20];

	//ACI on the diagram above, top left
	sMakeCornerArcPoints(A, C, I, seggz, points);
	sMakeCornerArcPoints(a, c, i, seggz, pointsInner);
	vCnt	+=sMakeCornerTrisHull(pUI, seggz, hullSize, A, C, I, a, c, i, points, pointsInner);

	//DBJ, top right
	sMakeCornerArcPoints(D, B, J, seggz, points);
	sMakeCornerArcPoints(d, b, j, seggz, pointsInner);
	vCnt	+=sMakeCornerTrisHull(pUI, seggz, hullSize, D, B, J, d, b, j, points, pointsInner);

	//EGK, bottom left
	sMakeCornerArcPoints(E, G, K, seggz, points);
	sMakeCornerArcPoints(e, g, k, seggz, pointsInner);
	vCnt	+=sMakeCornerTrisHull(pUI, seggz, hullSize, E, G, K, e, g, k, points, pointsInner);

	//HFL, bottom right
	sMakeCornerArcPoints(H, F, L, seggz, points);
	sMakeCornerArcPoints(h, f, l, seggz, pointsInner);
	vCnt	+=sMakeCornerTrisHull(pUI, seggz, hullSize, H, F, L, h, f, l, points, pointsInner);

	vec4	col;
	Misc_SRGBToLinear(color, col);

	//zero out the texture with z of 0
	//set w to 1 to boost to white
	vec4	uv	={	0, 0, 0, 1	};

	//color and UV
	for(int n=0;n < vCnt;n++)
	{
		int	idx	=(pUI->mNumVerts - vCnt) + n;

		Misc_ConvertVec4ToF16(uv, pUI->mpUIBuf[idx].TexCoord0);
		Misc_ConvertVec4ToF16(col, pUI->mpUIBuf[idx].Color);
	}

	assert(pUI->mNumVerts < pUI->mMaxVerts);
}


//Add a triangle to the ui buffer
//returns num verts added
static int	sAddTri(UIStuff *pUI, const vec2 A, const vec2 B, const vec2 C)
{
	glm_vec2_copy(A, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;
	glm_vec2_copy(B, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;
	glm_vec2_copy(C, pUI->mpUIBuf[pUI->mNumVerts].Position);
	pUI->mNumVerts++;

	return	3;
}

//Add a triangle to the ui buffer
//returns num verts added
static int	sAddTriUVC(UIStuff *pUI, const vec2 A, const vec2 B, const vec2 C,
						const vec4 uvA, const vec4 uvB, const vec4 uvC,
						const vec4 color)
{
	glm_vec2_copy(A, pUI->mpUIBuf[pUI->mNumVerts].Position);
	Misc_ConvertVec4ToF16(uvA, pUI->mpUIBuf[pUI->mNumVerts].TexCoord0);
	Misc_ConvertVec4ToF16(color, pUI->mpUIBuf[pUI->mNumVerts].Color);
	pUI->mNumVerts++;
	glm_vec2_copy(B, pUI->mpUIBuf[pUI->mNumVerts].Position);
	Misc_ConvertVec4ToF16(uvB, pUI->mpUIBuf[pUI->mNumVerts].TexCoord0);
	Misc_ConvertVec4ToF16(color, pUI->mpUIBuf[pUI->mNumVerts].Color);
	pUI->mNumVerts++;
	glm_vec2_copy(C, pUI->mpUIBuf[pUI->mNumVerts].Position);
	Misc_ConvertVec4ToF16(uvC, pUI->mpUIBuf[pUI->mNumVerts].TexCoord0);
	Misc_ConvertVec4ToF16(color, pUI->mpUIBuf[pUI->mNumVerts].Color);
	pUI->mNumVerts++;

	return	3;
}

//create points on an arc between right triangle verts A and B
//with C as the 90 degree corner
static void	sMakeCornerArcPoints(const vec2 A, const vec2 B, const vec2 C,
								 int segments, vec2 *pPoints)
{
	vec2	ACRay;
	glm_vec2_sub(A, C, ACRay);

	//why is length named norm?!?
	float	ACLen	=glm_vec2_norm(ACRay);

	//find points between A and B
	vec2	AB;
	glm_vec2_sub(A, B, AB);

	float	ABLen	=glm_vec2_norm(AB);

	//divide into segments
	float	seg	=ABLen / (segments + 1);

	//normalize AB to use to generate  points
	glm_vec2_scale(AB, 1.0f / ABLen, AB);

	//store points along the AB vector
	//scale the AB vec by seg
	vec2	ABSeg, BMarch;
	glm_vec2_scale(AB, seg, ABSeg);
	glm_vec2_copy(B, BMarch);

	for(int i=0;i < segments;i++)
	{
		//advance from B to A
		glm_vec2_add(BMarch, ABSeg, BMarch);

		glm_vec2_copy(BMarch, pPoints[i]);
	}

	//for each point, find a vector to C
	//use this to scale to ACLen
	for(int i=0;i < segments;i++)
	{
		vec2	ray;
		glm_vec2_sub(pPoints[i], C, ray);

		glm_vec2_normalize(ray);

		glm_vec2_scale(ray, ACLen, ray);

		//store ray end back in point
		glm_vec2_add(C, ray, pPoints[i]);
	}
}

//add corner geometry to ui buf from triangle ABC
//where C is the 90 degree corner
//returns num verts added
static int	sMakeCornerTris(UIStuff *pUI, int segments,
							const vec2 A, const vec2 B, const vec2 C,
							const vec2 *pPoints)
{
	int	vCnt	=0;

	//make triangles for the endpoints of the arc
	//tri6 BCPoints[0]
	vCnt	+=sAddTri(pUI, B, C, pPoints[0]);

	//tri7 CAPoints[seggz-1]
	vCnt	+=sAddTri(pUI, C, A, pPoints[segments - 1]);

	//rest of the triangles come from points
	for(int i=1;i < segments;i++)
	{
		vCnt	+=sAddTri(pUI, pPoints[i-1], C, pPoints[i]);
	}
	return	vCnt;
}

//add corner geometry to ui buf from triangle ABC
//where C is the 90 degree corner
//returns num verts added
static int	sMakeCornerTrisHull(UIStuff *pUI, int segments, int hullSize,
								const vec2 A, const vec2 B, const vec2 C,
								const vec2 a, const vec2 b, const vec2 c,
								const vec2 *pPoints, const vec2 *pPointsInner)
{
	int	vCnt	=0;

	//make triangles for the endpoints of the arc
	//B b p0  b p0 pi0
	vCnt	+=sAddTri(pUI, B, b, pPoints[0]);
	vCnt	+=sAddTri(pUI, pPoints[0], b, pPointsInner[0]);

	//A a p  a p pi
	vCnt	+=sAddTri(pUI, a, A, pPoints[segments - 1]);
	vCnt	+=sAddTri(pUI, a, pPoints[segments - 1], pPointsInner[segments - 1]);

	//rest of the triangles come from points
	for(int i=1;i < segments;i++)
	{
		vCnt	+=sAddTri(pUI, pPoints[i-1], pPointsInner[i], pPoints[i]);
		vCnt	+=sAddTri(pUI, pPointsInner[i], pPoints[i - 1], pPointsInner[i - 1]);
	}
	return	vCnt;
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

	//drawing text?
	if(pUI->mpSRV == NULL)
	{
		GD_PSSetSRV(pUI->mpGD, GFont_GetSRV(pUI->mpFont), 0);
	}
	else
	{
		GD_PSSetSRV(pUI->mpGD, pUI->mpSRV, 0);
	}

	GD_PSSetSampler(pUI->mpGD, pUI->mpSS, 0);

	GD_OMSetDepthStencilState(pUI->mpGD, pUI->mpDSS);
	GD_OMSetBlendState(pUI->mpGD, pUI->mpBS);

	GD_Draw(pUI->mpGD, pUI->mNumVerts, 0);
}

static void	sMakeVBDesc(D3D11_BUFFER_DESC *pDesc, uint32_t byteSize)
{
	memset(pDesc, 0, sizeof(D3D11_BUFFER_DESC));

	pDesc->BindFlags			=D3D11_BIND_VERTEX_BUFFER;
	pDesc->ByteWidth			=byteSize;
	pDesc->CPUAccessFlags		=DXGI_CPU_ACCESS_DYNAMIC;
	pDesc->MiscFlags			=0;
	pDesc->StructureByteStride	=0;
	pDesc->Usage				=D3D11_USAGE_DYNAMIC;
}

static void sComputeAtoL(const UIRect r, float roundNess, vec2 A, vec2 B, vec2 C, vec2 D, vec2 E, vec2 F, vec2 G, vec2 H, vec2 I, vec2 J, vec2 K, vec2 L)
{
	//find length of sides
	float	xLen	=r.width;
	float	yLen	=r.height;

	//at full roundness, the shortest side forms
	//the diameter of a circle
	float	rad	=0.5f * ((xLen > yLen)? yLen : xLen);

	//scale rad by roundNess factor
	rad	*=roundNess;

	A[0]	=r.x + rad;				A[1]	=r.y;
	B[0]	=(r.x + r.width) - rad;	B[1]	=r.y;
	C[0]	=r.x;					C[1]	=r.y + rad;
	D[0]	=(r.x + r.width);		D[1]	=r.y + rad;
	E[0]	=r.x;					E[1]	=(r.y + r.height) - rad;
	F[0]	=(r.x + r.width);		F[1]	=(r.y + r.height) - rad;
	G[0]	=r.x + rad;				G[1]	=r.y + r.height;
	H[0]	=(r.x + r.width) - rad;	H[1]	=r.y + r.height;
	I[0]	=r.x + rad;				I[1]	=r.y + rad;
	J[0]	=(r.x + r.width) - rad;	J[1]	=r.y + rad;
	K[0]	=r.x + rad;				K[1]	=(r.y + r.height) - rad;
	L[0]	=(r.x + r.width) - rad;	L[1]	=(r.y + r.height) - rad;
}


//clay stuff!


void	UI_AddFont(UIStuff *pUI, const char *szFontName, uint16_t id)
{
	if(pUI == NULL || pUI->mpSK == NULL || szFontName == NULL)
	{
		return;
	}

	GrogFont	*pFont	=StuffKeeper_GetFont(pUI->mpSK, szFontName);
	if(pFont == NULL)
	{
		printf("Font not found! %s\n", szFontName);
		return;
	}

	int	key	=(int)id;

	UIFonts	*pF;

	//make sure font not already added under a different ID
	for(pF = pUI->mpFonts;pF != NULL;pF=pF->hh.next)
	{
		if(pF->mpFont == pFont)
		{
			printf("Already added font %s!\n", szFontName);
			return;
		}
	}

	//check if id already in the list
	HASH_FIND_INT(pUI->mpFonts, &key, pF);
	if(pF == NULL)
	{
		pF	=malloc(sizeof(UIFonts));
		memset(pF, 0, sizeof(UIFonts));

		pF->id		=key;
		pF->mpFont	=pFont;

		HASH_ADD_INT(pUI->mpFonts, id, pF);
	}
	else
	{
		printf("Already added font %s!\n", szFontName);
	}
}

static GrogFont	*sGetFont(const UIStuff *pUI, uint16_t fontID)
{
	UIFonts	*pF;

	int	key	=(int)fontID;

	HASH_FIND_INT(pUI->mpFonts, &key, pF);
	if(pF == NULL)
	{
		return	NULL;
	}

	return	pF->mpFont;
}

Clay_Dimensions	UI_MeasureText(Clay_StringSlice text,
	Clay_TextElementConfig *pConfig, void *pUserData)
{
	//Measure string size for Font
	Clay_Dimensions	textSize	={ 0 };

	UIStuff	*pUI	=(UIStuff *)pUserData;
	if(pUI == NULL)
	{
		printf("Bad userData in UI_MeasureText!  String is %s\n", text.chars);
		return	textSize;
	}
	
	float	maxTextWidth	=0.0f;
	
	float	textHeight	=pConfig->fontSize;

	GrogFont	*pFontToUse	=sGetFont(pUI, pConfig->fontId);
	if(pFontToUse == NULL)
	{
		printf("Font not found in MeasureText! %u\n", (uint32_t)pConfig->fontId);
		return	textSize;
	}

	maxTextWidth	=GFont_MeasureTextClay(pFontToUse, text.chars, text.length);
	
	//TODO: uiscale?
	float	scaleFactor	=1.0f;
	
	textSize.width	=maxTextWidth * scaleFactor;
	textSize.height	=textHeight;
	
	return	textSize;
}

void	UI_ClayRender(UIStuff *pUI, Clay_RenderCommandArray renderCommands)
{
	for(int j=0;j < renderCommands.length;j++)
	{
		Clay_RenderCommand	*pRC		=Clay_RenderCommandArray_Get(&renderCommands, j);
		Clay_BoundingBox	boundingBox	=pRC->boundingBox;
		
		switch(pRC->commandType)
		{
			case CLAY_RENDER_COMMAND_TYPE_TEXT:
			{
				GrogFont	*pFontToUse	=NULL;

				pFontToUse	=sGetFont(pUI, pRC->renderData.text.fontId);
				
				if(pFontToUse == NULL)
				{
					printf("Font not found! %u\n", (uint32_t)pRC->renderData.text.fontId);
					continue;
				}

				Clay_StringSlice	*pCss	=&pRC->renderData.text.stringContents;

				vec2	pos		={ boundingBox.x, boundingBox.y };
				vec4	colour;
				UI_ClayColorToVec4(pRC->renderData.text.textColor, colour);
				UI_DrawString(pUI, pCss->chars, pCss->length, pFontToUse, pos, colour);
				break;
			}
			
			case CLAY_RENDER_COMMAND_TYPE_IMAGE:
			{
				Clay_ImageRenderData	*pCird	=&pRC->renderData.image;

				vec4	colour;
				UI_ClayColorToVec4(pCird->backgroundColor, colour);

				UI_DrawImage(pUI, pCird->imageData,
					(vec2){ boundingBox.x, boundingBox.y },
					0.0f, colour);
				break;
			}
			case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
			{
				//flush 2D stuff
				UI_EndDraw(pUI);
				UI_BeginDraw(pUI);

				//d3d rects are all coords, not width/height
				D3D11_RECT	r[1]	={{
					(int)roundf(boundingBox.x), (int)roundf(boundingBox.y),
					(int)roundf(boundingBox.width + boundingBox.x),
					(int)roundf(boundingBox.height + boundingBox.y) }};
				GD_RSSetScissorRects(pUI->mpGD, 1, r);
				GD_RSSetState(pUI->mpGD, pUI->mpScissorRast);
				break;
			}
			case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
			{
				//flush 2D stuff
				UI_EndDraw(pUI);
				UI_BeginDraw(pUI);

				GD_RSSetState(pUI->mpGD, pUI->mpNormalRast);
				break;
			}
			case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
			{
				Clay_RectangleRenderData	*pCrrd	=&pRC->renderData.rectangle;
				vec4	color;
				UI_ClayColorToVec4(pCrrd->backgroundColor, color);
				if(pCrrd->cornerRadius.topLeft > 0)
				{
					//TODO: find some formulae for roundness and segments
//					float	radius	=(pCrrd->cornerRadius.topLeft * 2) / (float)((boundingBox.width > boundingBox.height) ? boundingBox.height : boundingBox.width);
					UI_DrawRectRounded(pUI, (UIRect) { boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height }, 0.75f, 3, color);
				}
				else
				{
					UI_DrawRect(pUI, (UIRect) { boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height }, color);
				}
				break;
			}
			case	CLAY_RENDER_COMMAND_TYPE_BORDER:
			{
				//I don't think I'm fully doing all cases here
				//the raylib renderererrer is way more complicated
				Clay_BorderRenderData	*pCbrd	=&pRC->renderData.border;

				vec4	col;
				UI_ClayColorToVec4(pCbrd->color, col);
				if(pCbrd->cornerRadius.topLeft > 0)
				{
					UI_DrawRRHollow(pUI,
						(UIRect){ boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height},
						pCbrd->width.left, 0.5, 3, col);
				}
				else
				{
					UI_DrawRectHollow(pUI,
						(UIRect){ boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height},
						pCbrd->width.left, col);
				}
				break;
			}

			case	CLAY_RENDER_COMMAND_TYPE_NONE:
			{
				printf("CLAY_RENDER_COMMAND_TYPE_NONE?\n");
				break;
			}

			case	CLAY_RENDER_COMMAND_TYPE_CUSTOM:
			{
				printf("CLAY_RENDER_COMMAND_TYPE_CUSTOM?\n");
				break;
			}
			default:
			{
				printf("Error: unhandled render command.");
#ifdef CLAY_OVERFLOW_TRAP
				raise(SIGTRAP);
#endif
				exit(1);
			}
		}
	}
}

//doesn't do any linear/srgb stuff
void	UI_ClayColorToVec4(Clay_Color in, vec4 out)
{
	float	oo255	=1.0f / 255.0f;

	out[0]	=in.r * oo255;
	out[1]	=in.g * oo255;
	out[2]	=in.b * oo255;
	out[3]	=in.a * oo255;
}


//font helper stuff
void	UI_AddAllFonts(UIStuff *pUI)
{
	//check default fonts dir for fonts
	DIR	*pDir	=opendir("Fonts");
	if(pDir == NULL)
	{
		printf("No fonts dir!");
	}
	else
	{
		//lazy
		char	pathBuf[256];
		int		count	=0;
		for(;;)
		{
			struct dirent	*pEnt	=readdir(pDir);

			if(pEnt == NULL)
			{
				break;
			}

			struct stat	fileStuff;

			strncpy(pathBuf, "Fonts", 255);
			strncat(pathBuf, "/", 255);
			strncat(pathBuf, pEnt->d_name, 255);

			int	res	=stat(pathBuf, &fileStuff);
			if(res)
			{
				FileStuff_PrintErrno(errno);
				continue;
			}

			//regular file?
			if(S_ISREG(fileStuff.st_mode))
			{
				UT_string	*pFName	=SZ_StripExtension(pEnt->d_name);

				UI_AddFont(pUI, utstring_body(pFName), count);
				count++;

				utstring_free(pFName);
			}
		}
	}
}

//returns -1 if font not found
int	UI_GetFontSize(const UIStuff *pUI, uint16_t id)
{
	GrogFont	*pFont	=sGetFont(pUI, id);
	if(pFont == NULL)
	{
		return	-1;
	}

	return	GFont_GetCharacterHeight(pFont);
}

uint16_t	UI_GetNearestFontSize(const UIStuff *pUI, int size)
{
	UIFonts	*pF;

	int	bestID		=-1;
	int	bestDiff	=INT32_MAX;
	for(pF = pUI->mpFonts;pF != NULL;pF = pF->hh.next)
	{
		int	h	=GFont_GetCharacterHeight(pF->mpFont);

		int	diff	=abs(size - h);
		if(diff == 0)
		{
			return	pF->id;
		}

		if(diff < bestDiff)
		{
			bestDiff	=diff;
			bestID		=pF->id;
		}
	}

	return	bestID;
}