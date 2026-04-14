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
#include	"../MeshLib/Helpers.h"


//a fullscreen CGA like mode, or like a c64 basic screen
typedef struct	Screen_t
{
	//fullscreen quad
	ID3D11Buffer	*mpQuadVB;
	ID3D11Buffer	*mpQuadIB;

	//structuredbuffer and srv
	ID3D11Buffer				*mpSB;
	ID3D11ShaderResourceView	*mpSBSRV;

	//screen text contents
	ID3D11Texture1D				*mpScreenContents;
	ID3D11Resource				*mpSCResource;
	ID3D11ShaderResourceView	*mpScreenContentsSRV;
	ID3D11ShaderResourceView	*mpFontSRV;

	ID3D11VertexShader	*mpVS;
	ID3D11PixelShader	*mpPS;

	//resolution of the screen rendertarget
	int	mResX, mResY;

	//font info
	int	mNumColumns;
	int	mCharWidth, mCharHeight;
	int	mScreenWidth, mScreenHeight;
	int	mStartChar;

	vec4	mClearColor;

}	Screen;


static void sMakeQuad(Screen *pScr, GraphicsDevice *pGD);


Screen	*Screen_Create(GraphicsDevice *pGD, StuffKeeper *pSK,
						CBKeeper *pCBK, GrogFont *pGF,
						int targWidth, int targHeight,
						int screenWidth, int screenHeight)
{
	Screen	*pRet	=malloc(sizeof(Screen));
	memset(pRet, 0, sizeof(Screen));

	//c64 dark blue
	pRet->mClearColor[0]	=0x35 / 255.0f;
	pRet->mClearColor[1]	=0x28 / 255.0f;
	pRet->mClearColor[2]	=0x79 / 255.0f;
	pRet->mClearColor[3]	=1.0f;

	pRet->mpFontSRV	=GFont_GetSRV(pGF);
	pRet->mpFontSRV->lpVtbl->AddRef(pRet->mpFontSRV);

	pRet->mResX			=targWidth;
	pRet->mResY			=targHeight;
	pRet->mScreenWidth	=screenWidth;
	pRet->mScreenHeight	=screenHeight;

	pRet->mCharWidth	=GFont_GetCharacterWidth(pGF, 'X');
	pRet->mCharHeight	=GFont_GetCharacterHeight(pGF);
	pRet->mStartChar	=GFont_GetStartChar(pGF);
	pRet->mNumColumns	=GFont_GetNumColumns(pGF);

	D3D11_TEXTURE1D_DESC	tdesc;

	tdesc.ArraySize			=1;
	tdesc.BindFlags			=D3D11_BIND_SHADER_RESOURCE;
	tdesc.CPUAccessFlags	=D3D11_CPU_ACCESS_WRITE;
	tdesc.Format			=DXGI_FORMAT_R8_UINT;
	tdesc.MipLevels			=1;
	tdesc.MiscFlags			=0;
	tdesc.Usage				=D3D11_USAGE_DYNAMIC;
	tdesc.Width				=screenWidth * screenHeight;

	pRet->mpScreenContents	=GD_CreateTexture1D(pGD, &tdesc);

	HRESULT	hres	=pRet->mpScreenContents->lpVtbl->QueryInterface(
		pRet->mpScreenContents, &IID_ID3D11Resource, (void **)&pRet->mpSCResource);
	
	D3D11_SHADER_RESOURCE_VIEW_DESC	srvDesc;
	memset(&srvDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

	srvDesc.Format			=DXGI_FORMAT_R8_UINT;
	srvDesc.ViewDimension	=D3D11_SRV_DIMENSION_TEXTURE1D;

	srvDesc.Texture1D.MipLevels			=1;
	srvDesc.Texture1D.MostDetailedMip	=0;

	pRet->mpScreenContentsSRV	=GD_CreateSRV(pGD, pRet->mpSCResource, &srvDesc);

	CBK_SetTextModeScreenSize(pCBK, targWidth, targHeight, screenWidth, screenHeight);
	CBK_SetTextModeFontInfo(pCBK, pRet->mStartChar, pRet->mNumColumns, pRet->mCharWidth, pRet->mCharHeight);

	sMakeQuad(pRet, pGD);

	pRet->mpVS	=StuffKeeper_GetVertexShader(pSK, "SimpleVS");
	pRet->mpPS	=StuffKeeper_GetPixelShader(pSK, "TextModePS");

	//add a ref
	pRet->mpVS->lpVtbl->AddRef(pRet->mpVS);
	pRet->mpPS->lpVtbl->AddRef(pRet->mpPS);

	return	pRet;
}

void	Screen_Destroy(Screen **ppScr)
{
	Screen	*pScr	=*ppScr;

	pScr->mpQuadIB->lpVtbl->Release(pScr->mpQuadIB);
	pScr->mpQuadVB->lpVtbl->Release(pScr->mpQuadVB);

	pScr->mpScreenContentsSRV->lpVtbl->Release(pScr->mpScreenContentsSRV);
	pScr->mpSCResource->lpVtbl->Release(pScr->mpSCResource);
	pScr->mpScreenContents->lpVtbl->Release(pScr->mpScreenContents);

	pScr->mpFontSRV->lpVtbl->Release(pScr->mpFontSRV);

	pScr->mpVS->lpVtbl->Release(pScr->mpVS);
	pScr->mpPS->lpVtbl->Release(pScr->mpPS);

	free(pScr);

	*ppScr	=NULL;
}


void	Screen_SetContents(Screen *pScr, GraphicsDevice *pGD,
							CBKeeper *pCBK, const uint8_t *pContents)
{
	GD_WriteToDynResource(pGD, pScr->mpSCResource, pContents,
			pScr->mScreenWidth * pScr->mScreenHeight);
	
	CBK_UpdateTextMode(pCBK, pGD);
	CBK_SetTextModeToShaders(pCBK, pGD);
}


void	Screen_Draw(Screen *pScr, GraphicsDevice *pGD)
{
	GD_IASetPrimitiveTopology(pGD, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	GD_VSSetShader(pGD, pScr->mpVS);
	GD_PSSetShader(pGD, pScr->mpPS);

	//not using an index buffer
	GD_IASetIndexBuffers(pGD, NULL, DXGI_FORMAT_UNKNOWN, 0);

	//view for the structured buffer
	GD_VSSetSRV(pGD, pScr->mpSBSRV, 0);

	GD_PSSetSRV(pGD, pScr->mpScreenContentsSRV, 0);
	GD_PSSetSRV(pGD, pScr->mpFontSRV, 1);

	GD_Draw(pGD, 6, 0);
}


static void sMakeQuad(Screen *pScr, GraphicsDevice *pGD)
{
	float	halfX	=pScr->mResX * 0.5f;
	float	halfY	=pScr->mResY * 0.5f;

	vec3	verts[6]	={
		{	-halfX, -halfY, 0.9f	},
		{	halfX, -halfY, 0.9f	},
		{	-halfX, halfY, 0.9f	},
		{	-halfX, halfY, 0.9f	},
		{	halfX, -halfY, 0.9f	},
		{	halfX, halfY, 0.9f	}
	};

	MakeStructuredBuffer(pGD, sizeof(vec3), 6, verts, &pScr->mpSB, &pScr->mpSBSRV);
}