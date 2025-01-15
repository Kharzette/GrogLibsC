#include	"d3d11.h"
#include	<cglm/call.h>
#include	<assert.h>
#include	"CBKeeper.h"
#include	"StuffKeeper.h"
#include	"../UtilityLib/GraphicsDevice.h"
#include	"../UtilityLib/MiscStuff.h"


//This is a dynamic VB that stores polygonish shapes for drawing.
//Probably will only be used for 2D UI.
//The stuff to draw is remade per frame
typedef struct	ShapeVert_t
{
	vec2		Position;
	uint16_t	Color[4];	//f16 rgba
}	ShapeVert;

typedef struct	GrogShapes_t
{
	ID3D11Buffer			*mpVB;	//dynamic
	ID3D11InputLayout		*mpLayout;
	ID3D11VertexShader		*mpVS;
	ID3D11PixelShader		*mpPS;
	ID3D11DepthStencilState	*mpDSS;
	ID3D11BlendState		*mpBS;
	ID3D11SamplerState		*mpSS;

	int	mMaxVerts, mNumVerts;

	bool	mbDrawStage;	//begin/end

	ShapeVert	*mpShapeBuf;

}	GrogShapes;


//statics
static void	MakeVBDesc(D3D11_BUFFER_DESC *pDesc, uint32_t byteSize);


GrogShapes	*GShapes_Create(GraphicsDevice *pGD, const StuffKeeper *pSK, int maxVerts)
{
	GrogShapes	*pRet	=malloc(sizeof(GrogShapes));
	memset(pRet, 0, sizeof(GrogShapes));

	pRet->mMaxVerts	=maxVerts;
	pRet->mpLayout	=StuffKeeper_GetInputLayout(pSK, "VPos2Col0");
	pRet->mpVS		=StuffKeeper_GetVertexShader(pSK, "ShapeVS");
	pRet->mpPS		=StuffKeeper_GetPixelShader(pSK, "ShapePS");
	pRet->mpDSS		=StuffKeeper_GetDepthStencilState(pSK, "DisableDepth");
	pRet->mpBS		=StuffKeeper_GetBlendState(pSK, "NoBlending");
	pRet->mpSS		=StuffKeeper_GetSamplerState(pSK, "PointClamp");

	//alloc shape buffer
	pRet->mpShapeBuf	=malloc(sizeof(ShapeVert) * maxVerts);

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(ShapeVert) * maxVerts);
	pRet->mpVB	=GD_CreateBuffer(pGD, &bufDesc);

	return	pRet;
}

void	GShapes_FreeAll(GrogShapes *pGS)
{
	pGS->mpVB->lpVtbl->Release(pGS->mpVB);

	free(pGS->mpShapeBuf);

	free(pGS);
}

void	GShapes_BeginDraw(GrogShapes *pGS)
{
	assert(!pGS->mbDrawStage);

	pGS->mNumVerts		=0;
	pGS->mbDrawStage	=true;
}

void	GShapes_EndDraw(GrogShapes *pGS, GraphicsDevice *pGD)
{
	assert(pGS->mbDrawStage);

	pGS->mbDrawStage	=false;

	D3D11_MAPPED_SUBRESOURCE	msr;

	memset(&msr, 0, sizeof(D3D11_MAPPED_SUBRESOURCE));

	GD_MapDiscard(pGD, (ID3D11Resource *)pGS->mpVB, &msr);

	memcpy(msr.pData, pGS->mpShapeBuf, sizeof(ShapeVert) * pGS->mNumVerts);

	GD_UnMap(pGD, (ID3D11Resource *)pGS->mpVB);
}

void	GShapes_Render(GrogShapes *pGS, GraphicsDevice *pGD)
{
	if(pGS == NULL || pGS->mNumVerts <= 0 || pGS->mbDrawStage)
	{
		return;
	}

	if(pGD == NULL)
	{
		return;
	}

	GD_IASetPrimitiveTopology(pGD, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	GD_IASetVertexBuffers(pGD, pGS->mpVB, 16, 0);
	GD_IASetIndexBuffers(pGD, NULL, DXGI_FORMAT_UNKNOWN, 0);
	GD_IASetInputLayout(pGD, pGS->mpLayout);

	GD_VSSetShader(pGD, pGS->mpVS);
	GD_PSSetShader(pGD, pGS->mpPS);

	GD_PSSetSampler(pGD, pGS->mpSS, 0);

	GD_OMSetDepthStencilState(pGD, pGS->mpDSS);
	GD_OMSetBlendState(pGD, pGS->mpBS);

	GD_Draw(pGD, pGS->mNumVerts, 0);
}


void	GShapes_DrawRect(GrogShapes *pGS, float x, float y, float width, float height, vec4 color)
{
	if(pGS == NULL || !pGS->mbDrawStage)
	{
		return;
	}

	//in my noggin, this should be the other way around
	//this seems counterclockwise which should be culled

	//tri 0
	//top left corner
	pGS->mpShapeBuf[pGS->mNumVerts].Position[0]	=x;
	pGS->mpShapeBuf[pGS->mNumVerts].Position[1]	=y;
	Misc_ConvertVec4ToF16(color, pGS->mpShapeBuf[pGS->mNumVerts].Color);
	pGS->mNumVerts++;

	//bottom left corner
	pGS->mpShapeBuf[pGS->mNumVerts].Position[0]	=x;
	pGS->mpShapeBuf[pGS->mNumVerts].Position[1]	=y + height;
	Misc_ConvertVec4ToF16(color, pGS->mpShapeBuf[pGS->mNumVerts].Color);
	pGS->mNumVerts++;

	//top right corner
	pGS->mpShapeBuf[pGS->mNumVerts].Position[0]	=x + width;
	pGS->mpShapeBuf[pGS->mNumVerts].Position[1]	=y;
	Misc_ConvertVec4ToF16(color, pGS->mpShapeBuf[pGS->mNumVerts].Color);
	pGS->mNumVerts++;


	//tri1
	//top right corner
	pGS->mpShapeBuf[pGS->mNumVerts].Position[0]	=x + width;
	pGS->mpShapeBuf[pGS->mNumVerts].Position[1]	=y;
	Misc_ConvertVec4ToF16(color, pGS->mpShapeBuf[pGS->mNumVerts].Color);
	pGS->mNumVerts++;

	//bottom left corner
	pGS->mpShapeBuf[pGS->mNumVerts].Position[0]	=x;
	pGS->mpShapeBuf[pGS->mNumVerts].Position[1]	=y + height;
	Misc_ConvertVec4ToF16(color, pGS->mpShapeBuf[pGS->mNumVerts].Color);
	pGS->mNumVerts++;

	//bottom right corner
	pGS->mpShapeBuf[pGS->mNumVerts].Position[0]	=x + width;
	pGS->mpShapeBuf[pGS->mNumVerts].Position[1]	=y + height;
	Misc_ConvertVec4ToF16(color, pGS->mpShapeBuf[pGS->mNumVerts].Color);
	pGS->mNumVerts++;

	assert(pGS->mNumVerts < pGS->mMaxVerts);
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