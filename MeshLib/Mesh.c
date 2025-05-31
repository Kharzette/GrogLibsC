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
#include	"Helpers.h"


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

	//structuredbuffer srv
	ID3D11ShaderResourceView	*mpSBSRV;

	int	mNumVerts, mNumTriangles, mVertSize;
	
	//extra editor stuff if needed
	void		*mpVertData;
	uint16_t	*mpIndData;
}	Mesh;


Mesh	*Mesh_Create(GraphicsDevice *pGD, const StuffKeeper *pSK,
	const UT_string *pName, void *pVData, const void *pIndData,
	const int vertElems[], int velCount,
	int	numVerts, int numInds, int vertSize)
{
	//vert formats have narrowed down to:
	//VPosNormTex
	//VPosNormTexCol
	//VPosNormTexIdx
	//VPosNormBoneTex
	//VPosNormBoneTexCol
	//VPosNormBoneTexIdx

	Mesh	*pRet	=malloc(sizeof(Mesh));

	memset(pRet, 0, sizeof(Mesh));

	utstring_new(pRet->mpName);
	utstring_printf(pRet->mpName, "%s", utstring_body(pName));

	D3D11_BUFFER_DESC	bufDesc;
	MakeIBDesc(&bufDesc, numInds * sizeof(uint16_t));
	pRet->mpIndexs	=GD_CreateBufferWithData(pGD, &bufDesc, pIndData, bufDesc.ByteWidth);

	pRet->mNumVerts		=numVerts;
	pRet->mNumTriangles	=numInds / 3;
	pRet->mVertSize		=vertSize;

	MakeStructuredBuffer(pGD, vertSize, numVerts, pVData,
		&pRet->mpVerts, &pRet->mpSBSRV);

	//keep vert and ind data since this is an editor path
	pRet->mpVertData	=pVData;

	//index data comes from the bin part of the file that
	//will be freed so make a copy
	pRet->mpIndData	=malloc(sizeof(uint16_t) * numInds);
	memcpy(pRet->mpIndData, pIndData, sizeof(uint16_t) * numInds);

	return	pRet;
}

Mesh	*Mesh_Read(GraphicsDevice *pGD, StuffKeeper *pSK,
					FILE *f, bool bEditor)
{
	assert(f != NULL);

	Mesh	*pMesh	=malloc(sizeof(Mesh));
	memset(pMesh, 0, sizeof(Mesh));
	
	uint32_t	magic;
	fread(&magic, sizeof(uint32_t), 1, f);
	if(magic != 0xb0135313)
	{
		fclose(f);
		free(pMesh);
		printf("Bad magic for Mesh_Read()\n");
		return	NULL;
	}
	
	pMesh->mpName	=SZ_ReadString(f);

	fread(&pMesh->mNumVerts, sizeof(int), 1, f);
	fread(&pMesh->mNumTriangles, sizeof(int), 1, f);
	fread(&pMesh->mVertSize, sizeof(int), 1, f);

	void	*pVerts	=malloc(pMesh->mVertSize * pMesh->mNumVerts);

	fread(pVerts, pMesh->mVertSize * pMesh->mNumVerts, 1, f);

	MakeStructuredBuffer(pGD, pMesh->mVertSize, pMesh->mNumVerts,
		pVerts, &pMesh->mpVerts, &pMesh->mpSBSRV);

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

		D3D11_BUFFER_DESC	bufDesc;
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

	return	pMesh;
}

Mesh	*Mesh_ReadFromFile(GraphicsDevice *pGD, StuffKeeper *pSK,
	const char *szFileName, bool bEditor)
{
	FILE	*f	=fopen(szFileName, "rb");
	if(f == NULL)
	{
		printf("Couldn't open file %s\n", szFileName);
		return	NULL;
	}

	Mesh	*pMesh	=Mesh_Read(pGD, pSK, f, bEditor);

	fclose(f);

	return	pMesh;
}

void	Mesh_Write(const Mesh *pMesh, FILE *f)
{
	assert(f != NULL);

	uint32_t	magic	=0xb0135313;
	fwrite(&magic, sizeof(uint32_t), 1, f);

	SZ_WriteString(f, pMesh->mpName);

	fwrite(&pMesh->mNumVerts, sizeof(int), 1, f);
	fwrite(&pMesh->mNumTriangles, sizeof(int), 1, f);
	fwrite(&pMesh->mVertSize, sizeof(int), 1, f);

	fwrite(pMesh->mpVertData, pMesh->mVertSize, pMesh->mNumVerts, f);

	bool	bIndexed	=(pMesh->mpIndData != NULL);
	fwrite(&bIndexed, sizeof(bool), 1, f);

	if(bIndexed)
	{
		int	numIndexes	=pMesh->mNumTriangles * 3;
		fwrite(&numIndexes, sizeof(int), 1, f);
		fwrite(pMesh->mpIndData, sizeof(uint16_t), numIndexes, f);
	}
}

void	Mesh_WriteToFile(const Mesh *pMesh, const char *szFileName)
{
	//make sure this has editor stuff
	assert(pMesh->mpVertData);

	FILE	*f	=fopen(szFileName, "wb");

	Mesh_Write(pMesh, f);

	fclose(f);
}


const UT_string	*Mesh_GetName(const Mesh *pMesh)
{
	return	pMesh->mpName;
}

//an early draw effort, later will have material lib
void	Mesh_Draw(Mesh *pMesh, GraphicsDevice *pGD, StuffKeeper *pSK,
	const char *szVS, const char *szPS, const char *szTex)
{
	GD_VSSetShader(pGD, StuffKeeper_GetVertexShader(pSK, szVS));
	GD_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, szPS));

	GD_VSSetSRV(pGD, pMesh->mpSBSRV, 0);
	GD_IASetIndexBuffers(pGD, pMesh->mpIndexs, DXGI_FORMAT_R16_UINT, 0);

	GD_PSSetSRV(pGD, StuffKeeper_GetSRV(pSK, szTex), 0);

	GD_DrawIndexed(pGD, pMesh->mNumTriangles * 3, 0, 0);
}

//Draw with Material
void	Mesh_DrawMat(const Mesh *pMesh, GraphicsDevice *pGD,
					CBKeeper *pCBK, const Material *pMat)
{
	GD_VSSetSRV(pGD, pMesh->mpSBSRV, 0);
	GD_IASetIndexBuffers(pGD, pMesh->mpIndexs, DXGI_FORMAT_R16_UINT, 0);

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
	pMesh->mpSBSRV->lpVtbl->Release(pMesh->mpSBSRV);
	pMesh->mpVerts->lpVtbl->Release(pMesh->mpVerts);
	pMesh->mpIndexs->lpVtbl->Release(pMesh->mpIndexs);

	utstring_done(pMesh->mpName);

	//free extra editor data if needed
	if(pMesh->mpVertData != NULL)
	{
		free(pMesh->mpVertData);
	}
	if(pMesh->mpIndData != NULL)
	{
		free(pMesh->mpIndData);
	}

	free(pMesh);
}