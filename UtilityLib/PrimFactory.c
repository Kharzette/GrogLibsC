#include	<d3d11_1.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<assert.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<x86intrin.h>
#include	<utlist.h>
#include	<cglm/call.h>
#include	"GraphicsDevice.h"
#include	"PrimFactory.h"
#include	"MiscStuff.h"
#include	"ConvexVolume.h"
#include	"PlaneMath.h"


typedef struct	VPosNorm_t
{
	vec3		Position;
	uint16_t	Normal[4];	//16 bit float4
}	VPosNorm;

typedef struct	VPosNormCol0_t
{
	vec3		Position;
	uint16_t	Normal[4];	//16 bit float4
	uint16_t	Color0[4];	//16 bit float4
}	VPosNormCol0;

static const	vec3	UnitX	={	1.0f, 0.0f, 0.0f	};
static const	vec3	UnitY	={	0.0f, 1.0f, 0.0f	};
static const	vec3	UnitZ	={	0.0f, 0.0f, 1.0f	};
static const	vec3	One		={	1.0f, 1.0f, 1.0f	};

static void	MulAdd2DestS(const vec2 a, const float b, const vec2 c, vec2 dest)
{
	dest[0]	=c[0] + (a[0] * b);
	dest[1]	=c[1] + (a[1] * b);
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

//make a vec out of stealing xyz from 3 different vectors
static void	ThreeXYZ(const vec3 x, const vec3 y, const vec3 z, vec3 dest)
{
	dest[0]	=x[0];
	dest[1]	=y[1];
	dest[2]	=z[2];
}


void	PF_DestroyPO(PrimObject **ppObj)
{
	//release buffers
	(*ppObj)->mpIB->lpVtbl->Release((*ppObj)->mpIB);
	(*ppObj)->mpVB->lpVtbl->Release((*ppObj)->mpVB);

	//free mem
	free(*ppObj);

	*ppObj	=NULL;
}


PrimObject	*PF_CreateCubeFromBounds(const vec3 min, const vec3 max, GraphicsDevice *pGD)
{
	vec3	corners[8];

	ThreeXYZ(max, min, max, corners[0]);
	ThreeXYZ(min, min, max, corners[1]);
	ThreeXYZ(max, min, min, corners[2]);
	ThreeXYZ(min, min, min, corners[3]);
	ThreeXYZ(max, max, max, corners[4]);
	ThreeXYZ(min, max, max, corners[5]);
	ThreeXYZ(max, max, min, corners[6]);
	ThreeXYZ(min, max, min, corners[7]);

	return	PF_CreateCubeFromCorners(corners, false, pGD);
}

PrimObject	*PF_CreateCubeFromCorners(const vec3 *pCorners, bool bFlipped, GraphicsDevice *pGD)
{
	VPosNorm	vpn[24];

	//cube corners
	vec3	lowerTopRight, lowerTopLeft, lowerBotRight, lowerBotLeft;
	vec3	upperTopRight, upperTopLeft, upperBotRight, upperBotLeft;

	glm_vec3_copy(pCorners[0], lowerTopRight);
	glm_vec3_copy(pCorners[1], lowerTopLeft);
	glm_vec3_copy(pCorners[2], lowerBotRight);
	glm_vec3_copy(pCorners[3], lowerBotLeft);
	glm_vec3_copy(pCorners[4], upperTopRight);
	glm_vec3_copy(pCorners[5], upperTopLeft);
	glm_vec3_copy(pCorners[6], upperBotRight);
	glm_vec3_copy(pCorners[7], upperBotLeft);

	//cube sides
	//top
	glm_vec3_copy(upperTopLeft,		vpn[0].Position);
	glm_vec3_copy(upperTopRight,	vpn[1].Position);
	glm_vec3_copy(upperBotRight,	vpn[2].Position);
	glm_vec3_copy(upperBotLeft,		vpn[3].Position);

	//bottom (note reversal)
	glm_vec3_copy(lowerTopLeft,		vpn[7].Position);
	glm_vec3_copy(lowerTopRight,	vpn[6].Position);
	glm_vec3_copy(lowerBotRight,	vpn[5].Position);
	glm_vec3_copy(lowerBotLeft,		vpn[4].Position);

	//top z side
	glm_vec3_copy(upperTopLeft,		vpn[11].Position);
	glm_vec3_copy(upperTopRight,	vpn[10].Position);
	glm_vec3_copy(lowerTopRight,	vpn[9].Position);
	glm_vec3_copy(lowerTopLeft,		vpn[8].Position);

	//bottom z side
	glm_vec3_copy(upperBotLeft,		vpn[12].Position);
	glm_vec3_copy(upperBotRight,	vpn[13].Position);
	glm_vec3_copy(lowerBotRight,	vpn[14].Position);
	glm_vec3_copy(lowerBotLeft,		vpn[15].Position);

	//-x side
	glm_vec3_copy(upperTopLeft,		vpn[16].Position);
	glm_vec3_copy(upperBotLeft,		vpn[17].Position);
	glm_vec3_copy(lowerBotLeft,		vpn[18].Position);
	glm_vec3_copy(lowerTopLeft,		vpn[19].Position);

	//+x side
	glm_vec3_copy(upperTopRight,	vpn[23].Position);
	glm_vec3_copy(upperBotRight,	vpn[22].Position);
	glm_vec3_copy(lowerBotRight,	vpn[21].Position);
	glm_vec3_copy(lowerTopRight,	vpn[20].Position);

	//normals
	Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpn[0].Normal);
	Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpn[1].Normal);
	Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpn[2].Normal);
	Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpn[3].Normal);

	Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpn[4].Normal);
	Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpn[5].Normal);
	Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpn[6].Normal);
	Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpn[7].Normal);

	Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpn[8].Normal);
	Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpn[9].Normal);
	Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpn[10].Normal);
	Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpn[11].Normal);

	Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpn[12].Normal);
	Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpn[13].Normal);
	Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpn[14].Normal);
	Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpn[15].Normal);

	Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpn[16].Normal);
	Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpn[17].Normal);
	Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpn[18].Normal);
	Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpn[19].Normal);

	Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpn[20].Normal);
	Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpn[21].Normal);
	Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpn[22].Normal);
	Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpn[23].Normal);

	//indexes
	uint16_t	idx, indexes[36];
	for(int i=idx=0;i < 36;i+=6)
	{
		if(bFlipped)
		{
			indexes[i]		=idx + 0;
			indexes[i + 1]	=idx + 2;
			indexes[i + 2]	=idx + 1;
			indexes[i + 3]	=idx + 0;
			indexes[i + 4]	=idx + 3;
			indexes[i + 5]	=idx + 2;
		}
		else
		{
			indexes[i]		=idx + 0;
			indexes[i + 1]	=idx + 1;
			indexes[i + 2]	=idx + 2;
			indexes[i + 3]	=idx + 0;
			indexes[i + 4]	=idx + 2;
			indexes[i + 5]	=idx + 3;
		}
		idx	+=4;
	}

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=24;
	pObj->mIndexCount	=36;

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(VPosNorm) * 24);
	pObj->mpVB	=GD_CreateBufferWithData(pGD, &bufDesc, vpn, bufDesc.ByteWidth);

	//make index buffer
	MakeIBDesc(&bufDesc, 36 * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, indexes, bufDesc.ByteWidth);

	pObj->mVertSize	=sizeof(VPosNorm);

	return	pObj;
}

