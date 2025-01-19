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

	//find length of sides
	float	xLen	=r.width;
	float	yLen	=r.height;

	//at full roundness, the shortest side forms
	//the diameter of a circle
	float	rad	=0.5f * ((xLen > yLen)? yLen : xLen);

	//scale rad by roundNess factor
	rad	*=roundNess;

	//I started out testing a wider rect, but tests show this works on
	//tall and even perfect square rects
	vec2	A	={	r.x + rad,				r.y						};
	vec2	B	={	(r.x + r.width) - rad,	r.y						};
	vec2	C	={	r.x,					r.y + rad				};
	vec2	D	={	(r.x + r.width),		r.y + rad				};
	vec2	E	={	r.x,					(r.y + r.height) - rad	};
	vec2	F	={	(r.x + r.width),		(r.y + r.height) - rad	};
	vec2	G	={	r.x + rad,				r.y + r.height			};
	vec2	H	={	(r.x + r.width) - rad,	r.y + r.height			};
	vec2	I	={	r.x + rad,				r.y + rad				};
	vec2	J	={	(r.x + r.width) - rad,	r.y + rad				};
	vec2	K	={	r.x + rad,				(r.y + r.height) - rad	};
	vec2	L	={	(r.x + r.width) - rad,	(r.y + r.height) - rad	};


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