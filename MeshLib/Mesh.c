#include	<stdint.h>
#include	<utstring.h>
#include	<d3d11.h>
#include	<stdio.h>
#include	<string.h>
#include	<assert.h>
#include	"../UtilityLib/FileStuff.h"
#include	"../UtilityLib/StringStuff.h"
#include	"../UtilityLib/GraphicsDevice.h"
#include	"../MaterialLib/StuffKeeper.h"
#include	"../MaterialLib/Material.h"


//scale factors, collada always in meterish scale
#define	MetersToQuakeUnits	37.6471f
#define	MetersToValveUnits	39.37001f
#define	MetersToGrogUnits	MetersToValveUnits
#define	MetersToCentiMeters	100f

//contains the gpu specific stuff for mesh rendering
//might be part of a static or a character
typedef struct	Mesh_t
{
	UT_string			*mpName;
	ID3D11Buffer		*mpVerts, *mpIndexs;
	ID3D11InputLayout	*mpLayout;
	int					mNumVerts, mNumTriangles, mVertSize;
	int					mTypeIndex;

	//extra editor stuff if needed
	void		*mpVertData;
	uint16_t	*mpIndData;
}	Mesh;

//C# stored types, but we just need the size
//dumped from MeshDumpTypes
size_t	sTypeSizes[110]	={
	12, 20, 24, 16, 20, 24, 28, 16, 20, 24, 
	28, 20, 24, 28, 32, 24, 28, 32, 36, 28, 
	32, 36, 40, 32, 36, 40, 44, 28, 32, 36, 
	40, 28, 32, 36, 40, 32, 36, 40, 44, 36, 
	40, 44, 48, 40, 44, 48, 52, 44, 48, 52, 
	56, 24, 28, 32, 36, 24, 28, 32, 36, 28, 
	32, 36, 40, 32, 36, 40, 44, 36, 40, 44, 
	48, 40, 44, 48, 52, 36, 40, 44, 48, 36, 
	40, 44, 48, 40, 44, 48, 52, 44, 48, 52, 
	56, 48, 52, 56, 60, 52, 56, 60, 64, 32, 
	44, 32, 40, 28, 32, 48, 48, 44, 40, 80	};