PrimObject	*PF_CreateCube(float size, bool bFlipped, GraphicsDevice *pGD)
{
	vec3	xScaled, yScaled, zScaled;
	vec3	xNeg, yNeg, zNeg;

	glm_vec3_scale(UnitX, size, xScaled);
	glm_vec3_scale(UnitY, size, yScaled);
	glm_vec3_scale(UnitZ, size, zScaled);

	glm_vec3_flipsign_to(xScaled, xNeg);
	glm_vec3_flipsign_to(yScaled, yNeg);
	glm_vec3_flipsign_to(zScaled, zNeg);

	vec3	corners[8];

	for(int i=0;i < 3;i++)
	{
		corners[0][i]	=yNeg[i]	+ xScaled[i]	+ zScaled[i];
		corners[1][i]	=yNeg[i]	+ xNeg[i]		+ zScaled[i];
		corners[2][i]	=yNeg[i]	+ xScaled[i]	+ zNeg[i];
		corners[3][i]	=yNeg[i]	+ xNeg[i]		+ zNeg[i];
		corners[4][i]	=yScaled[i]	+ xScaled[i]	+ zScaled[i];
		corners[5][i]	=yScaled[i]	+ xNeg[i]		+ zScaled[i];
		corners[6][i]	=yScaled[i]	+ xScaled[i]	+ zNeg[i];
		corners[7][i]	=yScaled[i]	+ xNeg[i]		+ zNeg[i];
	}

	return	PF_CreateCubeFromCorners(corners, bFlipped, pGD);
}

PrimObject	*PF_CreateCubesFromBoundArray(const vec3 *pMins, const vec3 *pMaxs, int numBounds, GraphicsDevice *pGD)
{
	VPosNormCol0	*vpnc	=malloc(sizeof(VPosNormCol0) * 24 * numBounds);
	uint32_t		*inds	=malloc(sizeof(uint32_t) * 36 * numBounds);

	for(int i=0;i < numBounds;i++)
	{
		int		ofs	=i * 24;
		vec3	mins, maxs;
		vec4	col;

		Misc_RandomColour(col);

		glm_vec3_copy(pMins[i], mins);
		glm_vec3_copy(pMaxs[i], maxs);

		//cube corners
		vec3	lowerTopRight, lowerTopLeft, lowerBotRight, lowerBotLeft;
		vec3	upperTopRight, upperTopLeft, upperBotRight, upperBotLeft;

		glm_vec3_zero(lowerTopRight);
		glm_vec3_zero(lowerTopLeft);
		glm_vec3_zero(lowerBotRight);
		glm_vec3_zero(lowerBotLeft);
		glm_vec3_zero(upperTopRight);
		glm_vec3_zero(upperTopLeft);
		glm_vec3_zero(upperBotRight);
		glm_vec3_zero(upperBotLeft);

		//generate corners from min/max
		glm_vec3_muladd(UnitX, maxs, lowerTopRight);
		glm_vec3_muladd(UnitY, mins, lowerTopRight);
		glm_vec3_muladd(UnitZ, maxs, lowerTopRight);

		glm_vec3_muladd(UnitX, mins, lowerTopLeft);
		glm_vec3_muladd(UnitY, mins, lowerTopLeft);
		glm_vec3_muladd(UnitZ, maxs, lowerTopLeft);

		glm_vec3_muladd(UnitX, maxs, lowerBotRight);
		glm_vec3_muladd(UnitY, mins, lowerBotRight);
		glm_vec3_muladd(UnitZ, mins, lowerBotRight);

		glm_vec3_muladd(UnitX, mins, lowerBotLeft);
		glm_vec3_muladd(UnitY, mins, lowerBotLeft);
		glm_vec3_muladd(UnitZ, mins, lowerBotLeft);

		glm_vec3_muladd(UnitX, maxs, upperTopRight);
		glm_vec3_muladd(UnitY, maxs, upperTopRight);
		glm_vec3_muladd(UnitZ, maxs, upperTopRight);

		glm_vec3_muladd(UnitX, mins, upperTopLeft);
		glm_vec3_muladd(UnitY, maxs, upperTopLeft);
		glm_vec3_muladd(UnitZ, maxs, upperTopLeft);

		glm_vec3_muladd(UnitX, maxs, upperBotRight);
		glm_vec3_muladd(UnitY, maxs, upperBotRight);
		glm_vec3_muladd(UnitZ, mins, upperBotRight);

		glm_vec3_muladd(UnitX, mins, upperBotLeft);
		glm_vec3_muladd(UnitY, maxs, upperBotLeft);
		glm_vec3_muladd(UnitZ, mins, upperBotLeft);

		//cube sides
		//top
		glm_vec3_copy(upperTopLeft,		vpnc[ofs + 0].Position);
		glm_vec3_copy(upperTopRight,	vpnc[ofs + 1].Position);
		glm_vec3_copy(upperBotRight,	vpnc[ofs + 2].Position);
		glm_vec3_copy(upperBotLeft,		vpnc[ofs + 3].Position);

		//bottom (note reversal)
		glm_vec3_copy(lowerTopLeft,		vpnc[ofs + 7].Position);
		glm_vec3_copy(lowerTopRight,	vpnc[ofs + 6].Position);
		glm_vec3_copy(lowerBotRight,	vpnc[ofs + 5].Position);
		glm_vec3_copy(lowerBotLeft,		vpnc[ofs + 4].Position);

		//top z side
		glm_vec3_copy(upperTopLeft,		vpnc[ofs + 11].Position);
		glm_vec3_copy(upperTopRight,	vpnc[ofs + 10].Position);
		glm_vec3_copy(lowerTopRight,	vpnc[ofs + 9].Position);
		glm_vec3_copy(lowerTopLeft,		vpnc[ofs + 8].Position);

		//bottom z side
		glm_vec3_copy(upperBotLeft,		vpnc[ofs + 12].Position);
		glm_vec3_copy(upperBotRight,	vpnc[ofs + 13].Position);
		glm_vec3_copy(lowerBotRight,	vpnc[ofs + 14].Position);
		glm_vec3_copy(lowerBotLeft,		vpnc[ofs + 15].Position);

		//-x side
		glm_vec3_copy(upperTopLeft,		vpnc[ofs + 16].Position);
		glm_vec3_copy(upperBotLeft,		vpnc[ofs + 17].Position);
		glm_vec3_copy(lowerBotLeft,		vpnc[ofs + 18].Position);
		glm_vec3_copy(lowerTopLeft,		vpnc[ofs + 19].Position);

		//+x side
		glm_vec3_copy(upperTopRight,	vpnc[ofs + 23].Position);
		glm_vec3_copy(upperBotRight,	vpnc[ofs + 22].Position);
		glm_vec3_copy(lowerBotRight,	vpnc[ofs + 21].Position);
		glm_vec3_copy(lowerTopRight,	vpnc[ofs + 20].Position);
		
		//normals
		Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpnc[ofs + 0].Normal);
		Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpnc[ofs + 1].Normal);
		Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpnc[ofs + 2].Normal);
		Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpnc[ofs + 3].Normal);

		Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpnc[ofs + 4].Normal);
		Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpnc[ofs + 5].Normal);
		Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpnc[ofs + 6].Normal);
		Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpnc[ofs + 7].Normal);

		Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpnc[ofs + 8].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpnc[ofs + 9].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpnc[ofs + 10].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpnc[ofs + 11].Normal);

		Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpnc[ofs + 12].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpnc[ofs + 13].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpnc[ofs + 14].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpnc[ofs + 15].Normal);

		Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpnc[ofs + 16].Normal);
		Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpnc[ofs + 17].Normal);
		Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpnc[ofs + 18].Normal);
		Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpnc[ofs + 19].Normal);

		Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpnc[ofs + 20].Normal);
		Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpnc[ofs + 21].Normal);
		Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpnc[ofs + 22].Normal);
		Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpnc[ofs + 23].Normal);

		for(int j=0;j < 24;j++)
		{
			Misc_ConvertVec4ToF16(col, vpnc[ofs + j].Color0);
		}

		//indexes
		uint32_t	idx	=ofs;
		for(int j=0;j < 36;j+=6)
		{
			int	indBox	=i * 36;

			inds[indBox + j]		=idx + 0;
			inds[indBox + j + 1]	=idx + 1;
			inds[indBox + j + 2]	=idx + 2;
			inds[indBox + j + 3]	=idx + 0;
			inds[indBox + j + 4]	=idx + 2;
			inds[indBox + j + 5]	=idx + 3;

			idx	+=4;
		}
	}

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=24 * numBounds;
	pObj->mIndexCount	=36 * numBounds;

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(VPosNormCol0) * 24 * numBounds);
	pObj->mpVB	=GD_CreateBufferWithData(pGD, &bufDesc, vpnc, bufDesc.ByteWidth);

	//make index buffer
	MakeIBDesc(&bufDesc, 36 * 4 * numBounds);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, inds, bufDesc.ByteWidth);

	free(vpnc);
	free(inds);

	pObj->mVertSize	=sizeof(VPosNormCol0);

	return	pObj;
}

