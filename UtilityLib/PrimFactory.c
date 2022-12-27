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

static	vec3	UnitX	={	1.0f, 0.0f, 0.0f	};
static	vec3	UnitY	={	0.0f, 1.0f, 0.0f	};
static	vec3	UnitZ	={	0.0f, 0.0f, 1.0f	};

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
