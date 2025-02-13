#include	<stdint.h>
#include	<utstring.h>
#include	<d3d11.h>
#include	<stdio.h>
#include	<string.h>
#include	<assert.h>
#include	"../UtilityLib/FileStuff.h"
#include	"../UtilityLib/MiscStuff.h"
#include	"../UtilityLib/ListStuff.h"
#include	"../UtilityLib/GraphicsDevice.h"
#include	"../MaterialLib/StuffKeeper.h"
#include	"../MaterialLib/Material.h"
#include	"../UtilityLib/PlaneMath.h"
#include	"Terrain.h"


typedef struct	Terrain_t
{
	UT_string					*mpName;
	ID3D11Buffer				*mpVerts, *mpIndexs;
	ID3D11InputLayout			*mpLayout;
	ID3D11ShaderResourceView	*mpSRV;
	int							mNumVerts, mNumTriangles, mVertSize;

	//jolt stuff
	JPH_HeightFieldShape	*mpHFS;
	JPH_BodyID				mTerBodyID;

}	Terrain;

//statics
static void	sSmoothPass(const float *pHeights, float *pOut, int w, int h);
static void	sVBAndIndex(GraphicsDevice *pGD, TerrainVert *pVerts,
	uint32_t w, uint32_t h, ID3D11Buffer **ppVB, ID3D11Buffer **ppIB);