PrimObject	*PF_CreateManyCubes(const vec3 *pCubeCenters, int numCubes,
								float size, GraphicsDevice *pGD)
{
	VPosNorm	*vpn	=malloc(sizeof(VPosNorm) * 24 * numCubes);
	uint32_t	*inds	=malloc(sizeof(uint32_t) * 36 * numCubes);

	vec3	sizeVec;
	glm_vec3_scale(GLM_VEC3_ONE, size, sizeVec);

	for(int i=0;i < numCubes;i++)
	{
		int		ofs	=i * 24;
		vec3	mins, maxs;

		glm_vec3_sub(pCubeCenters[i], sizeVec, mins);
		glm_vec3_add(pCubeCenters[i], sizeVec, maxs);

		//cube corners
		vec3	lowerTopRight, lowerTopLeft, lowerBotRight, lowerBotLeft;
		vec3	upperTopRight, upperTopLeft, upperBotRight, upperBotLeft;

		glm_vec3_zero(lowerTopRight);
		glm_vec3_zero(lowerTopLeft);
		glm_vec3_zero(lowerBotRight);
		glm_vec3_zero(lowerBotLeft);
		glm_vec3_zero(upperTopRight);
		glm_vec3_zero(upperTopLeft);
		glm_vec3_zero(upperBotRight);
		glm_vec3_zero(upperBotLeft);

		//generate corners from min/max
		glm_vec3_muladd(UnitX, maxs, lowerTopRight);
		glm_vec3_muladd(UnitY, mins, lowerTopRight);
		glm_vec3_muladd(UnitZ, maxs, lowerTopRight);

		glm_vec3_muladd(UnitX, mins, lowerTopLeft);
		glm_vec3_muladd(UnitY, mins, lowerTopLeft);
		glm_vec3_muladd(UnitZ, maxs, lowerTopLeft);

		glm_vec3_muladd(UnitX, maxs, lowerBotRight);
		glm_vec3_muladd(UnitY, mins, lowerBotRight);
		glm_vec3_muladd(UnitZ, mins, lowerBotRight);

		glm_vec3_muladd(UnitX, mins, lowerBotLeft);
		glm_vec3_muladd(UnitY, mins, lowerBotLeft);
		glm_vec3_muladd(UnitZ, mins, lowerBotLeft);

		glm_vec3_muladd(UnitX, maxs, upperTopRight);
		glm_vec3_muladd(UnitY, maxs, upperTopRight);
		glm_vec3_muladd(UnitZ, maxs, upperTopRight);

		glm_vec3_muladd(UnitX, mins, upperTopLeft);
		glm_vec3_muladd(UnitY, maxs, upperTopLeft);
		glm_vec3_muladd(UnitZ, maxs, upperTopLeft);

		glm_vec3_muladd(UnitX, maxs, upperBotRight);
		glm_vec3_muladd(UnitY, maxs, upperBotRight);
		glm_vec3_muladd(UnitZ, mins, upperBotRight);

		glm_vec3_muladd(UnitX, mins, upperBotLeft);
		glm_vec3_muladd(UnitY, maxs, upperBotLeft);
		glm_vec3_muladd(UnitZ, mins, upperBotLeft);

		//cube sides
		//top
		glm_vec3_copy(upperTopLeft,		vpn[ofs + 0].Position);
		glm_vec3_copy(upperTopRight,	vpn[ofs + 1].Position);
		glm_vec3_copy(upperBotRight,	vpn[ofs + 2].Position);
		glm_vec3_copy(upperBotLeft,		vpn[ofs + 3].Position);

		//bottom (note reversal)
		glm_vec3_copy(lowerTopLeft,		vpn[ofs + 7].Position);
		glm_vec3_copy(lowerTopRight,	vpn[ofs + 6].Position);
		glm_vec3_copy(lowerBotRight,	vpn[ofs + 5].Position);
		glm_vec3_copy(lowerBotLeft,		vpn[ofs + 4].Position);

		//top z side
		glm_vec3_copy(upperTopLeft,		vpn[ofs + 11].Position);
		glm_vec3_copy(upperTopRight,	vpn[ofs + 10].Position);
		glm_vec3_copy(lowerTopRight,	vpn[ofs + 9].Position);
		glm_vec3_copy(lowerTopLeft,		vpn[ofs + 8].Position);

		//bottom z side
		glm_vec3_copy(upperBotLeft,		vpn[ofs + 12].Position);
		glm_vec3_copy(upperBotRight,	vpn[ofs + 13].Position);
		glm_vec3_copy(lowerBotRight,	vpn[ofs + 14].Position);
		glm_vec3_copy(lowerBotLeft,		vpn[ofs + 15].Position);

		//-x side
		glm_vec3_copy(upperTopLeft,		vpn[ofs + 16].Position);
		glm_vec3_copy(upperBotLeft,		vpn[ofs + 17].Position);
		glm_vec3_copy(lowerBotLeft,		vpn[ofs + 18].Position);
		glm_vec3_copy(lowerTopLeft,		vpn[ofs + 19].Position);

		//+x side
		glm_vec3_copy(upperTopRight,	vpn[ofs + 23].Position);
		glm_vec3_copy(upperBotRight,	vpn[ofs + 22].Position);
		glm_vec3_copy(lowerBotRight,	vpn[ofs + 21].Position);
		glm_vec3_copy(lowerTopRight,	vpn[ofs + 20].Position);
		
		//normals
		Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpn[ofs + 0].Normal);
		Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpn[ofs + 1].Normal);
		Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpn[ofs + 2].Normal);
		Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpn[ofs + 3].Normal);

		Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpn[ofs + 4].Normal);
		Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpn[ofs + 5].Normal);
		Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpn[ofs + 6].Normal);
		Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpn[ofs + 7].Normal);

		Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpn[ofs + 8].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpn[ofs + 9].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpn[ofs + 10].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpn[ofs + 11].Normal);

		Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpn[ofs + 12].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpn[ofs + 13].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpn[ofs + 14].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpn[ofs + 15].Normal);

		Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpn[ofs + 16].Normal);
		Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpn[ofs + 17].Normal);
		Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpn[ofs + 18].Normal);
		Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpn[ofs + 19].Normal);

		Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpn[ofs + 20].Normal);
		Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpn[ofs + 21].Normal);
		Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpn[ofs + 22].Normal);
		Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpn[ofs + 23].Normal);

		//indexes
		uint32_t	idx	=ofs;
		for(int j=0;j < 36;j+=6)
		{
			int	indBox	=i * 36;

			inds[indBox + j]		=idx + 0;
			inds[indBox + j + 1]	=idx + 1;
			inds[indBox + j + 2]	=idx + 2;
			inds[indBox + j + 3]	=idx + 0;
			inds[indBox + j + 4]	=idx + 2;
			inds[indBox + j + 5]	=idx + 3;

			idx	+=4;
		}
	}

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=24 * numCubes;
	pObj->mIndexCount	=36 * numCubes;

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(VPosNorm) * 24 * numCubes);
	pObj->mpVB	=GD_CreateBufferWithData(pGD, &bufDesc, vpn, bufDesc.ByteWidth);

	//make index buffer
	MakeIBDesc(&bufDesc, 36 * 4 * numCubes);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, inds, bufDesc.ByteWidth);

	free(vpn);
	free(inds);

	pObj->mVertSize	=sizeof(VPosNorm);

	return	pObj;
}