const char *sTypeNames[110]	={
	"VPos", "VPosNorm", "VPosBone", "VPosTex0", "VPosTex0Tex1", 
	"VPosTex0Tex1Tex2", "VPosTex0Tex1Tex2Tex3", "VPosCol0", "VPosCol0Col1", "VPosCol0Col1Col2", 
	"VPosCol0Col1Col2Col3", "VPosTex0Col0", "VPosTex0Col0Col1", "VPosTex0Col0Col1Col2", "VPosTex0Col0Col1Col2Col3", 
	"VPosTex0Tex1Col0", "VPosTex0Tex1Col0Col1", "VPosTex0Tex1Col0Col1Col2", "VPosTex0Tex1Col0Col1Col2Col3", "VPosTex0Tex1Tex2Col0", 
	"VPosTex0Tex1Tex2Col0Col1", "VPosTex0Tex1Tex2Col0Col1Col2", "VPosTex0Tex1Tex2Col0Col1Col2Col3", "VPosTex0Tex1Tex2Tex3Col0", "VPosTex0Tex1Tex2Tex3Col0Col1", 
	"VPosTex0Tex1Tex2Tex3Col0Col1Col2", "VPosTex0Tex1Tex2Tex3Col0Col1Col2Col3", "VPosBoneTex0", "VPosBoneTex0Tex1", "VPosBoneTex0Tex1Tex2", 
	"VPosBoneTex0Tex1Tex2Tex3", "VPosBoneCol0", "VPosBoneCol0Col1", "VPosBoneCol0Col1Col2", "VPosBoneCol0Col1Col2Col3", 
	"VPosBoneTex0Col0", "VPosBoneTex0Col0Col1", "VPosBoneTex0Col0Col1Col2", "VPosBoneTex0Col0Col1Col2Col3", "VPosBoneTex0Tex1Col0", 
	"VPosBoneTex0Tex1Col0Col1", "VPosBoneTex0Tex1Col0Col1Col2", "VPosBoneTex0Tex1Col0Col1Col2Col3", "VPosBoneTex0Tex1Tex2Col0", "VPosBoneTex0Tex1Tex2Col0Col1", 
	"VPosBoneTex0Tex1Tex2Col0Col1Col2", "VPosBoneTex0Tex1Tex2Col0Col1Col2Col3", "VPosBoneTex0Tex1Tex2Tex3Col0", "VPosBoneTex0Tex1Tex2Tex3Col0Col1", "VPosBoneTex0Tex1Tex2Tex3Col0Col1Col2", 
	"VPosBoneTex0Tex1Tex2Tex3Col0Col1Col2Col3", "VPosNormTex0", "VPosNormTex0Tex1", "VPosNormTex0Tex1Tex2", "VPosNormTex0Tex1Tex2Tex3", 
	"VPosNormCol0", "VPosNormCol0Col1", "VPosNormCol0Col1Col2", "VPosNormCol0Col1Col2Col3", "VPosNormTex0Col0", 
	"VPosNormTex0Col0Col1", "VPosNormTex0Col0Col1Col2", "VPosNormTex0Col0Col1Col2Col3", "VPosNormTex0Tex1Col0", "VPosNormTex0Tex1Col0Col1", 
	"VPosNormTex0Tex1Col0Col1Col2", "VPosNormTex0Tex1Col0Col1Col2Col3", "VPosNormTex0Tex1Tex2Col0", "VPosNormTex0Tex1Tex2Col0Col1", "VPosNormTex0Tex1Tex2Col0Col1Col2", 
	"VPosNormTex0Tex1Tex2Col0Col1Col2Col3", "VPosNormTex0Tex1Tex2Tex3Col0", "VPosNormTex0Tex1Tex2Tex3Col0Col1", "VPosNormTex0Tex1Tex2Tex3Col0Col1Col2", "VPosNormTex0Tex1Tex2Tex3Col0Col1Col2Col3", 
	"VPosNormBoneTex0", "VPosNormBoneTex0Tex1", "VPosNormBoneTex0Tex1Tex2", "VPosNormBoneTex0Tex1Tex2Tex3", "VPosNormBoneCol0", 
	"VPosNormBoneCol0Col1", "VPosNormBoneCol0Col1Col2", "VPosNormBoneCol0Col1Col2Col3", "VPosNormBoneTex0Col0", "VPosNormBoneTex0Col0Col1", 
	"VPosNormBoneTex0Col0Col1Col2", "VPosNormBoneTex0Col0Col1Col2Col3", "VPosNormBoneTex0Tex1Col0", "VPosNormBoneTex0Tex1Col0Col1", "VPosNormBoneTex0Tex1Col0Col1Col2", 
	"VPosNormBoneTex0Tex1Col0Col1Col2Col3", "VPosNormBoneTex0Tex1Tex2Col0", "VPosNormBoneTex0Tex1Tex2Col0Col1", "VPosNormBoneTex0Tex1Tex2Col0Col1Col2", "VPosNormBoneTex0Tex1Tex2Col0Col1Col2Col3", 
	"VPosNormBoneTex0Tex1Tex2Tex3Col0", "VPosNormBoneTex0Tex1Tex2Tex3Col0Col1", "VPosNormBoneTex0Tex1Tex2Tex3Col0Col1Col2", "VPosNormBoneTex0Tex1Tex2Tex3Col0Col1Col2Col3", "VPosNormBone", 
	"VPosNormBlendTex0Tex1Tex2Tex3Tex4", "VPosNormTanTex0", "VPosNormTanBiTanTex0", "VPosNormTex04", "VPosNormTex04Col0", 
	"VPosNormTex04Tex14Tex24Color0", "VPosNormBoneTanTex0Col0", "VPosNormTex04F", "VPosNormTex0Col0F", "VPosNormTex04Tex14Tex24Color0F"
};

