#include	<stdint.h>
#include	<utstring.h>
#include	<d3d11.h>
#include	<stdio.h>
#include	<string.h>
#include	<assert.h>
#include	"../UtilityLib/FileStuff.h"
#include	"../UtilityLib/MiscStuff.h"
#include	"../UtilityLib/GraphicsDevice.h"
#include	"../MaterialLib/StuffKeeper.h"


typedef struct	Terrain_t
{
	UT_string			*mpName;
	ID3D11Buffer		*mpVerts, *mpIndexs;
	ID3D11InputLayout	*mpLayout;
	int					mNumVerts, mNumTriangles, mVertSize;
}	Terrain;

typedef struct	TerrainVert_t
{
	vec3		mPosition;
	uint16_t	mNormal[4];		//16 bit float4

	//these are percentages of each texture in the 8 way atlas
	uint16_t	mTexFactor0[4];	//16 bit float4
	uint16_t	mTexFactor1[4];	//16 bit float4

}	TerrainVert;


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

static void	SmoothPass(BYTE **pRows, int w, int h, int rowPitch)
{
	int	bytesPerHeight	=rowPitch / w;

	for(int y=0;y < h;y++)
	{
		for(int x=0;x < w;x++)
		{
			int	ofsX	=(x * bytesPerHeight);

			float	upLeft, up, upRight;
			float	left, right;
			float	downLeft, down, downRight;

			if(y > 0)
			{
				if(x > 0)
				{
					upLeft	=pRows[y - 1][ofsX - bytesPerHeight];
				}
				else
				{
					upLeft	=pRows[y - 1][ofsX];
				}
				if(x < (w - 1))
				{
					upRight	=pRows[y - 1][ofsX + bytesPerHeight];
				}
				else
				{
					upRight	=pRows[y - 1][ofsX];
				}
				up	=pRows[y - 1][ofsX];
			}
			else
			{
				if(x > 0)
				{
					upLeft	=pRows[y][ofsX - bytesPerHeight];
				}
				else
				{
					upLeft	=pRows[y][ofsX];
				}
				if(x < (w - 1))
				{
					upRight	=pRows[y][ofsX + bytesPerHeight];
				}
				else
				{
					upRight	=pRows[y][ofsX];
				}
				up	=pRows[y][ofsX];
			}

			if(x > 0)
			{
				left	=pRows[y][ofsX - bytesPerHeight];
			}
			else
			{
				left	=pRows[y][ofsX];
			}

			if(x < (w - 1))
			{
				right	=pRows[y][ofsX + bytesPerHeight];
			}
			else
			{
				right	=pRows[y][ofsX];
			}

			if(y < (h - 1))
			{
				if(x > 0)
				{
					downLeft	=pRows[y + 1][ofsX - bytesPerHeight];
				}
				else
				{
					downLeft	=pRows[y + 1][ofsX];
				}

				if(x < (w - 1))
				{
					downRight	=pRows[y + 1][ofsX + bytesPerHeight];
				}
				else
				{
					downRight	=pRows[y + 1][ofsX];
				}

				down	=pRows[y + 1][ofsX];
			}
			else
			{
				if(x > 0)
				{
					downLeft	=pRows[y][ofsX - bytesPerHeight];
				}
				else
				{
					downLeft	=pRows[y][ofsX];
				}

				if(x < (w - 1))
				{
					downRight	=pRows[y][ofsX + bytesPerHeight];
				}
				else
				{
					downRight	=pRows[y][ofsX];
				}

				down	=pRows[y][ofsX];
			}

			float	sum	=upLeft + up + upRight + left
				+ right + downLeft + down + downRight;

			sum	/=8.0f;

			pRows[y][ofsX]	=sum;
		}
	}
}