PrimObject	*PF_CreateManyRays(const vec3 *pStarts, const vec3 *pEnds, const vec4 *pColours,
								int numRays, float rayWidth, GraphicsDevice *pGD)
{
	VPosNormCol0	*vpnc	=malloc(sizeof(VPosNormCol0) * 24 * numRays);
	uint32_t		*inds	=malloc(sizeof(uint32_t) * 36 * numRays);

	//these will be the full extent, passing
	//through the colliding object
	for(int i=0;i < numRays;i++)
	{
		int		ofs	=i * 24;
		vec3	start, end;

		glm_vec3_copy(pStarts[i], start);
		glm_vec3_copy(pEnds[i], end);

		//Make some basis vectors to build a box around the ray
		vec3	rayX, rayY, rayZ;

		//z for the ray direction
		glm_vec3_sub(end, start, rayZ);

		Misc_BuildBasisVecsFromDirection(rayZ, rayX, rayY, rayZ);

		glm_vec3_scale(rayX, rayWidth, rayX);
		glm_vec3_scale(rayY, rayWidth, rayY);

		//make negatives for doing normals below
		vec3	rayNegX, rayNegY, rayNegZ;
		glm_vec3_negate_to(rayX, rayNegX);
		glm_vec3_negate_to(rayY, rayNegY);
		glm_vec3_negate_to(rayZ, rayNegZ);

		//cube corners
		vec3	lowerTopRight, lowerTopLeft, lowerBotRight, lowerBotLeft;
		vec3	upperTopRight, upperTopLeft, upperBotRight, upperBotLeft;

		glm_vec3_zero(lowerTopRight);
		glm_vec3_zero(lowerTopLeft);
		glm_vec3_zero(lowerBotRight);
		glm_vec3_zero(lowerBotLeft);
		glm_vec3_zero(upperTopRight);
		glm_vec3_zero(upperTopLeft);
		glm_vec3_zero(upperBotRight);
		glm_vec3_zero(upperBotLeft);

		//generate corners from min/max
		glm_vec3_add(start, rayX, lowerTopRight);
		glm_vec3_add(lowerTopRight, rayY, lowerTopRight);

		glm_vec3_add(start, rayY, lowerTopLeft);
		glm_vec3_sub(lowerTopLeft, rayX, lowerTopLeft);

		glm_vec3_add(start, rayX, lowerBotRight);
		glm_vec3_sub(lowerBotRight, rayY, lowerBotRight);

		glm_vec3_sub(start, rayY, lowerBotLeft);
		glm_vec3_sub(lowerBotLeft, rayX, lowerBotLeft);

		glm_vec3_add(end, rayX, upperTopRight);
		glm_vec3_add(upperTopRight, rayY, upperTopRight);

		glm_vec3_add(end, rayY, upperTopLeft);
		glm_vec3_sub(upperTopLeft, rayX, upperTopLeft);

		glm_vec3_add(end, rayX, upperBotRight);
		glm_vec3_sub(upperBotRight, rayY, upperBotRight);

		glm_vec3_sub(end, rayY, upperBotLeft);
		glm_vec3_sub(upperBotLeft, rayX, upperBotLeft);

		//cube sides
		//top (Z+)
		glm_vec3_copy(upperTopLeft,		vpnc[ofs + 0].Position);
		glm_vec3_copy(upperTopRight,	vpnc[ofs + 1].Position);
		glm_vec3_copy(upperBotRight,	vpnc[ofs + 2].Position);
		glm_vec3_copy(upperBotLeft,		vpnc[ofs + 3].Position);

		//bottom (note reversal) (Z-)
		glm_vec3_copy(lowerTopLeft,		vpnc[ofs + 7].Position);
		glm_vec3_copy(lowerTopRight,	vpnc[ofs + 6].Position);
		glm_vec3_copy(lowerBotRight,	vpnc[ofs + 5].Position);
		glm_vec3_copy(lowerBotLeft,		vpnc[ofs + 4].Position);

		//top Y side (Y+)
		glm_vec3_copy(upperTopLeft,		vpnc[ofs + 11].Position);
		glm_vec3_copy(upperTopRight,	vpnc[ofs + 10].Position);
		glm_vec3_copy(lowerTopRight,	vpnc[ofs + 9].Position);
		glm_vec3_copy(lowerTopLeft,		vpnc[ofs + 8].Position);

		//bottom Y side (Y-)
		glm_vec3_copy(upperBotLeft,		vpnc[ofs + 12].Position);
		glm_vec3_copy(upperBotRight,	vpnc[ofs + 13].Position);
		glm_vec3_copy(lowerBotRight,	vpnc[ofs + 14].Position);
		glm_vec3_copy(lowerBotLeft,		vpnc[ofs + 15].Position);

		//-x side
		glm_vec3_copy(upperTopLeft,		vpnc[ofs + 16].Position);
		glm_vec3_copy(upperBotLeft,		vpnc[ofs + 17].Position);
		glm_vec3_copy(lowerBotLeft,		vpnc[ofs + 18].Position);
		glm_vec3_copy(lowerTopLeft,		vpnc[ofs + 19].Position);

		//+x side
		glm_vec3_copy(upperTopRight,	vpnc[ofs + 23].Position);
		glm_vec3_copy(upperBotRight,	vpnc[ofs + 22].Position);
		glm_vec3_copy(lowerBotRight,	vpnc[ofs + 21].Position);
		glm_vec3_copy(lowerTopRight,	vpnc[ofs + 20].Position);
		
		//normals
		//top (Z+)
		Misc_ConvertVec3ToF16(rayZ, vpnc[ofs + 0].Normal);
		Misc_ConvertVec3ToF16(rayZ, vpnc[ofs + 1].Normal);
		Misc_ConvertVec3ToF16(rayZ, vpnc[ofs + 2].Normal);
		Misc_ConvertVec3ToF16(rayZ, vpnc[ofs + 3].Normal);

		//bottom (Z-)
		Misc_ConvertVec3ToF16(rayNegZ, vpnc[ofs + 4].Normal);
		Misc_ConvertVec3ToF16(rayNegZ, vpnc[ofs + 5].Normal);
		Misc_ConvertVec3ToF16(rayNegZ, vpnc[ofs + 6].Normal);
		Misc_ConvertVec3ToF16(rayNegZ, vpnc[ofs + 7].Normal);

		//top Y side (Y+)
		Misc_ConvertVec3ToF16(rayY, vpnc[ofs + 8].Normal);
		Misc_ConvertVec3ToF16(rayY, vpnc[ofs + 9].Normal);
		Misc_ConvertVec3ToF16(rayY, vpnc[ofs + 10].Normal);
		Misc_ConvertVec3ToF16(rayY, vpnc[ofs + 11].Normal);

		//bottom Y side (Y-)
		Misc_ConvertVec3ToF16(rayNegY, vpnc[ofs + 12].Normal);
		Misc_ConvertVec3ToF16(rayNegY, vpnc[ofs + 13].Normal);
		Misc_ConvertVec3ToF16(rayNegY, vpnc[ofs + 14].Normal);
		Misc_ConvertVec3ToF16(rayNegY, vpnc[ofs + 15].Normal);

		//-x side
		Misc_ConvertVec3ToF16(rayNegX, vpnc[ofs + 16].Normal);
		Misc_ConvertVec3ToF16(rayNegX, vpnc[ofs + 17].Normal);
		Misc_ConvertVec3ToF16(rayNegX, vpnc[ofs + 18].Normal);
		Misc_ConvertVec3ToF16(rayNegX, vpnc[ofs + 19].Normal);

		//+x side
		Misc_ConvertVec3ToF16(rayX, vpnc[ofs + 20].Normal);
		Misc_ConvertVec3ToF16(rayX, vpnc[ofs + 21].Normal);
		Misc_ConvertVec3ToF16(rayX, vpnc[ofs + 22].Normal);
		Misc_ConvertVec3ToF16(rayX, vpnc[ofs + 23].Normal);

		//vert colours
		for(int j=0;j < 24;j++)
		{
			Misc_ConvertVec4ToF16(pColours[i], vpnc[ofs + j].Color0);
		}

		//indexes
		uint32_t	idx	=ofs;
		for(int j=0;j < 36;j+=6)
		{
			int	indBox	=i * 36;

			inds[indBox + j]		=idx + 0;
			inds[indBox + j + 1]	=idx + 2;
			inds[indBox + j + 2]	=idx + 1;
			inds[indBox + j + 3]	=idx + 0;
			inds[indBox + j + 4]	=idx + 3;
			inds[indBox + j + 5]	=idx + 2;

			idx	+=4;
		}
	}

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=24 * numRays;
	pObj->mIndexCount	=36 * numRays;

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(VPosNormCol0) * 24 * numRays);
	pObj->mpVB	=GD_CreateBufferWithData(pGD, &bufDesc, vpnc, bufDesc.ByteWidth);

	//make index buffer
	MakeIBDesc(&bufDesc, 36 * 4 * numRays);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, inds, bufDesc.ByteWidth);

	free(vpnc);
	free(inds);

	pObj->mVertSize	=sizeof(VPosNormCol0);

	return	pObj;
}