Terrain	*Terrain_Create(GraphicsDevice *pGD, JPH_BodyInterface *pBI,
	const char *pName, const char *pPath, JPH_ObjectLayer objLayer,
	int numSmoothPasses, float heightScalar)
{
	uint32_t	w, h, wp1, hp1;
	int			rowPitch, i;

	Terrain	*pRet	=malloc(sizeof(Terrain));

	memset(pRet, 0, sizeof(Terrain));

	BYTE	**pRows	=SK_LoadTextureBytes(pPath, &rowPitch, &w, &h);

	//a note on height texture sizes...
	//Most textures will be power of two, and thusly will be split evenly
	//at the midpoint between two heights.  This would need an extra set of
	//verts at this strange midpoint, which would require splitting the quads
	//and such.  Instead I think I will copy the last row and column to make
	//one extra row and column, so like a 64x64 becomes a 65x65.  This would
	//make the middle point directly on the 32nd row/column

	wp1	=w + 1;
	hp1	=h + 1;

	int	numQuads	=w * h;
	int	numTris		=numQuads * 2;

	int	bytesPerHeight	=rowPitch / w;

	TerrainVert	*pVerts	=malloc(sizeof(TerrainVert) * wp1 * hp1);

	float	*pSmooth0, *pSmooth1;

	//allocate some smoothing space if needed
	if(numSmoothPasses > 0)
	{
		pSmooth0	=malloc(sizeof(float) * wp1 * hp1);
		pSmooth1	=malloc(sizeof(float) * wp1 * hp1);
	}

	//copy in the height data
	for(int y=0;y < h;y++)
	{
		for(int x=0;x < w;x++)
		{
			int	ofsX	=(x * bytesPerHeight);
			int	idx		=(y * wp1) + x;

			//red is fine
			float	height	=pRows[y][ofsX] * heightScalar;

			pVerts[idx].mPosition[0]	=x;
			pVerts[idx].mPosition[1]	=height;
			pVerts[idx].mPosition[2]	=y;

#if	0
			//test tex factors
			vec3	fac0, fac1, fac2;
			Misc_RandomDirection(fac0);
			Misc_RandomDirection(fac1);
			Misc_RandomDirection(fac2);

			glm_vec3_scale(fac0, 0.33333f, fac0);
			glm_vec3_scale(fac1, 0.33333f, fac1);
			glm_vec3_scale(fac2, 0.33333f, fac2);

			//set texture 0 to fully on by default for now
			Misc_Convert4ToF16(fac0[0], fac0[1], fac0[2], fac1[0], pVerts[idx].mTexFactor0);
			Misc_Convert4ToF16(fac1[1], fac1[2], fac2[0], fac2[1], pVerts[idx].mTexFactor1);
#else
			Misc_Convert4ToF16(0.0f, 0.0f, 0.0f, 0.0f, pVerts[idx].mTexFactor0);
			Misc_Convert4ToF16(0.0f, 0.0f, 0.0f, 1.0f, pVerts[idx].mTexFactor1);
#endif
		}
	}

	//copy extra column on the far end
	for(int y=0;y < h;y++)
	{
		memcpy(&pVerts[(y * wp1) + w], &pVerts[(y * wp1) + (w - 1)], sizeof(TerrainVert));

		//bump over the x by 1
		pVerts[(y * wp1) + w].mPosition[0]++;
	}

	//copy extra row at the bottom
	memcpy(&pVerts[h * wp1], &pVerts[(h - 1) * wp1], sizeof(TerrainVert) * wp1);

	//increment Z in that bottom row
	for(int x=0;x < wp1;x++)
	{
		pVerts[(h * wp1) + x].mPosition[2]++;
	}

	//do copy into smoothing buffer 0 if needed
	if(numSmoothPasses > 0)
	{
		for(int i=0;i < (wp1 * hp1);i++)
		{
			pSmooth0[i]	=pVerts[i].mPosition[1];
		}
	}

	//smooth a bit
	bool	bToggle	=false;
	for(i=0;i < numSmoothPasses;i++)
	{
		if(bToggle)
		{
			sSmoothPass(pSmooth1, pSmooth0, wp1, hp1);
		}
		else
		{
			sSmoothPass(pSmooth0, pSmooth1, wp1, hp1);
		}

		bToggle	=!bToggle;
	}

	if(numSmoothPasses > 0)
	{
		//copy back into verts
		float	*pSmoothed;
		if(bToggle)
		{
			pSmoothed	=pSmooth1;
		}
		else
		{
			pSmoothed	=pSmooth0;
		}

		for(int i=0;i < (wp1 * hp1);i++)
		{
			pVerts[i].mPosition[1]	=pSmoothed[i];
		}

		free(pSmooth0);
		free(pSmooth1);
	}

	//free data
	for(int y=0;y < h;y++)
	{
		free(pRows[y]);
	}
	free(pRows);

	//store heights for jolt
	float	*pJHeights	=malloc(sizeof(float) * (hp1 * wp1));

	//compute normals
	for(int y=0;y < hp1;y++)
	{
		for(int x=0;x < wp1;x++)
		{
			//grab edge vectors connected to this vert
			vec3	edge0, edge1, edge2, edge3;
			int		leftIdx, upIdx, rightIdx, downIdx;

			int	idx	=(y * wp1) + x;

			int	leftX	=(x > 0)?	x - 1 : x;
			int	upY		=(y > 0)?	y - 1 : y;
			int	rightX	=(x < w)?	x + 1 : x;
			int	downY	=(y < h)?	y + 1 : y;

			//copy height
			pJHeights[idx]	=pVerts[idx].mPosition[1];

			//indexes of the nearby clamped
			leftIdx		=(y * wp1) + leftX;
			upIdx		=(upY * wp1) + x;
			rightIdx	=(y * wp1) + rightX;
			downIdx		=(downY * wp1) + x;

			glm_vec3_sub(pVerts[leftIdx].mPosition, pVerts[idx].mPosition, edge0);
			glm_vec3_sub(pVerts[upIdx].mPosition, pVerts[idx].mPosition, edge1);
			glm_vec3_sub(pVerts[rightIdx].mPosition, pVerts[idx].mPosition, edge2);
			glm_vec3_sub(pVerts[downIdx].mPosition, pVerts[idx].mPosition, edge3);

			vec3	n0, n1, n2, n3;
			glm_vec3_cross(edge1, edge0, n0);
			glm_vec3_cross(edge2, edge1, n1);
			glm_vec3_cross(edge3, edge2, n2);
			glm_vec3_cross(edge0, edge3, n3);

			//accumulate
			glm_vec3_add(n0, n1, n0);
			glm_vec3_add(n0, n2, n0);
			glm_vec3_add(n0, n3, n0);

			glm_vec3_normalize(n0);

			Misc_Convert4ToF16(n0[0], n0[1], n0[2], 1.0f, pVerts[idx].mNormal);
		}
	}

	sVBAndIndex(pGD, pVerts, wp1, hp1, &pRet->mpVerts, &pRet->mpIndexs);

	pRet->mNumTriangles	=numTris;
	pRet->mNumVerts		=wp1 * hp1;
	pRet->mVertSize		=sizeof(TerrainVert);

	//create physics body
	JPH_Vec3	zeroVec	={0};
	JPH_Vec3	oneVec	={1,1,1};

	assert(wp1 == hp1);

	JPH_HeightFieldShapeSettings	*pHFSS	=JPH_HeightFieldShapeSettings_Create(
			pJHeights, &zeroVec, &oneVec, wp1);

	pRet->mpHFS	=JPH_HeightFieldShapeSettings_CreateShape(pHFSS);

	JPH_BodyCreationSettings	*pTBSettings	=JPH_BodyCreationSettings_Create2(
		(const JPH_ShapeSettings *)pHFSS,
		&zeroVec, NULL, JPH_MotionType_Static, objLayer);

	pRet->mTerBodyID	=JPH_BodyInterface_CreateAndAddBody(pBI, pTBSettings,
					JPH_Activation_DontActivate);

	JPH_BodyCreationSettings_Destroy(pTBSettings);

	//free data
	free(pVerts);
	free(pJHeights);

	return	pRet;
}