static void	MakeVBDesc(D3D11_BUFFER_DESC *pDesc, uint32_t byteSize)
{
	memset(pDesc, 0, sizeof(D3D11_BUFFER_DESC));

	pDesc->BindFlags			=D3D11_BIND_VERTEX_BUFFER;
	pDesc->ByteWidth			=byteSize;
	pDesc->CPUAccessFlags		=DXGI_CPU_ACCESS_NONE;
	pDesc->MiscFlags			=0;
	pDesc->StructureByteStride	=0;
	pDesc->Usage				=D3D11_USAGE_IMMUTABLE;
}

static void	MakeIBDesc(D3D11_BUFFER_DESC *pDesc, uint32_t byteSize)
{
	memset(pDesc, 0, sizeof(D3D11_BUFFER_DESC));

	pDesc->BindFlags			=D3D11_BIND_INDEX_BUFFER;
	pDesc->ByteWidth			=byteSize;
	pDesc->CPUAccessFlags		=DXGI_CPU_ACCESS_NONE;
	pDesc->MiscFlags			=0;
	pDesc->StructureByteStride	=0;
	pDesc->Usage				=D3D11_USAGE_IMMUTABLE;
}


Mesh	*Mesh_Read(GraphicsDevice *pGD, StuffKeeper *pSK,
					const char *szFileName, bool bEditor)
{
	Mesh	*pMesh	=malloc(sizeof(Mesh));
	memset(pMesh, 0, sizeof(Mesh));

	FILE	*f	=fopen(szFileName, "rb");
	if(f == NULL)
	{
		free(pMesh);
		return	NULL;
	}

	uint32_t	magic;
	fread(&magic, sizeof(uint32_t), 1, f);
	if(magic != 0xb0135313)
	{
		fclose(f);
		free(pMesh);
		return	NULL;
	}

	pMesh->mpName	=SZ_ReadString(f);

	fread(&pMesh->mNumVerts, sizeof(int), 1, f);
	fread(&pMesh->mNumTriangles, sizeof(int), 1, f);
	fread(&pMesh->mVertSize, sizeof(int), 1, f);
	fread(&pMesh->mTypeIndex, sizeof(int), 1, f);

	size_t	vertSize	=sTypeSizes[pMesh->mTypeIndex];

	pMesh->mpLayout	=StuffKeeper_GetInputLayout(pSK, sTypeNames[pMesh->mTypeIndex]);

	//add a ref
	pMesh->mpLayout->lpVtbl->AddRef(pMesh->mpLayout);

	assert(vertSize == pMesh->mVertSize);

	int	arrTypeIdx, arrNumVerts;

	fread(&arrTypeIdx, sizeof(int), 1, f);
	fread(&arrNumVerts, sizeof(int), 1, f);

	assert(arrNumVerts == pMesh->mNumVerts);

	void	*pVerts	=malloc(vertSize * arrNumVerts);

	fread(pVerts, vertSize * arrNumVerts, 1, f);

	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, vertSize * arrNumVerts);
	pMesh->mpVerts	=GD_CreateBufferWithData(pGD, &bufDesc, pVerts, bufDesc.ByteWidth);

	//read indexes
	uint16_t	*pInds	=NULL;
	bool		bNull;
	fread(&bNull, sizeof(bool), 1, f);
	if(bNull)
	{
		int	numIdx;
		fread(&numIdx, sizeof(int), 1, f);
		pInds	=malloc(numIdx * sizeof(uint16_t));

		fread(pInds, numIdx * sizeof(uint16_t), 1, f);

		MakeIBDesc(&bufDesc, numIdx * sizeof(uint16_t));
		pMesh->mpIndexs	=GD_CreateBufferWithData(pGD, &bufDesc, pInds, bufDesc.ByteWidth);
	}

	if(bEditor)
	{
		pMesh->mpVertData	=pVerts;
		pMesh->mpIndData	=pInds;
	}
	else
	{
		free(pVerts);
		free(pInds);
	}

	fclose(f);

	return	pMesh;
}