PrimObject	*PF_CreatePrism(float size, float sizeY, GraphicsDevice *pGD)
{
	//positions
	vec3	topPoint, bottomPoint;
	vec3	top, bottom, left, right;

	//texcoords
	vec2	topPointTex, bottomPointTex;
	vec2	topTex, bottomTex;
	vec2	leftTex, rightTex;

	//normals
	vec3	topUpperLeft	={	1.0f,	1.0f,	1.0f	};
	vec3	topUpperRight	={	-1.0f,	1.0f,	1.0f	};
	vec3	topLowerLeft	={	1.0f,	1.0f,	-1.0f	};
	vec3	topLowerRight	={	-1.0f,	1.0f,	-1.0f	};
	vec3	botUpperLeft	={	1.0f,	-1.0f,	1.0f	};
	vec3	botUpperRight	={	-1.0f,	-1.0f,	1.0f	};
	vec3	botLowerLeft	={	1.0f,	-1.0f,	-1.0f	};
	vec3	botLowerRight	={	-1.0f,	-1.0f,	-1.0f	};

	//verts
	VPosNorm	vpn[24];

	glm_vec3_scale(UnitY, 2 * sizeY, topPoint);
	glm_vec3_zero(bottomPoint);
	
	glm_vec3_scale(UnitY, sizeY, top);
	glm_vec3_muladds(UnitZ, size, top);

	glm_vec3_scale(UnitY, sizeY, bottom);
	glm_vec3_muladds(UnitZ, -size, bottom);

	glm_vec3_scale(UnitY, sizeY, left);
	glm_vec3_muladds(UnitX, size, left);

	glm_vec3_scale(UnitY, sizeY, right);
	glm_vec3_muladds(UnitX, -size, right);

	glm_vec2_scale(One, 0.5f, topPointTex);
	glm_vec2_scale(One, 0.5f, bottomPointTex);
	glm_vec2_scale(UnitX, 0.5f, topTex);
	MulAdd2DestS(UnitX, 0.5f, UnitY, bottomTex);
	glm_vec2_scale(UnitY, 0.5f, leftTex);
	MulAdd2DestS(UnitY, 0.5f, UnitX, rightTex);

	//need to have a lot of duplicates since each
	//vertex will contain a copy of the face normal
	//as we want this to be flat shaded
	Misc_ConvertVec3ToF16(topUpperLeft, vpn[0].Normal);
	Misc_ConvertVec3ToF16(topUpperLeft, vpn[1].Normal);
	Misc_ConvertVec3ToF16(topUpperLeft, vpn[2].Normal);

	Misc_ConvertVec3ToF16(topUpperRight, vpn[3].Normal);
	Misc_ConvertVec3ToF16(topUpperRight, vpn[4].Normal);
	Misc_ConvertVec3ToF16(topUpperRight, vpn[5].Normal);

	Misc_ConvertVec3ToF16(topLowerLeft, vpn[6].Normal);
	Misc_ConvertVec3ToF16(topLowerLeft, vpn[7].Normal);
	Misc_ConvertVec3ToF16(topLowerLeft, vpn[8].Normal);

	Misc_ConvertVec3ToF16(topLowerRight, vpn[9].Normal);
	Misc_ConvertVec3ToF16(topLowerRight, vpn[10].Normal);
	Misc_ConvertVec3ToF16(topLowerRight, vpn[11].Normal);

	Misc_ConvertVec3ToF16(botUpperLeft, vpn[12].Normal);
	Misc_ConvertVec3ToF16(botUpperLeft, vpn[13].Normal);
	Misc_ConvertVec3ToF16(botUpperLeft, vpn[14].Normal);

	Misc_ConvertVec3ToF16(botUpperRight, vpn[15].Normal);
	Misc_ConvertVec3ToF16(botUpperRight, vpn[16].Normal);
	Misc_ConvertVec3ToF16(botUpperRight, vpn[17].Normal);

	Misc_ConvertVec3ToF16(botLowerLeft, vpn[18].Normal);
	Misc_ConvertVec3ToF16(botLowerLeft, vpn[19].Normal);
	Misc_ConvertVec3ToF16(botLowerLeft, vpn[20].Normal);

	Misc_ConvertVec3ToF16(botLowerRight, vpn[21].Normal);
	Misc_ConvertVec3ToF16(botLowerRight, vpn[22].Normal);
	Misc_ConvertVec3ToF16(botLowerRight, vpn[23].Normal);

	//top upper left face
	glm_vec3_copy(topPoint, vpn[0].Position);
	glm_vec3_copy(left, vpn[2].Position);
	glm_vec3_copy(top, vpn[1].Position);

	//top upper right face
	glm_vec3_copy(topPoint, vpn[3].Position);
	glm_vec3_copy(top, vpn[5].Position);
	glm_vec3_copy(right, vpn[4].Position);

	//top lower left face
	glm_vec3_copy(topPoint, vpn[6].Position);
	glm_vec3_copy(bottom, vpn[8].Position);
	glm_vec3_copy(left, vpn[7].Position);

	//top lower right face
	glm_vec3_copy(topPoint, vpn[9].Position);
	glm_vec3_copy(right, vpn[11].Position);
	glm_vec3_copy(bottom, vpn[10].Position);

	//bottom upper left face
	glm_vec3_copy(bottomPoint, vpn[12].Position);
	glm_vec3_copy(top, vpn[14].Position);
	glm_vec3_copy(left, vpn[13].Position);

	//bottom upper right face
	glm_vec3_copy(bottomPoint, vpn[15].Position);
	glm_vec3_copy(right, vpn[17].Position);
	glm_vec3_copy(top, vpn[16].Position);

	//bottom lower left face
	glm_vec3_copy(bottomPoint, vpn[18].Position);
	glm_vec3_copy(left, vpn[20].Position);
	glm_vec3_copy(bottom, vpn[19].Position);

	//bottom lower right face
	glm_vec3_copy(bottomPoint, vpn[21].Position);
	glm_vec3_copy(bottom, vpn[23].Position);
	glm_vec3_copy(right, vpn[22].Position);

	//just reference in order, no verts shared
	uint16_t	idx, indexes[24];
	for(int i=idx=0;i < 24;i++)
	{
		indexes[i]	=idx++;
	}

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=24;
	pObj->mIndexCount	=24;

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(VPosNorm) * 24);
	pObj->mpVB	=GD_CreateBufferWithData(pGD, &bufDesc, vpn, bufDesc.ByteWidth);

	//make index buffer
	MakeIBDesc(&bufDesc, 24 * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, indexes, bufDesc.ByteWidth);

	pObj->mVertSize	=sizeof(VPosNorm);

	return	pObj;
}

