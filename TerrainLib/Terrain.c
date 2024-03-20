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
#include	"../MaterialLib/Material.h"
#include	"../UtilityLib/ConvexVolume.h"
#include	"../UtilityLib/PlaneMath.h"
#include	"QuadTree.h"
#include	"QuadNode.h"	//for testing splits


typedef struct	Terrain_t
{
	UT_string					*mpName;
	ID3D11Buffer				*mpVerts, *mpIndexs;
	ID3D11InputLayout			*mpLayout;
	ID3D11ShaderResourceView	*mpSRV;
	int							mNumVerts, mNumTriangles, mVertSize;

	QuadTree	*mpQT;
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

static void	SmoothPass(const float *pHeights, float *pOut, int w, int h)
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

static void	VBAndIndex(GraphicsDevice *pGD, TerrainVert *pVerts, uint32_t w, uint32_t h,
						ID3D11Buffer **ppVB, ID3D11Buffer **ppIB)
{
	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;

	MakeVBDesc(&bufDesc, sizeof(TerrainVert) * w * h);

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


Terrain	*Terrain_Create(GraphicsDevice *pGD,
	const char *pName, const char *pPath,
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
			SmoothPass(pSmooth1, pSmooth0, wp1, hp1);
		}
		else
		{
			SmoothPass(pSmooth0, pSmooth1, wp1, hp1);
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

	//build quadtree
	pRet->mpQT	=QT_Create(pVerts, wp1, hp1);

	VBAndIndex(pGD, pVerts, wp1, hp1, &pRet->mpVerts, &pRet->mpIndexs);

	pRet->mNumTriangles	=numTris;
	pRet->mNumVerts		=wp1 * hp1;
	pRet->mVertSize		=sizeof(TerrainVert);

	//free data
	free(pVerts);

	return	pRet;
}


void	Terrain_SetSRV(Terrain *pTer, const char *szSRV, const StuffKeeper *pSK)
{
	pTer->mpSRV	=StuffKeeper_GetSRV(pSK, szSRV);
}


void	Terrain_Draw(Terrain *pTer, GraphicsDevice *pGD, const StuffKeeper *pSK)
{
	GD_IASetVertexBuffers(pGD, pTer->mpVerts, pTer->mVertSize, 0);
	GD_IASetIndexBuffers(pGD, pTer->mpIndexs, DXGI_FORMAT_R32_UINT, 0);
	GD_IASetInputLayout(pGD, StuffKeeper_GetInputLayout(pSK, "VPosNormTex04Tex14"));

	GD_VSSetShader(pGD, StuffKeeper_GetVertexShader(pSK, "WNormWPosTexFactVS"));
	GD_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, "TriTexFact8PS"));

	GD_PSSetSRV(pGD, pTer->mpSRV, 0);

	GD_DrawIndexed(pGD, pTer->mNumTriangles * 3, 0, 0);
}

void	Terrain_DrawMat(Terrain *pTer, GraphicsDevice *pGD, CBKeeper *pCBK, const Material *pMat)
{
	GD_IASetVertexBuffers(pGD, pTer->mpVerts, pTer->mVertSize, 0);
	GD_IASetIndexBuffers(pGD, pTer->mpIndexs, DXGI_FORMAT_R32_UINT, 0);

	MAT_Apply(pMat, pCBK, pGD);

	GD_DrawIndexed(pGD, pTer->mNumTriangles * 3, 0, 0);
}


void	Terrain_GetBounds(const Terrain *pTer, vec3 mins, vec3 maxs)
{
	QT_GetBounds(pTer->mpQT, mins, maxs);
}

void	Terrain_GetQuadTreeLeafBoxes(Terrain *pTer, vec3 **ppMins, vec3 **ppMaxs, int *pNumBounds)
{
	assert(pTer != NULL);
	assert(pTer->mpQT != NULL);

	QT_GatherLeafBounds(pTer->mpQT, ppMins, ppMaxs, pNumBounds);
}

int	Terrain_LineIntersect(const Terrain *pTer, const vec3 start, const vec3 end,
							vec3 intersection, vec4 planeHit)
{
	return	QT_LineIntersect(pTer->mpQT, start, end, intersection, planeHit);
}