Terrain	*Terrain_Create(GraphicsDevice *pGD,
	const char *pName, const char *pPath, int numSmoothPasses)
{
	uint32_t	w, h, wm1, hm1;
	int			rowPitch, i;

	Terrain	*pRet	=malloc(sizeof(Terrain));

	memset(pRet, 0, sizeof(Terrain));

	BYTE	**pRows	=SK_LoadTextureBytes(pPath, &rowPitch, &w, &h);

	//smooth a bit
	for(i=0;i < numSmoothPasses;i++)
	{
		SmoothPass(pRows, w, h, rowPitch);
	}

	//-1 for anding
	wm1	=w - 1;
	hm1	=h - 1;

	int	numQuads	=wm1 * hm1;
	int	numTris		=numQuads * 2;

	int	bytesPerHeight	=rowPitch / w;

	TerrainVert	*pVerts	=malloc(sizeof(TerrainVert) * w * h);

	for(int y=0;y < h;y++)
	{
		for(int x=0;x < w;x++)
		{
			int	ofsX	=(x * bytesPerHeight);
			int	idx		=(y * w) + x;

			pVerts[idx].mPosition[0]	=x;
			pVerts[idx].mPosition[1]	=pRows[y][ofsX];	//red is fine
			pVerts[idx].mPosition[2]	=y;

			//set texture 0 to fully on by default
			Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 0.0f, pVerts[idx].mTexFactor0);
			Misc_Convert4ToF16(0.0f, 0.0f, 0.0f, 0.0f, pVerts[idx].mTexFactor1);
		}
	}

	//indexes
	uint16_t	*pIndexes	=malloc(2 * numTris * 3);

	int	curIdx	=0;
	for(int y=0;y < hm1;y++)
	{
		for(int x=0;x < wm1;x++)
		{
			int	idx		=(y * w) + x;

			//tri 0
			pIndexes[curIdx++]	=idx + w;
			pIndexes[curIdx++]	=idx + 1;
			pIndexes[curIdx++]	=idx;

			//tri1
			pIndexes[curIdx++]	=idx + w;
			pIndexes[curIdx++]	=idx + 1 + w;
			pIndexes[curIdx++]	=idx + 1;
		}
	}

	//free data
	for(int y=0;y < h;y++)
	{
		free(pRows[y]);
	}
	free(pRows);

	//compute normals
	for(int y=0;y < h;y++)
	{
		for(int x=0;x < w;x++)
		{
			//grab edge vectors connected to this vert
			vec3	edge0, edge1, edge2, edge3;
			int		leftIdx, upIdx, rightIdx, downIdx;

			int	idx	=(y * w) + x;

			//indexes of the nearby wrapped if needed
			leftIdx		=(y * w) + ((x - 1) & wm1);
			upIdx		=( ((y - 1) & hm1) * w) + x;
			rightIdx	=(y * w) + ((x + 1) & wm1);
			downIdx		=( ((y + 1) & hm1) * w) + x;

			glmc_vec3_sub(pVerts[leftIdx].mPosition, pVerts[idx].mPosition, edge0);
			glmc_vec3_sub(pVerts[upIdx].mPosition, pVerts[idx].mPosition, edge1);
			glmc_vec3_sub(pVerts[rightIdx].mPosition, pVerts[idx].mPosition, edge2);
			glmc_vec3_sub(pVerts[downIdx].mPosition, pVerts[idx].mPosition, edge3);

			vec3	n0, n1, n2, n3;

			glmc_vec3_cross(edge0, edge1, n0);
			glmc_vec3_cross(edge1, edge2, n1);
			glmc_vec3_cross(edge2, edge3, n2);
			glmc_vec3_cross(edge3, edge0, n3);

			//accumulate
			glmc_vec3_add(n0, n1, n0);
			glmc_vec3_add(n0, n2, n0);
			glmc_vec3_add(n0, n3, n0);

			glmc_vec3_normalize(n0);

			Misc_Convert4ToF16(n0[0], n0[1], n0[2], 1.0f, pVerts[idx].mNormal);
		}
	}

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(TerrainVert) * w * h);

	pRet->mpVerts	=GD_CreateBufferWithData(pGD, &bufDesc, pVerts, bufDesc.ByteWidth);

	pRet->mNumTriangles	=numTris;
	pRet->mNumVerts		=w * h;
	pRet->mVertSize		=sizeof(TerrainVert);


	MakeIBDesc(&bufDesc, 2 * numTris * 3);

	pRet->mpIndexs	=GD_CreateBufferWithData(pGD, &bufDesc, pIndexes, 2 * numTris * 3);

	//free data
	free(pVerts);
	free(pIndexes);

	return	pRet;
}


void	Terrain_Draw(Terrain *pTer, GraphicsDevice *pGD, const StuffKeeper *pSK)
{
	GD_IASetVertexBuffers(pGD, pTer->mpVerts, pTer->mVertSize, 0);
	GD_IASetIndexBuffers(pGD, pTer->mpIndexs, DXGI_FORMAT_R16_UINT, 0);
	GD_IASetInputLayout(pGD, StuffKeeper_GetInputLayout(pSK, "VPosNormTex04Tex14"));

	GD_VSSetShader(pGD, StuffKeeper_GetVertexShader(pSK, "WNormWPosTexFactVS"));
	GD_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, "TriTexFact8PS"));

	GD_DrawIndexed(pGD, pTer->mNumTriangles * 3, 0, 0);
}