PrimObject	*PF_CreateHalfPrism(float size, float sizeY, GraphicsDevice *pGD)
{
	//positions
	vec3	topPoint;
	vec3	top, bottom, left, right;

	//texcoords
	vec2	topPointTex, bottomPointTex;
	vec2	topTex, bottomTex;
	vec2	leftTex, rightTex;

	//normals
	vec3	topUpperLeft	={	1.0f,	1.0f,	1.0f	};
	vec3	topUpperRight	={	-1.0f,	1.0f,	1.0f	};
	vec3	topLowerLeft	={	1.0f,	1.0f,	-1.0f	};
	vec3	topLowerRight	={	-1.0f,	1.0f,	-1.0f	};

	glm_vec3_normalize(topUpperLeft);
	glm_vec3_normalize(topUpperRight);
	glm_vec3_normalize(topLowerLeft);
	glm_vec3_normalize(topLowerRight);

	//all normals are backwards for some strange reason I need to look into
	glm_vec3_inv(topUpperLeft);
	glm_vec3_inv(topUpperRight);
	glm_vec3_inv(topLowerLeft);
	glm_vec3_inv(topLowerRight);

	//verts
	VPosNorm	vpnt[18];

	glm_vec3_zero(topPoint);

	glm_vec3_scale(UnitY, sizeY, top);
	glm_vec3_muladds(UnitZ, size, top);

	glm_vec3_scale(UnitY, sizeY, bottom);
	glm_vec3_muladds(UnitZ, -size, bottom);

	glm_vec3_scale(UnitY, sizeY, left);
	glm_vec3_muladds(UnitX, size, left);

	glm_vec3_scale(UnitY, sizeY, right);
	glm_vec3_muladds(UnitX, -size, right);

	glm_vec2_scale(One, 0.5f, topPointTex);
	glm_vec2_scale(One, 0.5f, bottomPointTex);
	glm_vec2_scale(UnitX, 0.5f, topTex);
	MulAdd2DestS(UnitX, 0.5f, UnitY, bottomTex);
	glm_vec2_scale(UnitY, 0.5f, leftTex);
	MulAdd2DestS(UnitY, 0.5f, UnitX, rightTex);

	//need to have a lot of duplicates since each
	//vertex will contain a copy of the face normal
	//as we want this to be flat shaded
	Misc_ConvertVec3ToF16(topUpperLeft, vpnt[0].Normal);
	Misc_ConvertVec3ToF16(topUpperLeft, vpnt[1].Normal);
	Misc_ConvertVec3ToF16(topUpperLeft, vpnt[2].Normal);

	Misc_ConvertVec3ToF16(topUpperRight, vpnt[3].Normal);
	Misc_ConvertVec3ToF16(topUpperRight, vpnt[4].Normal);
	Misc_ConvertVec3ToF16(topUpperRight, vpnt[5].Normal);

	Misc_ConvertVec3ToF16(topLowerLeft, vpnt[6].Normal);
	Misc_ConvertVec3ToF16(topLowerLeft, vpnt[7].Normal);
	Misc_ConvertVec3ToF16(topLowerLeft, vpnt[8].Normal);

	Misc_ConvertVec3ToF16(topLowerRight, vpnt[9].Normal);
	Misc_ConvertVec3ToF16(topLowerRight, vpnt[10].Normal);
	Misc_ConvertVec3ToF16(topLowerRight, vpnt[11].Normal);

	//bottom face
	Misc_Convert4ToF16(0, 1, 0, 1, vpnt[12].Normal);
	Misc_Convert4ToF16(0, 1, 0, 1, vpnt[13].Normal);
	Misc_Convert4ToF16(0, 1, 0, 1, vpnt[14].Normal);
	Misc_Convert4ToF16(0, 1, 0, 1, vpnt[15].Normal);
	Misc_Convert4ToF16(0, 1, 0, 1, vpnt[16].Normal);
	Misc_Convert4ToF16(0, 1, 0, 1, vpnt[17].Normal);

	//top upper left face
	glm_vec3_copy(topPoint, vpnt[0].Position);
	glm_vec3_copy(left, vpnt[1].Position);
	glm_vec3_copy(top, vpnt[2].Position);

	//top upper right face
	glm_vec3_copy(topPoint, vpnt[3].Position);
	glm_vec3_copy(top, vpnt[4].Position);
	glm_vec3_copy(right, vpnt[5].Position);

	//top lower left face
	glm_vec3_copy(topPoint, vpnt[6].Position);
	glm_vec3_copy(bottom, vpnt[7].Position);
	glm_vec3_copy(left, vpnt[8].Position);

	//top lower right face
	glm_vec3_copy(topPoint, vpnt[9].Position);
	glm_vec3_copy(right, vpnt[10].Position);
	glm_vec3_copy(bottom, vpnt[11].Position);

	//bottom face (2 triangles)
	glm_vec3_copy(top, vpnt[12].Position);
	glm_vec3_copy(bottom, vpnt[13].Position);
	glm_vec3_copy(right, vpnt[14].Position);
	glm_vec3_copy(top, vpnt[15].Position);
	glm_vec3_copy(left, vpnt[16].Position);
	glm_vec3_copy(bottom, vpnt[17].Position);

	//just reference in order, no verts shared
	uint16_t	idx, indexes[18];
	for(int i=idx=0;i < 18;i++)
	{
		indexes[i]	=idx++;
	}

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=18;
	pObj->mIndexCount	=18;

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(VPosNorm) * 18);
	pObj->mpVB	=GD_CreateBufferWithData(pGD, &bufDesc, vpnt, bufDesc.ByteWidth);

	//make index buffer
	MakeIBDesc(&bufDesc, 18 * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, indexes, bufDesc.ByteWidth);

	pObj->mVertSize	=sizeof(VPosNorm);

	return	pObj;
}