void	Mesh_Write(const Mesh *pMesh, const char *szFileName)
{
	//make sure this has editor stuff
	assert(pMesh->mpVertData);

	FILE	*f	=fopen(szFileName, "wb");

	uint32_t	magic	=0xb0135313;
	fwrite(&magic, sizeof(uint32_t), 1, f);

	SZ_WriteString(f, pMesh->mpName);

	fwrite(&pMesh->mNumVerts, sizeof(int), 1, f);
	fwrite(&pMesh->mNumTriangles, sizeof(int), 1, f);
	fwrite(&pMesh->mVertSize, sizeof(int), 1, f);
	fwrite(&pMesh->mTypeIndex, sizeof(int), 1, f);

	//this is written twice, weird c# stuff
	fwrite(&pMesh->mTypeIndex, sizeof(int), 1, f);
	fwrite(&pMesh->mNumVerts, sizeof(int), 1, f);

	fwrite(pMesh->mpVertData, sTypeSizes[pMesh->mTypeIndex], pMesh->mNumVerts, f);

	bool	bIndexed	=(pMesh->mpIndData != NULL);
	fwrite(&bIndexed, sizeof(bool), 1, f);

	if(bIndexed)
	{
		int	numIndexes	=pMesh->mNumTriangles * 3;
		fwrite(&numIndexes, sizeof(int), 1, f);
		fwrite(pMesh->mpIndData, sizeof(uint16_t), numIndexes, f);
	}

	fclose(f);
}

//an early draw effort, later will have material lib
void	Mesh_Draw(Mesh *pMesh, GraphicsDevice *pGD, StuffKeeper *pSK,
	const char *szVS, const char *szPS, const char *szTex)
{
	GD_VSSetShader(pGD, StuffKeeper_GetVertexShader(pSK, szVS));
	GD_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, szPS));

	GD_IASetVertexBuffers(pGD, pMesh->mpVerts, pMesh->mVertSize, 0);
	GD_IASetIndexBuffers(pGD, pMesh->mpIndexs, DXGI_FORMAT_R16_UINT, 0);

	GD_IASetInputLayout(pGD, pMesh->mpLayout);

	GD_PSSetSRV(pGD, StuffKeeper_GetSRV(pSK, szTex), 0);

	GD_DrawIndexed(pGD, pMesh->mNumTriangles * 3, 0, 0);
}

//Draw with Material
void	Mesh_DrawMat(const Mesh *pMesh, GraphicsDevice *pGD,
					CBKeeper *pCBK, const Material *pMat)
{
	GD_IASetVertexBuffers(pGD, pMesh->mpVerts, pMesh->mVertSize, 0);
	GD_IASetIndexBuffers(pGD, pMesh->mpIndexs, DXGI_FORMAT_R16_UINT, 0);
	GD_IASetInputLayout(pGD, pMesh->mpLayout);

	MAT_Apply(pMat, pCBK, pGD);

	GD_DrawIndexed(pGD, pMesh->mNumTriangles * 3, 0, 0);
}

void	Mesh_SetName(Mesh *pMesh, const char *szNew)
{
	utstring_clear(pMesh->mpName);
	utstring_printf(pMesh->mpName, "%s", szNew);
}

void	Mesh_Destroy(Mesh *pMesh, GraphicsDevice *pGD)
{
	pMesh->mpVerts->lpVtbl->Release(pMesh->mpVerts);
	pMesh->mpIndexs->lpVtbl->Release(pMesh->mpIndexs);

	utstring_done(pMesh->mpName);

	free(pMesh);
}