void	Terrain_Destroy(Terrain **ppTer, JPH_BodyInterface *pBI)
{
	Terrain	*pTer	=*ppTer;

	//free GPU stuff, srv and layout are not addrefd
	pTer->mpVerts->lpVtbl->Release(pTer->mpVerts);
	pTer->mpIndexs->lpVtbl->Release(pTer->mpIndexs);

	JPH_BodyInterface_RemoveAndDestroyBody(pBI, pTer->mTerBodyID);

	free(pTer);

	*ppTer	=NULL;
}


void	Terrain_SetSRVAndLayout(Terrain *pTer, const char *szSRV, const StuffKeeper *pSK)
{
	if(szSRV == NULL)
	{
		pTer->mpSRV	=NULL;
	}
	else
	{
		pTer->mpSRV		=StuffKeeper_GetSRV(pSK, szSRV);
	}
	
	pTer->mpLayout	=StuffKeeper_GetInputLayout(pSK, "VPosNormTex04Tex14");
}


void	Terrain_Draw(Terrain *pTer, GraphicsDevice *pGD, const StuffKeeper *pSK)
{
	GD_IASetVertexBuffers(pGD, pTer->mpVerts, pTer->mVertSize, 0);
	GD_IASetIndexBuffers(pGD, pTer->mpIndexs, DXGI_FORMAT_R32_UINT, 0);
	GD_IASetInputLayout(pGD, pTer->mpLayout);

	GD_VSSetShader(pGD, StuffKeeper_GetVertexShader(pSK, "WNormWPosTexFactVS"));
	GD_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, "TriTexFact8PS"));

	GD_PSSetSRV(pGD, pTer->mpSRV, 0);

	GD_DrawIndexed(pGD, pTer->mNumTriangles * 3, 0, 0);
}