PrimObject	*PF_CreateSphere(vec3 center, float radius, GraphicsDevice *pGD)
{
	int	theta, phi;

	//density
	int	dtheta	=15;
	int	dphi	=15;

	//get a vert count
	int	vertCount	=0;
	int	indCount	=0;
	for(theta=-90;theta <= 0-dtheta;theta += dtheta)
	{
		for(phi=0;phi <= 360-dphi;phi += dphi)
		{
			vertCount	+=3;

			if(theta > -90 && theta < 0)
			{
				vertCount++;
				indCount	+=6;
			}
			else
			{
				indCount	+=3;
			}
		}
	}

	//alloc some points
	vec3	*pPoints	=malloc(sizeof(vec3) * vertCount);

	//alloc indexes
	uint16_t	*pInds	=malloc(2 * indCount * 2);

	//build and index a hemisphere
	uint16_t	curIdx	=0;
	int			pIdx	=0;
	int			iIdx	=0;
	for(theta=-90;theta <= 0-dtheta;theta += dtheta)
	{
		for(phi=0;phi <= 360-dphi;phi += dphi)
		{
			vec3	pos;

			glm_vec3_zero(pos);

			float	rtheta	=glm_rad(theta);
			float	rdtheta	=glm_rad(dtheta);
			float	rphi	=glm_rad(phi);
			float	rdphi	=glm_rad(dphi);

			pos[0]	=cosf(rtheta) * cosf(rphi);
			pos[1]	=cosf(rtheta) * sinf(rphi);
			pos[2]	=sinf(rtheta);

			glm_vec3_copy(pos, pPoints[pIdx++]);

			pos[0]	=cosf(rtheta + rdtheta) * cosf(rphi);
			pos[1]	=cosf(rtheta + rdtheta) * sinf(rphi);
			pos[2]	=sinf(rtheta + rdtheta);

			glm_vec3_copy(pos, pPoints[pIdx++]);

			pos[0]	=cosf(rtheta + rdtheta) * cosf(rphi + rdphi);
			pos[1]	=cosf(rtheta + rdtheta) * sinf(rphi + rdphi);
			pos[2]	=sinf(rtheta + rdtheta);

			glm_vec3_copy(pos, pPoints[pIdx++]);

			if(theta > -90 && theta < 0)
			{
				pos[0]	=cosf(rtheta) * cosf(rphi + rdphi);
				pos[1]	=cosf(rtheta) * sinf(rphi + rdphi);
				pos[2]	=sinf(rtheta);

				glm_vec3_copy(pos, pPoints[pIdx++]);

				pInds[iIdx++]	=curIdx;
				pInds[iIdx++]	=curIdx + 2;
				pInds[iIdx++]	=curIdx + 1;
				pInds[iIdx++]	=curIdx;
				pInds[iIdx++]	=curIdx + 3;
				pInds[iIdx++]	=curIdx + 2;

				curIdx	+=4;
			}
			else
			{
				pInds[iIdx++]	=curIdx;
				pInds[iIdx++]	=curIdx + 2;
				pInds[iIdx++]	=curIdx + 1;

				curIdx	+=3;
			}
		}
	}

	//alloc verts
	VPosNorm	*vpn	=malloc(sizeof(VPosNorm) * vertCount * 2);

	//copy in hemisphere
	for(int i=0;i < vertCount;i++)
	{
		vec3	norm;

		glm_vec3_normalize_to(pPoints[i], norm);

		glm_vec3_copy(center, vpn[i].Position);
		glm_vec3_muladds(norm, radius, vpn[i].Position);

		Misc_ConvertVec3ToF16(norm, vpn[i].Normal);
	}

	//dupe for other half
	int	ofs	=vertCount;
	for(int i=ofs;i < vertCount + ofs;i++)
	{
		vec3	norm;

		glm_vec3_normalize_to(pPoints[i - ofs], norm);

		glm_vec3_copy(center, vpn[i].Position);
		glm_vec3_muladds(norm, -radius, vpn[i].Position);

		Misc_Convert4ToF16(-norm[0], -norm[1], -norm[2], 1.0f, vpn[i].Normal);
	}

	//index other half, flip winding
	for(int i=indCount;i < (indCount * 2);i+=3)
	{
		pInds[i]		=vertCount + pInds[i - indCount];
		pInds[i + 1]	=vertCount + pInds[(i + 2) - indCount];
		pInds[i + 2]	=vertCount + pInds[(i + 1) - indCount];
	}

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=vertCount;
	pObj->mIndexCount	=indCount * 2;

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(VPosNorm) * vertCount * 2);
	pObj->mpVB	=GD_CreateBufferWithData(pGD, &bufDesc, vpn, bufDesc.ByteWidth);

	//make index buffer
	MakeIBDesc(&bufDesc, indCount * 2 * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, pInds, bufDesc.ByteWidth);

	//free temp buffers
	free(pPoints);
	free(pInds);
	free(vpn);

	pObj->mVertSize	=sizeof(VPosNorm);

	return	pObj;
}

