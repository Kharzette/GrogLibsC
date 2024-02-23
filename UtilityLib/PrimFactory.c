#include	<d3d11_1.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<x86intrin.h>
#include	<cglm/call.h>
#include	"GraphicsDevice.h"
#include	"PrimFactory.h"
#include	"MiscStuff.h"


typedef struct	VPosNormTex0_t
{
	vec3		Position;
	uint16_t	Normal[4];		//16 bit float4
	uint16_t	TexCoord0[2];	//16 bit float2
}	VPosNormTex0;

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


PrimObject	*PF_CreateCubeFromCorners(const vec3 *pCorners, GraphicsDevice *pGD)
{
	VPosNormTex0	vpnt[24];

	//cube corners
	vec3	lowerTopRight, lowerTopLeft, lowerBotRight, lowerBotLeft;
	vec3	upperTopRight, upperTopLeft, upperBotRight, upperBotLeft;

	glmc_vec3_copy(pCorners[0], lowerTopRight);
	glmc_vec3_copy(pCorners[1], lowerTopLeft);
	glmc_vec3_copy(pCorners[2], lowerBotRight);
	glmc_vec3_copy(pCorners[3], lowerBotLeft);
	glmc_vec3_copy(pCorners[4], upperTopRight);
	glmc_vec3_copy(pCorners[5], upperTopLeft);
	glmc_vec3_copy(pCorners[6], upperBotRight);
	glmc_vec3_copy(pCorners[7], upperBotLeft);

	//cube sides
	//top
	glmc_vec3_copy(upperTopLeft,	vpnt[0].Position);
	glmc_vec3_copy(upperTopRight,	vpnt[1].Position);
	glmc_vec3_copy(upperBotRight,	vpnt[2].Position);
	glmc_vec3_copy(upperBotLeft,	vpnt[3].Position);

	//bottom (note reversal)
	glmc_vec3_copy(lowerTopLeft,	vpnt[7].Position);
	glmc_vec3_copy(lowerTopRight,	vpnt[6].Position);
	glmc_vec3_copy(lowerBotRight,	vpnt[5].Position);
	glmc_vec3_copy(lowerBotLeft,	vpnt[4].Position);

	//top z side
	glmc_vec3_copy(upperTopLeft,	vpnt[11].Position);
	glmc_vec3_copy(upperTopRight,	vpnt[10].Position);
	glmc_vec3_copy(lowerTopRight,	vpnt[9].Position);
	glmc_vec3_copy(lowerTopLeft,	vpnt[8].Position);

	//bottom z side
	glmc_vec3_copy(upperBotLeft,	vpnt[12].Position);
	glmc_vec3_copy(upperBotRight,	vpnt[13].Position);
	glmc_vec3_copy(lowerBotRight,	vpnt[14].Position);
	glmc_vec3_copy(lowerBotLeft,	vpnt[15].Position);

	//-x side
	glmc_vec3_copy(upperTopLeft,	vpnt[16].Position);
	glmc_vec3_copy(upperBotLeft,	vpnt[17].Position);
	glmc_vec3_copy(lowerBotLeft,	vpnt[18].Position);
	glmc_vec3_copy(lowerTopLeft,	vpnt[19].Position);

	//+x side
	glmc_vec3_copy(upperTopRight,	vpnt[23].Position);
	glmc_vec3_copy(upperBotRight,	vpnt[22].Position);
	glmc_vec3_copy(lowerBotRight,	vpnt[21].Position);
	glmc_vec3_copy(lowerTopRight,	vpnt[20].Position);

	//normals
	Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpnt[0].Normal);
	Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpnt[1].Normal);
	Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpnt[2].Normal);
	Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpnt[3].Normal);

	Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpnt[4].Normal);
	Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpnt[5].Normal);
	Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpnt[6].Normal);
	Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpnt[7].Normal);

	Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpnt[8].Normal);
	Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpnt[9].Normal);
	Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpnt[10].Normal);
	Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpnt[11].Normal);

	Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpnt[12].Normal);
	Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpnt[13].Normal);
	Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpnt[14].Normal);
	Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpnt[15].Normal);

	Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpnt[16].Normal);
	Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpnt[17].Normal);
	Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpnt[18].Normal);
	Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpnt[19].Normal);

	Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpnt[20].Normal);
	Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpnt[21].Normal);
	Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpnt[22].Normal);
	Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpnt[23].Normal);

	//texcoords
	for(int i=0;i < 24;i+=4)
	{
		Misc_Convert2ToF16(0.0f, 0.0f, vpnt[i].TexCoord0);
		Misc_Convert2ToF16(1.0f, 0.0f, vpnt[i + 1].TexCoord0);
		Misc_Convert2ToF16(1.0f, 1.0f, vpnt[i + 2].TexCoord0);
		Misc_Convert2ToF16(0.0f, 1.0f, vpnt[i + 3].TexCoord0);
	}

	//indexes
	uint16_t	idx, indexes[36];
	for(int i=idx=0;i < 36;i+=6)
	{
		indexes[i]		=idx + 0;
		indexes[i + 1]	=idx + 1;
		indexes[i + 2]	=idx + 2;
		indexes[i + 3]	=idx + 0;
		indexes[i + 4]	=idx + 2;
		indexes[i + 5]	=idx + 3;

		idx	+=4;
	}

	size_t	vpntSize	=sizeof(VPosNormTex0);

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=24;
	pObj->mIndexCount	=36;

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(VPosNormTex0) * 24);
	pObj->mpVB	=GD_CreateBufferWithData(pGD, &bufDesc, vpnt, bufDesc.ByteWidth);

	//make index buffer
	MakeIBDesc(&bufDesc, 36 * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, indexes, bufDesc.ByteWidth);

	return	pObj;
}