void	Terrain_DrawMat(Terrain *pTer, GraphicsDevice *pGD, CBKeeper *pCBK, const Material *pMat)
{
	GD_IASetVertexBuffers(pGD, pTer->mpVerts, pTer->mVertSize, 0);
	GD_IASetIndexBuffers(pGD, pTer->mpIndexs, DXGI_FORMAT_R32_UINT, 0);

	GD_IASetInputLayout(pGD, pTer->mpLayout);

	MAT_Apply(pMat, pCBK, pGD);

	GD_DrawIndexed(pGD, pTer->mNumTriangles * 3, 0, 0);
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

static void	sMakeVBDesc(D3D11_BUFFER_DESC *pDesc, uint32_t byteSize)
{
	memset(pDesc, 0, sizeof(D3D11_BUFFER_DESC));

	pDesc->BindFlags			=D3D11_BIND_VERTEX_BUFFER;
	pDesc->ByteWidth			=byteSize;
	pDesc->CPUAccessFlags		=DXGI_CPU_ACCESS_NONE;
	pDesc->MiscFlags			=0;
	pDesc->StructureByteStride	=0;
	pDesc->Usage				=D3D11_USAGE_IMMUTABLE;
}

static void	sSmoothPass(const float *pHeights, float *pOut, int w, int h)
{
	for(int y=0;y < h;y++)
	{
		for(int x=0;x < w;x++)
		{
			float	upLeft, up, upRight;
			float	left, right;
			float	downLeft, down, downRight;

			if(y > 0)
			{
				if(x > 0)
				{
					upLeft	=pHeights[((y - 1) * w) + (x - 1)];
				}
				else
				{
					upLeft	=pHeights[((y - 1) * w)];
				}

				if(x < (w - 1))
				{
					upRight	=pHeights[((y - 1) * w) + (x + 1)];
				}
				else
				{
					upRight	=pHeights[((y - 1) * w) + x];
				}
				up	=pHeights[((y - 1) * w) + x];
			}
			else
			{
				if(x > 0)
				{
					upLeft	=pHeights[(y * w) + (x - 1)];
				}
				else
				{
					upLeft	=pHeights[(y * w)];
				}
				if(x < (w - 1))
				{
					upRight	=pHeights[(y * w) + (x + 1)];
				}
				else
				{
					upRight	=pHeights[(y * w) + x];
				}
				up	=pHeights[(y * w) + x];
			}

			if(x > 0)
			{
				left	=pHeights[(y * w) + (x - 1)];
			}
			else
			{
				left	=pHeights[(y * w)];
			}

			if(x < (w - 1))
			{
				right	=pHeights[(y * w) + (x + 1)];
			}
			else
			{
				right	=pHeights[(y * w) + x];
			}

			if(y < (h - 1))
			{
				if(x > 0)
				{
					downLeft	=pHeights[((y + 1) * w) + (x - 1)];
				}
				else
				{
					downLeft	=pHeights[((y + 1) * w)];
				}

				if(x < (w - 1))
				{
					downRight	=pHeights[((y + 1) * w) + (x + 1)];
				}
				else
				{
					downRight	=pHeights[((y + 1) * w) + x];
				}

				down	=pHeights[((y + 1) * w) + x];
			}
			else
			{
				if(x > 0)
				{
					downLeft	=pHeights[(y * w) + (x - 1)];
				}
				else
				{
					downLeft	=pHeights[(y * w)];
				}

				if(x < (w - 1))
				{
					downRight	=pHeights[(y * w) + (x + 1)];
				}
				else
				{
					downRight	=pHeights[(y * w) + x];
				}

				down	=pHeights[(y * w) + x];
			}

			float	sum	=upLeft + up + upRight + left
				+ right + downLeft + down + downRight;

			sum	/=8.0f;

			pOut[(y * w) + x]	=sum;
		}
	}
}

static void	sVBAndIndex(GraphicsDevice *pGD, TerrainVert *pVerts,
						uint32_t w, uint32_t h,
						ID3D11Buffer **ppVB, ID3D11Buffer **ppIB)
{
	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;

	sMakeVBDesc(&bufDesc, sizeof(TerrainVert) * w * h);

	*ppVB	=GD_CreateBufferWithData(pGD, &bufDesc, pVerts, bufDesc.ByteWidth);

	//-1 for anding
	uint32_t	wm1	=w - 1;
	uint32_t	hm1	=h - 1;

	int	numQuads	=wm1 * hm1;
	int	numTris		=numQuads * 2;

	//indexes
	uint32_t	*pIndexes	=malloc(4 * numTris * 3);

	int	curIdx	=0;
	for(int y=0;y < hm1;y++)
	{
		for(int x=0;x < wm1;x++)
		{
			int	idx		=(y * w) + x;

			//tri 0
			pIndexes[curIdx++]	=idx + w;
			pIndexes[curIdx++]	=idx;
			pIndexes[curIdx++]	=idx + 1;

			//tri1
			pIndexes[curIdx++]	=idx + w;
			pIndexes[curIdx++]	=idx + 1;
			pIndexes[curIdx++]	=idx + 1 + w;
		}
	}

	MakeIBDesc(&bufDesc, 4 * numTris * 3);

	*ppIB	=GD_CreateBufferWithData(pGD, &bufDesc, pIndexes, 4 * numTris * 3);

	//free data
	free(pIndexes);
}