int	Terrain_SweptSphereIntersect(const Terrain *pTer, const vec3 start, const vec3 end,
									float radius, vec3 intersection, vec4 planeHit)
{
	return	QT_SweptSphereIntersect(pTer->mpQT, start, end, radius, intersection, planeHit);
}

int	Terrain_SweptBoundIntersect(const Terrain *pTer, const vec3 start, const vec3 end,
								const vec3 min, const vec3 max,
								vec3 intersection, vec4 planeHit)
{
	//box needs to be centered
	vec3	diff;
	glm_vec3_add(max, min, diff);

	assert(glm_vec3_eq_eps(diff, 0.0f));

	return	QT_SweptBoundIntersect(pTer->mpQT, start, end, min, max, intersection, planeHit);
}

bool	Terrain_MoveBox(const Terrain *pTer, const vec3 min, const vec3 max,
						const vec3 start, const vec3 end, vec3 finalPos)
{
	int	i	=0;

	vec3	newEnd, newStart;
	glm_vec3_copy(start, newStart);
	glm_vec3_copy(end, newEnd);

	int	maxIter	=5;

//	List<ZonePlane>	hitPlanes	=new List<ZonePlane>();
	for(i=0;i < maxIter;i++)
	{
//		RayTrace	rt	=new RayTrace(newStart, newEnd);
//		rt.mBounds		=box;
		vec3	intersection;
		vec4	plane;

		int	hit	=Terrain_SweptBoundIntersect(pTer, newStart, newEnd, min, max, intersection, plane);
		if(hit == VOL_MISS)
		{
			break;
		}

//		ZonePlane	zp	=rt.mCollision.mPlaneHit;

		if(hit == VOL_HIT_INSIDE)
		{
			//see if start is exactly plane on
			float	checkDist	=PM_Distance(plane, newStart);
			if(checkDist == 0.0f)
			{
				PM_ReflectPosition(plane, newStart);
				continue;
			}
			//TODO: report and solve via intersection
			//can't solve!
			glm_vec3_copy(start, finalPos);
			return	false;
		}
		else if(hit == VOL_INSIDE)
		{
			//started in a solid leaf
			//cast up
			vec3	bump	={	0.0f, 1.0f, 0.0f	};
			vec3	bumpHit;
			vec4	bumpPlane;
			glm_vec3_add(newStart, bump, bump);

			int	bumpRet	=Terrain_SweptBoundIntersect(pTer, newStart, bump, min, max, bumpHit, bumpPlane);
			if(bumpRet == VOL_HIT_VISIBLE)
			{
				PM_ReflectPosition(bumpPlane, newStart);
				continue;
			}
			return	false;
		}

		//reflect off plane hit to new endpoint
		PM_ReflectMove(plane, newStart, newEnd, newEnd);

		//copy intersect point to start
		glm_vec3_copy(intersection, newStart);

//		if(!hitPlanes.Contains(zp))
//		{
//			hitPlanes.Add(zp);
//		}
	}

	glm_vec3_copy(newEnd, finalPos);

/*	if(i == MaxMoveBoxIterations)
	{
		//this is usually caused by oblique planes causing
		//the reflected motion to bounce back and forth

		//get all the collision points along the motion
		List<Vector3>	contacts	=GetCollisions(hitPlanes, start, end);

		//get distance start to end
		float	motionDistance	=start.Distance(end);

		//stop at the closest contact to start
		float	bestDist	=float.MaxValue;
		Vector3	bestCon		=start;
		foreach(Vector3 con in contacts)
		{
			float	startDist	=con.Distance(start);
			float	endDist		=con.Distance(end);

			if(endDist > motionDistance)
			{
				//this contact is in front of the start!
				//this can only mean the starting position
				//was in solid
				finalPos	=start;
				return	false;
			}

			if(startDist < bestDist)
			{
				bestDist	=startDist;
				bestCon		=con;
			}
		}

		if(bestCon.Distance(start) <= Mathery.VCompareEpsilon)
		{
			//so close might as well use the start position
			finalPos	=start;
			return	true;
		}
		//push back along the vector a bit
		Vector3	motionVec	=end - start;

		motionVec	/=motionDistance;
		motionVec	*=Mathery.VCompareEpsilon;

		finalPos	=bestCon - motionVec;
		return	true;
	}*/
	return	true;
}