PrimObject	*PF_CreateCube(float size, GraphicsDevice *pGD)
{
	vec3	xScaled, yScaled, zScaled;
	vec3	xNeg, yNeg, zNeg;

	glmc_vec3_scale(UnitX, size, xScaled);
	glmc_vec3_scale(UnitY, size, yScaled);
	glmc_vec3_scale(UnitZ, size, zScaled);

	glmc_vec3_flipsign_to(xScaled, xNeg);
	glmc_vec3_flipsign_to(yScaled, yNeg);
	glmc_vec3_flipsign_to(zScaled, zNeg);

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

	return	PF_CreateCubeFromCorners(corners, pGD);
}

PrimObject	*PF_CreateCubesFromBoundArray(const vec3 *pMins, const vec3 *pMaxs, int numBounds, GraphicsDevice *pGD)
{
	VPosNormTex0	*vpnt	=malloc(sizeof(VPosNormTex0) * 24 * numBounds);
	uint32_t		*inds	=malloc(sizeof(uint32_t) * 36 * numBounds);

	for(int i=0;i < numBounds;i++)
	{
		int		ofs	=i * 24;
		vec3	mins, maxs;

		glmc_vec3_copy(pMins[i], mins);
		glmc_vec3_copy(pMaxs[i], maxs);

		//cube corners
		vec3	lowerTopRight, lowerTopLeft, lowerBotRight, lowerBotLeft;
		vec3	upperTopRight, upperTopLeft, upperBotRight, upperBotLeft;

		glmc_vec3_zero(lowerTopRight);
		glmc_vec3_zero(lowerTopLeft);
		glmc_vec3_zero(lowerBotRight);
		glmc_vec3_zero(lowerBotLeft);
		glmc_vec3_zero(upperTopRight);
		glmc_vec3_zero(upperTopLeft);
		glmc_vec3_zero(upperBotRight);
		glmc_vec3_zero(upperBotLeft);

		//generate corners from min/max
		glmc_vec3_muladd(UnitX, maxs, lowerTopRight);
		glmc_vec3_muladd(UnitY, mins, lowerTopRight);
		glmc_vec3_muladd(UnitZ, maxs, lowerTopRight);

		glmc_vec3_muladd(UnitX, mins, lowerTopLeft);
		glmc_vec3_muladd(UnitY, mins, lowerTopLeft);
		glmc_vec3_muladd(UnitZ, maxs, lowerTopLeft);

		glmc_vec3_muladd(UnitX, maxs, lowerBotRight);
		glmc_vec3_muladd(UnitY, mins, lowerBotRight);
		glmc_vec3_muladd(UnitZ, mins, lowerBotRight);

		glmc_vec3_muladd(UnitX, mins, lowerBotLeft);
		glmc_vec3_muladd(UnitY, mins, lowerBotLeft);
		glmc_vec3_muladd(UnitZ, mins, lowerBotLeft);

		glmc_vec3_muladd(UnitX, maxs, upperTopRight);
		glmc_vec3_muladd(UnitY, maxs, upperTopRight);
		glmc_vec3_muladd(UnitZ, maxs, upperTopRight);

		glmc_vec3_muladd(UnitX, mins, upperTopLeft);
		glmc_vec3_muladd(UnitY, maxs, upperTopLeft);
		glmc_vec3_muladd(UnitZ, maxs, upperTopLeft);

		glmc_vec3_muladd(UnitX, maxs, upperBotRight);
		glmc_vec3_muladd(UnitY, maxs, upperBotRight);
		glmc_vec3_muladd(UnitZ, mins, upperBotRight);

		glmc_vec3_muladd(UnitX, mins, upperBotLeft);
		glmc_vec3_muladd(UnitY, maxs, upperBotLeft);
		glmc_vec3_muladd(UnitZ, mins, upperBotLeft);

		//cube sides
		//top
		glmc_vec3_copy(upperTopLeft,	vpnt[ofs + 0].Position);
		glmc_vec3_copy(upperTopRight,	vpnt[ofs + 1].Position);
		glmc_vec3_copy(upperBotRight,	vpnt[ofs + 2].Position);
		glmc_vec3_copy(upperBotLeft,	vpnt[ofs + 3].Position);

		//bottom (note reversal)
		glmc_vec3_copy(lowerTopLeft,	vpnt[ofs + 7].Position);
		glmc_vec3_copy(lowerTopRight,	vpnt[ofs + 6].Position);
		glmc_vec3_copy(lowerBotRight,	vpnt[ofs + 5].Position);
		glmc_vec3_copy(lowerBotLeft,	vpnt[ofs + 4].Position);

		//top z side
		glmc_vec3_copy(upperTopLeft,	vpnt[ofs + 11].Position);
		glmc_vec3_copy(upperTopRight,	vpnt[ofs + 10].Position);
		glmc_vec3_copy(lowerTopRight,	vpnt[ofs + 9].Position);
		glmc_vec3_copy(lowerTopLeft,	vpnt[ofs + 8].Position);

		//bottom z side
		glmc_vec3_copy(upperBotLeft,	vpnt[ofs + 12].Position);
		glmc_vec3_copy(upperBotRight,	vpnt[ofs + 13].Position);
		glmc_vec3_copy(lowerBotRight,	vpnt[ofs + 14].Position);
		glmc_vec3_copy(lowerBotLeft,	vpnt[ofs + 15].Position);

		//-x side
		glmc_vec3_copy(upperTopLeft,	vpnt[ofs + 16].Position);
		glmc_vec3_copy(upperBotLeft,	vpnt[ofs + 17].Position);
		glmc_vec3_copy(lowerBotLeft,	vpnt[ofs + 18].Position);
		glmc_vec3_copy(lowerTopLeft,	vpnt[ofs + 19].Position);

		//+x side
		glmc_vec3_copy(upperTopRight,	vpnt[ofs + 23].Position);
		glmc_vec3_copy(upperBotRight,	vpnt[ofs + 22].Position);
		glmc_vec3_copy(lowerBotRight,	vpnt[ofs + 21].Position);
		glmc_vec3_copy(lowerTopRight,	vpnt[ofs + 20].Position);
		
		//normals
		Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpnt[ofs + 0].Normal);
		Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpnt[ofs + 1].Normal);
		Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpnt[ofs + 2].Normal);
		Misc_Convert4ToF16(0.0f, 1.0f, 0.0f, 1.0f, vpnt[ofs + 3].Normal);

		Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpnt[ofs + 4].Normal);
		Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpnt[ofs + 5].Normal);
		Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpnt[ofs + 6].Normal);
		Misc_Convert4ToF16(0.0f, -1.0f, 0.0f, 1.0f, vpnt[ofs + 7].Normal);

		Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpnt[ofs + 8].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpnt[ofs + 9].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpnt[ofs + 10].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, 1.0f, 1.0f, vpnt[ofs + 11].Normal);

		Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpnt[ofs + 12].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpnt[ofs + 13].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpnt[ofs + 14].Normal);
		Misc_Convert4ToF16(0.0f, 0.0f, -1.0f, 1.0f, vpnt[ofs + 15].Normal);

		Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpnt[ofs + 16].Normal);
		Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpnt[ofs + 17].Normal);
		Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpnt[ofs + 18].Normal);
		Misc_Convert4ToF16(-1.0f, 0.0f, 0.0f, 1.0f, vpnt[ofs + 19].Normal);

		Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpnt[ofs + 20].Normal);
		Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpnt[ofs + 21].Normal);
		Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpnt[ofs + 22].Normal);
		Misc_Convert4ToF16(1.0f, 0.0f, 0.0f, 1.0f, vpnt[ofs + 23].Normal);

		//texcoords
		for(int j=0;j < 24;j+=4)
		{
			Misc_Convert2ToF16(0.0f, 0.0f, vpnt[ofs + j].TexCoord0);
			Misc_Convert2ToF16(1.0f, 0.0f, vpnt[ofs + j + 1].TexCoord0);
			Misc_Convert2ToF16(1.0f, 1.0f, vpnt[ofs + j + 2].TexCoord0);
			Misc_Convert2ToF16(0.0f, 1.0f, vpnt[ofs + j + 3].TexCoord0);
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

	size_t	vpntSize	=sizeof(VPosNormTex0);

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=24 * numBounds;
	pObj->mIndexCount	=36 * numBounds;

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(VPosNormTex0) * 24 * numBounds);
	pObj->mpVB	=GD_CreateBufferWithData(pGD, &bufDesc, vpnt, bufDesc.ByteWidth);

	//make index buffer
	MakeIBDesc(&bufDesc, 36 * 4 * numBounds);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, inds, bufDesc.ByteWidth);

	free(vpnt);
	free(inds);

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
	VPosNormTex0	vpnt[24];

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

	Misc_ConvertVec3ToF16(botUpperLeft, vpnt[12].Normal);
	Misc_ConvertVec3ToF16(botUpperLeft, vpnt[13].Normal);
	Misc_ConvertVec3ToF16(botUpperLeft, vpnt[14].Normal);

	Misc_ConvertVec3ToF16(botUpperRight, vpnt[15].Normal);
	Misc_ConvertVec3ToF16(botUpperRight, vpnt[16].Normal);
	Misc_ConvertVec3ToF16(botUpperRight, vpnt[17].Normal);

	Misc_ConvertVec3ToF16(botLowerLeft, vpnt[18].Normal);
	Misc_ConvertVec3ToF16(botLowerLeft, vpnt[19].Normal);
	Misc_ConvertVec3ToF16(botLowerLeft, vpnt[20].Normal);

	Misc_ConvertVec3ToF16(botLowerRight, vpnt[21].Normal);
	Misc_ConvertVec3ToF16(botLowerRight, vpnt[22].Normal);
	Misc_ConvertVec3ToF16(botLowerRight, vpnt[23].Normal);

	//top upper left face
	glm_vec3_copy(topPoint, vpnt[0].Position);
	glm_vec3_copy(left, vpnt[2].Position);
	glm_vec3_copy(top, vpnt[1].Position);

	//top upper right face
	glm_vec3_copy(topPoint, vpnt[3].Position);
	glm_vec3_copy(top, vpnt[5].Position);
	glm_vec3_copy(right, vpnt[4].Position);

	//top lower left face
	glm_vec3_copy(topPoint, vpnt[6].Position);
	glm_vec3_copy(bottom, vpnt[8].Position);
	glm_vec3_copy(left, vpnt[7].Position);

	//top lower right face
	glm_vec3_copy(topPoint, vpnt[9].Position);
	glm_vec3_copy(right, vpnt[11].Position);
	glm_vec3_copy(bottom, vpnt[10].Position);

	//bottom upper left face
	glm_vec3_copy(bottomPoint, vpnt[12].Position);
	glm_vec3_copy(top, vpnt[14].Position);
	glm_vec3_copy(left, vpnt[13].Position);

	//bottom upper right face
	glm_vec3_copy(bottomPoint, vpnt[15].Position);
	glm_vec3_copy(right, vpnt[17].Position);
	glm_vec3_copy(top, vpnt[16].Position);

	//bottom lower left face
	glm_vec3_copy(bottomPoint, vpnt[18].Position);
	glm_vec3_copy(left, vpnt[20].Position);
	glm_vec3_copy(bottom, vpnt[19].Position);

	//bottom lower right face
	glm_vec3_copy(bottomPoint, vpnt[21].Position);
	glm_vec3_copy(bottom, vpnt[23].Position);
	glm_vec3_copy(right, vpnt[22].Position);

	Misc_ConvertVec2ToF16(topPointTex, vpnt[0].TexCoord0);
	Misc_ConvertVec2ToF16(leftTex, vpnt[1].TexCoord0);
	Misc_ConvertVec2ToF16(topTex, vpnt[2].TexCoord0);
	Misc_ConvertVec2ToF16(topPointTex, vpnt[3].TexCoord0);
	Misc_ConvertVec2ToF16(topTex, vpnt[4].TexCoord0);
	Misc_ConvertVec2ToF16(rightTex, vpnt[5].TexCoord0);
	Misc_ConvertVec2ToF16(topPointTex, vpnt[6].TexCoord0);
	Misc_ConvertVec2ToF16(bottomTex, vpnt[7].TexCoord0);
	Misc_ConvertVec2ToF16(leftTex, vpnt[8].TexCoord0);
	Misc_ConvertVec2ToF16(topPointTex, vpnt[9].TexCoord0);
	Misc_ConvertVec2ToF16(rightTex, vpnt[10].TexCoord0);
	Misc_ConvertVec2ToF16(bottomTex, vpnt[11].TexCoord0);
	Misc_ConvertVec2ToF16(bottomPointTex, vpnt[12].TexCoord0);
	Misc_ConvertVec2ToF16(leftTex, vpnt[13].TexCoord0);
	Misc_ConvertVec2ToF16(topTex, vpnt[14].TexCoord0);
	Misc_ConvertVec2ToF16(bottomPointTex, vpnt[15].TexCoord0);
	Misc_ConvertVec2ToF16(topTex, vpnt[16].TexCoord0);
	Misc_ConvertVec2ToF16(rightTex, vpnt[17].TexCoord0);
	Misc_ConvertVec2ToF16(bottomPointTex, vpnt[18].TexCoord0);
	Misc_ConvertVec2ToF16(bottomTex, vpnt[19].TexCoord0);
	Misc_ConvertVec2ToF16(leftTex, vpnt[20].TexCoord0);
	Misc_ConvertVec2ToF16(bottomPointTex, vpnt[21].TexCoord0);
	Misc_ConvertVec2ToF16(rightTex, vpnt[22].TexCoord0);
	Misc_ConvertVec2ToF16(bottomTex, vpnt[23].TexCoord0);

	//just reference in order, no verts shared
	uint16_t	idx, indexes[24];
	for(int i=idx=0;i < 24;i++)
	{
		indexes[i]	=idx++;
	}

	size_t	vpntSize	=sizeof(VPosNormTex0);

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=24;
	pObj->mIndexCount	=24;

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(VPosNormTex0) * 24);
	pObj->mpVB	=GD_CreateBufferWithData(pGD, &bufDesc, vpnt, bufDesc.ByteWidth);

	//make index buffer
	MakeIBDesc(&bufDesc, 24 * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, indexes, bufDesc.ByteWidth);

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

	//verts
	VPosNormTex0	vpnt[24];

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

	Misc_ConvertVec2ToF16(topPointTex, vpnt[0].TexCoord0);
	Misc_ConvertVec2ToF16(leftTex, vpnt[1].TexCoord0);
	Misc_ConvertVec2ToF16(topTex, vpnt[2].TexCoord0);
	Misc_ConvertVec2ToF16(topPointTex, vpnt[3].TexCoord0);
	Misc_ConvertVec2ToF16(topTex, vpnt[4].TexCoord0);
	Misc_ConvertVec2ToF16(rightTex, vpnt[5].TexCoord0);
	Misc_ConvertVec2ToF16(topPointTex, vpnt[6].TexCoord0);
	Misc_ConvertVec2ToF16(bottomTex, vpnt[7].TexCoord0);
	Misc_ConvertVec2ToF16(leftTex, vpnt[8].TexCoord0);
	Misc_ConvertVec2ToF16(topPointTex, vpnt[9].TexCoord0);
	Misc_ConvertVec2ToF16(rightTex, vpnt[10].TexCoord0);
	Misc_ConvertVec2ToF16(bottomTex, vpnt[11].TexCoord0);

	//just reference in order, no verts shared
	uint16_t	idx, indexes[12];
	for(int i=idx=0;i < 12;i++)
	{
		indexes[i]	=idx++;
	}

	size_t	vpntSize	=sizeof(VPosNormTex0);

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=12;
	pObj->mIndexCount	=12;

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(VPosNormTex0) * 12);
	pObj->mpVB	=GD_CreateBufferWithData(pGD, &bufDesc, vpnt, bufDesc.ByteWidth);

	//make index buffer
	MakeIBDesc(&bufDesc, 12 * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, indexes, bufDesc.ByteWidth);

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
	VPosNormTex0	*vpnt	=malloc(sizeof(VPosNormTex0) * vertCount * 2);

	//copy in hemisphere
	for(int i=0;i < vertCount;i++)
	{
		vec3	norm;

		glm_vec3_normalize_to(pPoints[i], norm);

		glm_vec3_copy(center, vpnt[i].Position);
		glm_vec3_muladds(norm, radius, vpnt[i].Position);

		//not tackling this yet
		Misc_Convert2ToF16(0.0f, 0.0f, vpnt[i].TexCoord0);

		Misc_ConvertVec3ToF16(norm, vpnt[i].Normal);
	}

	//dupe for other half
	int	ofs	=vertCount;
	for(int i=ofs;i < vertCount + ofs;i++)
	{
		vec3	norm;

		glm_vec3_normalize_to(pPoints[i - ofs], norm);

		glm_vec3_copy(center, vpnt[i].Position);
		glm_vec3_muladds(norm, -radius, vpnt[i].Position);

		//not tackling this yet
		Misc_Convert2ToF16(0.0f, 0.0f, vpnt[i].TexCoord0);

		Misc_Convert4ToF16(-norm[0], -norm[1], -norm[2], 1.0f, vpnt[i].Normal);
	}

	//index other half, flip winding
	for(int i=indCount;i < (indCount * 2);i+=3)
	{
		pInds[i]		=vertCount + pInds[i - indCount];
		pInds[i + 1]	=vertCount + pInds[(i + 2) - indCount];
		pInds[i + 2]	=vertCount + pInds[(i + 1) - indCount];
	}

	size_t	vpntSize	=sizeof(VPosNormTex0);

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=vertCount;
	pObj->mIndexCount	=indCount * 2;

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(VPosNormTex0) * vertCount * 2);
	pObj->mpVB	=GD_CreateBufferWithData(pGD, &bufDesc, vpnt, bufDesc.ByteWidth);

	//make index buffer
	MakeIBDesc(&bufDesc, indCount * 2 * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, pInds, bufDesc.ByteWidth);

	//free temp buffers
	free(pPoints);
	free(pInds);
	free(vpnt);

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
			float	rdphi	=glm_rad(dphi);

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
	VPosNormTex0	*vpnt	=malloc(sizeof(VPosNormTex0) * vertCount * 2);

	//copy in hemisphere
	for(int i=0;i < vertCount;i++)
	{
		vec3	norm;

		glm_vec3_normalize_to(pPoints[i], norm);

		glm_vec3_zero(vpnt[i].Position);
		glm_vec3_muladds(norm, radius, vpnt[i].Position);

		//not tackling this yet
		Misc_Convert2ToF16(0.0f, 0.0f, vpnt[i].TexCoord0);

		Misc_ConvertVec3ToF16(norm, vpnt[i].Normal);
	}

	//dupe for other half
	int	ofs	=vertCount;
	for(int i=ofs;i < vertCount + ofs;i++)
	{
		vec3	norm;

		glm_vec3_normalize_to(pPoints[i - ofs], norm);

		glm_vec3_zero(vpnt[i].Position);
		glm_vec3_muladds(norm, -radius, vpnt[i].Position);
		glm_vec3_muladds(UnitY, len, vpnt[i].Position);

		//not tackling this yet
		Misc_Convert2ToF16(0.0f, 0.0f, vpnt[i].TexCoord0);

		Misc_Convert4ToF16(-norm[0], -norm[1], -norm[2], 1.0f, vpnt[i].Normal);
	}

	//index other half, flip winding
	for(int i=indCount;i < (indCount * 2);i+=3)
	{
		pInds[i]		=vertCount + pInds[i - indCount];
		pInds[i + 1]	=vertCount + pInds[(i + 2) - indCount];
		pInds[i + 2]	=vertCount + pInds[(i + 1) - indCount];
	}

	size_t	vpntSize	=sizeof(VPosNormTex0);

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=vertCount * 2;
	pObj->mIndexCount	=indCount * 2;
//	pObj->mIndexCount	=270;

	//make vertex buffer
	D3D11_BUFFER_DESC	bufDesc;
	MakeVBDesc(&bufDesc, sizeof(VPosNormTex0) * vertCount * 2);
	pObj->mpVB	=GD_CreateBufferWithData(pGD, &bufDesc, vpnt, bufDesc.ByteWidth);

	//make index buffer
	MakeIBDesc(&bufDesc, indCount * 2 * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, pInds, bufDesc.ByteWidth);

	//free temp buffers
	free(pPoints);
	free(pInds);
	free(vpnt);

	return	pObj;
}