PrimObject	*PF_CreateCapsule(float radius, float len, GraphicsDevice *pGD)
{
	int	theta, phi;

	//density
	int	dtheta	=20;
	int	dphi	=20;

	//get a vert count
	int	vertCount	=0;
	for(theta=-90;theta <= 0-dtheta;theta += dtheta)
	{
		for(phi=0;phi <= 360-dphi;phi += dphi)
		{
			if(vertCount == 0)
			{
				vertCount++;
			}
			vertCount++;
		}
	}

	//alloc some points
	vec3	*pPoints	=malloc(sizeof(vec3) * vertCount * 2);

	//build and index a hemisphere
	int	pIdx	=0;
	for(theta=-90;theta <= 0-dtheta;theta += dtheta)
	{
		for(phi=0;phi <= 360-dphi;phi += dphi)
		{
			vec3	pos;

			glm_vec3_zero(pos);

			float	rtheta	=glm_rad(theta);
			float	rdtheta	=glm_rad(dtheta);
			float	rphi	=glm_rad(phi);

			if(pIdx == 0)
			{
				pos[0]	=cosf(rtheta) * cosf(rphi);
				pos[2]	=cosf(rtheta) * sinf(rphi);
				pos[1]	=sinf(rtheta);

				glm_vec3_copy(pos, pPoints[pIdx++]);
			}

			pos[0]	=cosf(rtheta + rdtheta) * cosf(rphi);
			pos[2]	=cosf(rtheta + rdtheta) * sinf(rphi);
			pos[1]	=sinf(rtheta + rdtheta);

			glm_vec3_copy(pos, pPoints[pIdx++]);
		}
	}

	//count indexes
	int	indCount	=0;
	for(uint16_t i=1;i < 18;i++)
	{
		indCount	+=3;
	}
	indCount	+=3;
	for(uint16_t i=19;i < 36;i++)
	{
		indCount	+=6;
	}
	indCount	+=6;
	for(uint16_t i=37;i < 54;i++)
	{
		indCount	+=6;
	}
	indCount	+=6;
	for(uint16_t i=55;i < 72;i++)
	{
		indCount	+=6;
	}
	indCount	+=6;
	int	ringOfs	=128 - 55 + 9;
	for(int i=55;i < 63;i++)
	{
		indCount	+=6;
	}
	indCount	+=3;
	ringOfs		=128 - 63;
	for(int i=63;i < 72;i++)
	{
		indCount	+=6;
	}
	indCount	+=3;

	//alloc indexes
	uint16_t	*pInds	=malloc(2 * indCount * 2);

	int	curIdx	=0;
	for(uint16_t i=1;i < 18;i++)
	{
		//base ring
		pInds[curIdx++]	=0;
		pInds[curIdx++]	=i;
		pInds[curIdx++]	=i + 1;
	}

	//final tri
	pInds[curIdx++]	=0;
	pInds[curIdx++]	=18;
	pInds[curIdx++]	=1;		//wrap

	//next ring
	for(uint16_t i=19;i < 36;i++)
	{
		pInds[curIdx++]	=(i - 18);
		pInds[curIdx++]	=i;
		pInds[curIdx++]	=i + 1;

		pInds[curIdx++]	=(i - 18);
		pInds[curIdx++]	=i + 1;
		pInds[curIdx++]	=i - 17;
	}

	//finish quad for this ring
	pInds[curIdx++]	=18;
	pInds[curIdx++]	=36;
	pInds[curIdx++]	=19;

	pInds[curIdx++]	=18;
	pInds[curIdx++]	=19;
	pInds[curIdx++]	=1;		//wrap

	//next ring
	for(uint16_t i=37;i < 54;i++)
	{
		pInds[curIdx++]	=(i - 18);
		pInds[curIdx++]	=i;
		pInds[curIdx++]	=i + 1;

		pInds[curIdx++]	=(i - 18);
		pInds[curIdx++]	=i + 1;
		pInds[curIdx++]	=i - 17;
	}

	//finish quad
	pInds[curIdx++]	=36;
	pInds[curIdx++]	=54;
	pInds[curIdx++]	=37;

	pInds[curIdx++]	=37;
	pInds[curIdx++]	=19;
	pInds[curIdx++]	=36;	//wrap

	//next ring
	for(uint16_t i=55;i < 72;i++)
	{
		pInds[curIdx++]	=(i - 18);
		pInds[curIdx++]	=i;
		pInds[curIdx++]	=i + 1;

		pInds[curIdx++]	=(i - 18);
		pInds[curIdx++]	=i + 1;
		pInds[curIdx++]	=i - 17;
	}

	//finish quad
	pInds[curIdx++]	=54;
	pInds[curIdx++]	=72;
	pInds[curIdx++]	=55;

	pInds[curIdx++]	=54;
	pInds[curIdx++]	=55;
	pInds[curIdx++]	=37;	//wrap

	//rings from renderdoc
	//55 to 72 for the lower index ring
	//128 to 145 for the higher index ring
	//9 is added because the verts are on opposite sides
	ringOfs	=128 - 55 + 9;

	//connect the 2 hemispheres
	//half ring
	for(int i=55;i < 63;i++)
	{
		pInds[curIdx++]	=i;
		pInds[curIdx++]	=i + ringOfs;
		pInds[curIdx++]	=i + ringOfs + 1;

		pInds[curIdx++]	=i;
		pInds[curIdx++]	=i + ringOfs + 1;
		pInds[curIdx++]	=i + 1;
	}

	pInds[curIdx++]	=63;
	pInds[curIdx++]	=145;
	pInds[curIdx++]	=128;	//wrap

	//correct back to opposite side
	ringOfs	=128 - 63;

	//other half
	for(int i=63;i < 72;i++)
	{
		pInds[curIdx++]	=i;
		pInds[curIdx++]	=i + ringOfs;
		pInds[curIdx++]	=i + ringOfs + 1;

		pInds[curIdx++]	=i;
		pInds[curIdx++]	=i + ringOfs + 1;
		pInds[curIdx++]	=i + 1;
	}

	pInds[curIdx++]	=72;
	pInds[curIdx++]	=137;
	pInds[curIdx++]	=55;	//wrap

	//alloc verts
	VPosNorm	*vpn	=malloc(sizeof(VPosNorm) * vertCount * 2);

	//copy in hemisphere
	for(int i=0;i < vertCount;i++)
	{
		vec3	norm;

		glm_vec3_normalize_to(pPoints[i], norm);

		glm_vec3_zero(vpn[i].Position);
		glm_vec3_muladds(norm, radius, vpn[i].Position);

		Misc_ConvertVec3ToF16(norm, vpn[i].Normal);
	}

	//dupe for other half
	int	ofs	=vertCount;
	for(int i=ofs;i < vertCount + ofs;i++)
	{
		vec3	norm;

		glm_vec3_normalize_to(pPoints[i - ofs], norm);

		glm_vec3_zero(vpn[i].Position);
		glm_vec3_muladds(norm, -radius, vpn[i].Position);
		glm_vec3_muladds(UnitY, len, vpn[i].Position);

		Misc_Convert4ToF16(-norm[0], -norm[1], -norm[2], 1.0f, vpn[i].Normal);
	}

	//index other half, flip winding
	for(int i=indCount;i < (indCount * 2);i+=3)
	{
		pInds[i]		=vertCount + pInds[i - indCount];
		pInds[i + 1]	=vertCount + pInds[(i + 2) - indCount];
		pInds[i + 2]	=vertCount + pInds[(i + 1) - indCount];
	}

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=vertCount * 2;
	pObj->mIndexCount	=indCount * 2;

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(VPosNorm) * vertCount * 2);
	pObj->mpVB	=GD_CreateBufferWithData(pGD, &bufDesc, vpn, bufDesc.ByteWidth);

	//make index buffer
	MakeIBDesc(&bufDesc, indCount * 2 * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, pInds, bufDesc.ByteWidth);

	//free temp buffers
	free(pPoints);
	free(pInds);
	free(vpn);

	pObj->mVertSize	=sizeof(VPosNorm);

	return	pObj;
}

//make a drawable convex volume
PrimObject	*PF_CreateCV(const ConvexVolume *pCV, GraphicsDevice *pGD)
{
	Winding	*pCur, *pWinds	=NULL;

	//this var isn't used because I end up counting
	//with foreeach which is lazier
//	int	numWindings	=
		CV_GenerateWindings(pCV, &pWinds);

	pCur	=NULL;

	//count up total verts
	int	totalVerts	=0;
	int	numIdx		=0;
	LL_FOREACH(pWinds, pCur)
	{
		totalVerts	+=pCur->mNumVerts;

		//count indexes
		for(int i=1;i < pCur->mNumVerts - 1;i++)
		{
			numIdx	+=3;
		}
	}

	VPosNorm	*pVerts	=malloc(sizeof(VPosNorm) * totalVerts);
	uint16_t	*pInds	=malloc(sizeof(uint16_t) * numIdx);

	int		cur		=0;
	int		curIdx	=0;
	pCur			=NULL;
	LL_FOREACH(pWinds, pCur)
	{
		vec4	plane;
		PM_FromVerts(pCur->mpVerts, pCur->mNumVerts, plane);

		int	idx	=cur;
		for(int i=0;i < pCur->mNumVerts;i++)
		{
			glm_vec3_copy(pCur->mpVerts[i],	pVerts[cur].Position);

			Misc_ConvertVec4ToF16(plane, pVerts[cur].Normal);
			cur++;
		}

		//indexes
		for(int i=1;i < pCur->mNumVerts - 1;i++)
		{
			pInds[curIdx++]	=idx;
			pInds[curIdx++]	=idx + ((i + 1) % pCur->mNumVerts);
			pInds[curIdx++]	=idx + i;
		}
	}

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=totalVerts;
	pObj->mIndexCount	=numIdx;

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(VPosNorm) * totalVerts);
	pObj->mpVB	=GD_CreateBufferWithData(pGD, &bufDesc, pVerts, bufDesc.ByteWidth);

	//make index buffer
	MakeIBDesc(&bufDesc, numIdx * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, pInds, bufDesc.ByteWidth);

	//free temp buffers
	free(pVerts);
	free(pInds);

	//free windings
	pCur	=NULL;
	Winding	*pTmp;
	LL_FOREACH_SAFE(pWinds, pCur, pTmp)
	{
		LL_DELETE(pWinds, pCur);
		free(pCur);
	}

	pObj->mVertSize	=sizeof(VPosNorm);

	return	pObj;
}