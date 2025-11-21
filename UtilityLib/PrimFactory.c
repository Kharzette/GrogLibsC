#include	<d3d11_1.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<assert.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<utlist.h>
#include	<cglm/call.h>
#include	"GraphicsDevice.h"
#include	"PrimFactory.h"
#include	"MiscStuff.h"
#include	"PlaneMath.h"


typedef struct	VPosNormCol_t
{
	vec4		PositionU;		//U in w
	uint32_t	NormVCol[4];	//packed norm and col in F16
}	VPosNormCol;

static const	vec3	UnitX	={	1.0f, 0.0f, 0.0f	};
static const	vec3	UnitY	={	0.0f, 1.0f, 0.0f	};
static const	vec3	UnitZ	={	0.0f, 0.0f, 1.0f	};
static const	vec3	One		={	1.0f, 1.0f, 1.0f	};

//static forward decs
static void	sThreeXYZ(const vec3 x, const vec3 y, const vec3 z, vec3 dest);
static void	sMakeIBDesc(D3D11_BUFFER_DESC *pDesc, uint32_t byteSize);
static void	sMulAdd2DestS(const vec2 a, const float b, const vec2 c, vec2 dest);
static void	sMakeStructuredBuffer(GraphicsDevice *pGD,
	int structSize, int numItems, void *pVData,
	ID3D11Buffer **ppBuffer, ID3D11ShaderResourceView **ppSRV);


void	PF_DestroyPO(PrimObject **ppObj)
{
	//release buffers
	(*ppObj)->mpIB->lpVtbl->Release((*ppObj)->mpIB);
	(*ppObj)->mpVB->lpVtbl->Release((*ppObj)->mpVB);
	(*ppObj)->mpVBSRV->lpVtbl->Release((*ppObj)->mpVBSRV);

	//free mem
	free(*ppObj);

	*ppObj	=NULL;
}


PrimObject	*PF_CreateCubeFromBounds(const vec3 min, const vec3 max, GraphicsDevice *pGD)
{
	vec3	corners[8];

	sThreeXYZ(max, min, max, corners[0]);
	sThreeXYZ(min, min, max, corners[1]);
	sThreeXYZ(max, min, min, corners[2]);
	sThreeXYZ(min, min, min, corners[3]);
	sThreeXYZ(max, max, max, corners[4]);
	sThreeXYZ(min, max, max, corners[5]);
	sThreeXYZ(max, max, min, corners[6]);
	sThreeXYZ(min, max, min, corners[7]);

	return	PF_CreateCubeFromCorners(corners, false, pGD);
}

PrimObject	*PF_CreateCubeFromCorners(const vec3 *pCorners, bool bFlipped, GraphicsDevice *pGD)
{
	VPosNormCol	vpn[24];

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
	glm_vec3_copy(upperTopLeft,		vpn[0].PositionU);
	glm_vec3_copy(upperTopRight,	vpn[1].PositionU);
	glm_vec3_copy(upperBotRight,	vpn[2].PositionU);
	glm_vec3_copy(upperBotLeft,		vpn[3].PositionU);

	//bottom (note reversal)
	glm_vec3_copy(lowerTopLeft,		vpn[7].PositionU);
	glm_vec3_copy(lowerTopRight,	vpn[6].PositionU);
	glm_vec3_copy(lowerBotRight,	vpn[5].PositionU);
	glm_vec3_copy(lowerBotLeft,		vpn[4].PositionU);

	//top z side
	glm_vec3_copy(upperTopLeft,		vpn[11].PositionU);
	glm_vec3_copy(upperTopRight,	vpn[10].PositionU);
	glm_vec3_copy(lowerTopRight,	vpn[9].PositionU);
	glm_vec3_copy(lowerTopLeft,		vpn[8].PositionU);

	//bottom z side
	glm_vec3_copy(upperBotLeft,		vpn[12].PositionU);
	glm_vec3_copy(upperBotRight,	vpn[13].PositionU);
	glm_vec3_copy(lowerBotRight,	vpn[14].PositionU);
	glm_vec3_copy(lowerBotLeft,		vpn[15].PositionU);

	//-x side
	glm_vec3_copy(upperTopLeft,		vpn[16].PositionU);
	glm_vec3_copy(upperBotLeft,		vpn[17].PositionU);
	glm_vec3_copy(lowerBotLeft,		vpn[18].PositionU);
	glm_vec3_copy(lowerTopLeft,		vpn[19].PositionU);

	//+x side
	glm_vec3_copy(upperTopRight,	vpn[23].PositionU);
	glm_vec3_copy(upperBotRight,	vpn[22].PositionU);
	glm_vec3_copy(lowerBotRight,	vpn[21].PositionU);
	glm_vec3_copy(lowerTopRight,	vpn[20].PositionU);

	vec4	colWhite	={	1,1,1,1	};

	//normals
	Misc_InterleaveVec4ToF16((vec4){	0.0f, 1.0f, 0.0f, 1.0f	}, colWhite, vpn[0].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	0.0f, 1.0f, 0.0f, 1.0f	}, colWhite, vpn[1].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	0.0f, 1.0f, 0.0f, 1.0f	}, colWhite, vpn[2].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	0.0f, 1.0f, 0.0f, 1.0f	}, colWhite, vpn[3].NormVCol);

	Misc_InterleaveVec4ToF16((vec4){	0.0f, -1.0f, 0.0f, 1.0f	}, colWhite, vpn[4].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	0.0f, -1.0f, 0.0f, 1.0f	}, colWhite, vpn[5].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	0.0f, -1.0f, 0.0f, 1.0f	}, colWhite, vpn[6].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	0.0f, -1.0f, 0.0f, 1.0f	}, colWhite, vpn[7].NormVCol);

	Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, 1.0f, 1.0f	}, colWhite, vpn[8].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, 1.0f, 1.0f	}, colWhite, vpn[9].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, 1.0f, 1.0f	}, colWhite, vpn[10].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, 1.0f, 1.0f	}, colWhite, vpn[11].NormVCol);

	Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, -1.0f, 1.0f	}, colWhite, vpn[12].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, -1.0f, 1.0f	}, colWhite, vpn[13].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, -1.0f, 1.0f	}, colWhite, vpn[14].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, -1.0f, 1.0f	}, colWhite, vpn[15].NormVCol);

	Misc_InterleaveVec4ToF16((vec4){	-1.0f, 0.0f, 0.0f, 1.0f	}, colWhite, vpn[16].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	-1.0f, 0.0f, 0.0f, 1.0f	}, colWhite, vpn[17].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	-1.0f, 0.0f, 0.0f, 1.0f	}, colWhite, vpn[18].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	-1.0f, 0.0f, 0.0f, 1.0f	}, colWhite, vpn[19].NormVCol);

	Misc_InterleaveVec4ToF16((vec4){	1.0f, 1.0f, 0.0f, 1.0f	}, colWhite, vpn[20].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	1.0f, 1.0f, 0.0f, 1.0f	}, colWhite, vpn[21].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	1.0f, 1.0f, 0.0f, 1.0f	}, colWhite, vpn[22].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){	1.0f, 1.0f, 0.0f, 1.0f	}, colWhite, vpn[23].NormVCol);

	//indexes
	uint16_t	idx, indexes[36];
	for(int i=idx=0;i < 36;i+=6)
	{
		if(!bFlipped)
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

	sMakeStructuredBuffer(pGD, sizeof(VPosNormCol), 24, vpn,
		&pObj->mpVB, &pObj->mpVBSRV);

	//make index buffer
	D3D11_BUFFER_DESC	bufDesc;
	sMakeIBDesc(&bufDesc, 36 * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, indexes, bufDesc.ByteWidth);

	pObj->mVertSize	=sizeof(VPosNormCol);

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
	VPosNormCol	*vpnc	=malloc(sizeof(VPosNormCol) * 24 * numBounds);
	uint32_t	*inds	=malloc(sizeof(uint32_t) * 36 * numBounds);

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
		glm_vec3_copy(upperTopLeft,		vpnc[ofs + 0].PositionU);
		glm_vec3_copy(upperTopRight,	vpnc[ofs + 1].PositionU);
		glm_vec3_copy(upperBotRight,	vpnc[ofs + 2].PositionU);
		glm_vec3_copy(upperBotLeft,		vpnc[ofs + 3].PositionU);

		//bottom (note reversal)
		glm_vec3_copy(lowerTopLeft,		vpnc[ofs + 7].PositionU);
		glm_vec3_copy(lowerTopRight,	vpnc[ofs + 6].PositionU);
		glm_vec3_copy(lowerBotRight,	vpnc[ofs + 5].PositionU);
		glm_vec3_copy(lowerBotLeft,		vpnc[ofs + 4].PositionU);

		//top z side
		glm_vec3_copy(upperTopLeft,		vpnc[ofs + 11].PositionU);
		glm_vec3_copy(upperTopRight,	vpnc[ofs + 10].PositionU);
		glm_vec3_copy(lowerTopRight,	vpnc[ofs + 9].PositionU);
		glm_vec3_copy(lowerTopLeft,		vpnc[ofs + 8].PositionU);

		//bottom z side
		glm_vec3_copy(upperBotLeft,		vpnc[ofs + 12].PositionU);
		glm_vec3_copy(upperBotRight,	vpnc[ofs + 13].PositionU);
		glm_vec3_copy(lowerBotRight,	vpnc[ofs + 14].PositionU);
		glm_vec3_copy(lowerBotLeft,		vpnc[ofs + 15].PositionU);

		//-x side
		glm_vec3_copy(upperTopLeft,		vpnc[ofs + 16].PositionU);
		glm_vec3_copy(upperBotLeft,		vpnc[ofs + 17].PositionU);
		glm_vec3_copy(lowerBotLeft,		vpnc[ofs + 18].PositionU);
		glm_vec3_copy(lowerTopLeft,		vpnc[ofs + 19].PositionU);

		//+x side
		glm_vec3_copy(upperTopRight,	vpnc[ofs + 23].PositionU);
		glm_vec3_copy(upperBotRight,	vpnc[ofs + 22].PositionU);
		glm_vec3_copy(lowerBotRight,	vpnc[ofs + 21].PositionU);
		glm_vec3_copy(lowerTopRight,	vpnc[ofs + 20].PositionU);
		
		//normals
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 1.0f, 0.0f, 1.0f	}, col, vpnc[0].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 1.0f, 0.0f, 1.0f	}, col, vpnc[1].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 1.0f, 0.0f, 1.0f	}, col, vpnc[2].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 1.0f, 0.0f, 1.0f	}, col, vpnc[3].NormVCol);

		Misc_InterleaveVec4ToF16((vec4){	0.0f, -1.0f, 0.0f, 1.0f	}, col, vpnc[4].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, -1.0f, 0.0f, 1.0f	}, col, vpnc[5].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, -1.0f, 0.0f, 1.0f	}, col, vpnc[6].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, -1.0f, 0.0f, 1.0f	}, col, vpnc[7].NormVCol);

		Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, 1.0f, 1.0f	}, col, vpnc[8].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, 1.0f, 1.0f	}, col, vpnc[9].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, 1.0f, 1.0f	}, col, vpnc[10].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, 1.0f, 1.0f	}, col, vpnc[11].NormVCol);

		Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, -1.0f, 1.0f	}, col, vpnc[12].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, -1.0f, 1.0f	}, col, vpnc[13].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, -1.0f, 1.0f	}, col, vpnc[14].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, -1.0f, 1.0f	}, col, vpnc[15].NormVCol);

		Misc_InterleaveVec4ToF16((vec4){	-1.0f, 0.0f, 0.0f, 1.0f	}, col, vpnc[16].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	-1.0f, 0.0f, 0.0f, 1.0f	}, col, vpnc[17].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	-1.0f, 0.0f, 0.0f, 1.0f	}, col, vpnc[18].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	-1.0f, 0.0f, 0.0f, 1.0f	}, col, vpnc[19].NormVCol);

		Misc_InterleaveVec4ToF16((vec4){	1.0f, 1.0f, 0.0f, 1.0f	}, col, vpnc[20].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	1.0f, 1.0f, 0.0f, 1.0f	}, col, vpnc[21].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	1.0f, 1.0f, 0.0f, 1.0f	}, col, vpnc[22].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	1.0f, 1.0f, 0.0f, 1.0f	}, col, vpnc[23].NormVCol);

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

	pObj->mVertCount	=24 * numBounds;
	pObj->mIndexCount	=36 * numBounds;

	//make index buffer
	D3D11_BUFFER_DESC	bufDesc;
	sMakeIBDesc(&bufDesc, 36 * 4 * numBounds);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, inds, bufDesc.ByteWidth);

	sMakeStructuredBuffer(pGD, sizeof(VPosNormCol), 24 * numBounds,
							vpnc, &pObj->mpVB, &pObj->mpVBSRV);

	free(vpnc);
	free(inds);

	pObj->mVertSize	=sizeof(VPosNormCol);

	return	pObj;
}

PrimObject	*PF_CreateManyCubes(const vec3 *pCubeCenters, int numCubes,
								float size, GraphicsDevice *pGD)
{
	VPosNormCol	*vpn	=malloc(sizeof(VPosNormCol) * 24 * numCubes);
	uint32_t	*inds	=malloc(sizeof(uint32_t) * 36 * numCubes);

	vec3	sizeVec;
	glm_vec3_scale(GLM_VEC3_ONE, size, sizeVec);

	for(int i=0;i < numCubes;i++)
	{
		int		ofs	=i * 24;
		vec3	mins, maxs;
		vec4	col;

		Misc_RandomColour(col);

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
		glm_vec3_copy(upperTopLeft,		vpn[ofs + 0].PositionU);
		glm_vec3_copy(upperTopRight,	vpn[ofs + 1].PositionU);
		glm_vec3_copy(upperBotRight,	vpn[ofs + 2].PositionU);
		glm_vec3_copy(upperBotLeft,		vpn[ofs + 3].PositionU);

		//bottom (note reversal)
		glm_vec3_copy(lowerTopLeft,		vpn[ofs + 7].PositionU);
		glm_vec3_copy(lowerTopRight,	vpn[ofs + 6].PositionU);
		glm_vec3_copy(lowerBotRight,	vpn[ofs + 5].PositionU);
		glm_vec3_copy(lowerBotLeft,		vpn[ofs + 4].PositionU);

		//top z side
		glm_vec3_copy(upperTopLeft,		vpn[ofs + 11].PositionU);
		glm_vec3_copy(upperTopRight,	vpn[ofs + 10].PositionU);
		glm_vec3_copy(lowerTopRight,	vpn[ofs + 9].PositionU);
		glm_vec3_copy(lowerTopLeft,		vpn[ofs + 8].PositionU);

		//bottom z side
		glm_vec3_copy(upperBotLeft,		vpn[ofs + 12].PositionU);
		glm_vec3_copy(upperBotRight,	vpn[ofs + 13].PositionU);
		glm_vec3_copy(lowerBotRight,	vpn[ofs + 14].PositionU);
		glm_vec3_copy(lowerBotLeft,		vpn[ofs + 15].PositionU);

		//-x side
		glm_vec3_copy(upperTopLeft,		vpn[ofs + 16].PositionU);
		glm_vec3_copy(upperBotLeft,		vpn[ofs + 17].PositionU);
		glm_vec3_copy(lowerBotLeft,		vpn[ofs + 18].PositionU);
		glm_vec3_copy(lowerTopLeft,		vpn[ofs + 19].PositionU);

		//+x side
		glm_vec3_copy(upperTopRight,	vpn[ofs + 23].PositionU);
		glm_vec3_copy(upperBotRight,	vpn[ofs + 22].PositionU);
		glm_vec3_copy(lowerBotRight,	vpn[ofs + 21].PositionU);
		glm_vec3_copy(lowerTopRight,	vpn[ofs + 20].PositionU);
		
		//normals
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 1.0f, 0.0f, 1.0f	}, col, vpn[0].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 1.0f, 0.0f, 1.0f	}, col, vpn[1].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 1.0f, 0.0f, 1.0f	}, col, vpn[2].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 1.0f, 0.0f, 1.0f	}, col, vpn[3].NormVCol);

		Misc_InterleaveVec4ToF16((vec4){	0.0f, -1.0f, 0.0f, 1.0f	}, col, vpn[4].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, -1.0f, 0.0f, 1.0f	}, col, vpn[5].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, -1.0f, 0.0f, 1.0f	}, col, vpn[6].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, -1.0f, 0.0f, 1.0f	}, col, vpn[7].NormVCol);

		Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, 1.0f, 1.0f	}, col, vpn[8].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, 1.0f, 1.0f	}, col, vpn[9].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, 1.0f, 1.0f	}, col, vpn[10].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, 1.0f, 1.0f	}, col, vpn[11].NormVCol);

		Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, -1.0f, 1.0f	}, col, vpn[12].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, -1.0f, 1.0f	}, col, vpn[13].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, -1.0f, 1.0f	}, col, vpn[14].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	0.0f, 0.0f, -1.0f, 1.0f	}, col, vpn[15].NormVCol);

		Misc_InterleaveVec4ToF16((vec4){	-1.0f, 0.0f, 0.0f, 1.0f	}, col, vpn[16].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	-1.0f, 0.0f, 0.0f, 1.0f	}, col, vpn[17].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	-1.0f, 0.0f, 0.0f, 1.0f	}, col, vpn[18].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	-1.0f, 0.0f, 0.0f, 1.0f	}, col, vpn[19].NormVCol);

		Misc_InterleaveVec4ToF16((vec4){	1.0f, 1.0f, 0.0f, 1.0f	}, col, vpn[20].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	1.0f, 1.0f, 0.0f, 1.0f	}, col, vpn[21].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	1.0f, 1.0f, 0.0f, 1.0f	}, col, vpn[22].NormVCol);
		Misc_InterleaveVec4ToF16((vec4){	1.0f, 1.0f, 0.0f, 1.0f	}, col, vpn[23].NormVCol);

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

	pObj->mVertCount	=24 * numCubes;
	pObj->mIndexCount	=36 * numCubes;

	//make index buffer
	D3D11_BUFFER_DESC	bufDesc;
	sMakeIBDesc(&bufDesc, 36 * 4 * numCubes);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, inds, bufDesc.ByteWidth);

	sMakeStructuredBuffer(pGD, sizeof(VPosNormCol), 24 * numCubes, vpn,
							&pObj->mpVB, &pObj->mpVBSRV);

	free(vpn);
	free(inds);

	pObj->mVertSize	=sizeof(VPosNormCol);

	return	pObj;
}

PrimObject	*PF_CreateManyRays(const vec3 *pStarts, const vec3 *pEnds, const vec4 *pColours,
								int numRays, float rayWidth, GraphicsDevice *pGD)
{
	VPosNormCol	*vpnc	=malloc(sizeof(VPosNormCol) * 24 * numRays);
	uint32_t	*inds	=malloc(sizeof(uint32_t) * 36 * numRays);

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
		glm_vec3_copy(upperTopLeft,		vpnc[ofs + 0].PositionU);
		glm_vec3_copy(upperTopRight,	vpnc[ofs + 1].PositionU);
		glm_vec3_copy(upperBotRight,	vpnc[ofs + 2].PositionU);
		glm_vec3_copy(upperBotLeft,		vpnc[ofs + 3].PositionU);

		//bottom (note reversal) (Z-)
		glm_vec3_copy(lowerTopLeft,		vpnc[ofs + 7].PositionU);
		glm_vec3_copy(lowerTopRight,	vpnc[ofs + 6].PositionU);
		glm_vec3_copy(lowerBotRight,	vpnc[ofs + 5].PositionU);
		glm_vec3_copy(lowerBotLeft,		vpnc[ofs + 4].PositionU);

		//top Y side (Y+)
		glm_vec3_copy(upperTopLeft,		vpnc[ofs + 11].PositionU);
		glm_vec3_copy(upperTopRight,	vpnc[ofs + 10].PositionU);
		glm_vec3_copy(lowerTopRight,	vpnc[ofs + 9].PositionU);
		glm_vec3_copy(lowerTopLeft,		vpnc[ofs + 8].PositionU);

		//bottom Y side (Y-)
		glm_vec3_copy(upperBotLeft,		vpnc[ofs + 12].PositionU);
		glm_vec3_copy(upperBotRight,	vpnc[ofs + 13].PositionU);
		glm_vec3_copy(lowerBotRight,	vpnc[ofs + 14].PositionU);
		glm_vec3_copy(lowerBotLeft,		vpnc[ofs + 15].PositionU);

		//-x side
		glm_vec3_copy(upperTopLeft,		vpnc[ofs + 16].PositionU);
		glm_vec3_copy(upperBotLeft,		vpnc[ofs + 17].PositionU);
		glm_vec3_copy(lowerBotLeft,		vpnc[ofs + 18].PositionU);
		glm_vec3_copy(lowerTopLeft,		vpnc[ofs + 19].PositionU);

		//+x side
		glm_vec3_copy(upperTopRight,	vpnc[ofs + 23].PositionU);
		glm_vec3_copy(upperBotRight,	vpnc[ofs + 22].PositionU);
		glm_vec3_copy(lowerBotRight,	vpnc[ofs + 21].PositionU);
		glm_vec3_copy(lowerTopRight,	vpnc[ofs + 20].PositionU);
		
		//normals
		//top (Z+)
		Misc_InterleaveVec34ToF16(rayZ, pColours[i], vpnc[ofs + 0].NormVCol);
		Misc_InterleaveVec34ToF16(rayZ, pColours[i], vpnc[ofs + 1].NormVCol);
		Misc_InterleaveVec34ToF16(rayZ, pColours[i], vpnc[ofs + 2].NormVCol);
		Misc_InterleaveVec34ToF16(rayZ, pColours[i], vpnc[ofs + 3].NormVCol);

		//bottom (Z-)
		Misc_InterleaveVec34ToF16(rayNegZ, pColours[i], vpnc[ofs + 4].NormVCol);
		Misc_InterleaveVec34ToF16(rayNegZ, pColours[i], vpnc[ofs + 5].NormVCol);
		Misc_InterleaveVec34ToF16(rayNegZ, pColours[i], vpnc[ofs + 6].NormVCol);
		Misc_InterleaveVec34ToF16(rayNegZ, pColours[i], vpnc[ofs + 7].NormVCol);

		//top Y side (Y+)
		Misc_InterleaveVec34ToF16(rayY, pColours[i], vpnc[ofs + 8].NormVCol);
		Misc_InterleaveVec34ToF16(rayY, pColours[i], vpnc[ofs + 9].NormVCol);
		Misc_InterleaveVec34ToF16(rayY, pColours[i], vpnc[ofs + 10].NormVCol);
		Misc_InterleaveVec34ToF16(rayY, pColours[i], vpnc[ofs + 11].NormVCol);

		//bottom Y side (Y-)
		Misc_InterleaveVec34ToF16(rayNegY, pColours[i], vpnc[ofs + 12].NormVCol);
		Misc_InterleaveVec34ToF16(rayNegY, pColours[i], vpnc[ofs + 13].NormVCol);
		Misc_InterleaveVec34ToF16(rayNegY, pColours[i], vpnc[ofs + 14].NormVCol);
		Misc_InterleaveVec34ToF16(rayNegY, pColours[i], vpnc[ofs + 15].NormVCol);

		//-x side
		Misc_InterleaveVec34ToF16(rayNegX, pColours[i], vpnc[ofs + 16].NormVCol);
		Misc_InterleaveVec34ToF16(rayNegX, pColours[i], vpnc[ofs + 17].NormVCol);
		Misc_InterleaveVec34ToF16(rayNegX, pColours[i], vpnc[ofs + 18].NormVCol);
		Misc_InterleaveVec34ToF16(rayNegX, pColours[i], vpnc[ofs + 19].NormVCol);

		//+x side
		Misc_InterleaveVec34ToF16(rayX, pColours[i], vpnc[ofs + 20].NormVCol);
		Misc_InterleaveVec34ToF16(rayX, pColours[i], vpnc[ofs + 21].NormVCol);
		Misc_InterleaveVec34ToF16(rayX, pColours[i], vpnc[ofs + 22].NormVCol);
		Misc_InterleaveVec34ToF16(rayX, pColours[i], vpnc[ofs + 23].NormVCol);

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

	pObj->mVertCount	=24 * numRays;
	pObj->mIndexCount	=36 * numRays;

	//make index buffer
	D3D11_BUFFER_DESC	bufDesc;
	sMakeIBDesc(&bufDesc, 36 * 4 * numRays);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, inds, bufDesc.ByteWidth);

	sMakeStructuredBuffer(pGD, sizeof(VPosNormCol), 24 * numRays,
							vpnc, &pObj->mpVB, &pObj->mpVBSRV);

	free(vpnc);
	free(inds);

	pObj->mVertSize	=sizeof(VPosNormCol);

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
	VPosNormCol	vpn[24];

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
	sMulAdd2DestS(UnitX, 0.5f, UnitY, bottomTex);
	glm_vec2_scale(UnitY, 0.5f, leftTex);
	sMulAdd2DestS(UnitY, 0.5f, UnitX, rightTex);

	vec4	colWhite	={	1,1,1,1	};

	//need to have a lot of duplicates since each
	//vertex will contain a copy of the face normal
	//as we want this to be flat shaded
	Misc_InterleaveVec34ToF16(topUpperLeft, colWhite, vpn[0].NormVCol);
	Misc_InterleaveVec34ToF16(topUpperLeft, colWhite, vpn[1].NormVCol);
	Misc_InterleaveVec34ToF16(topUpperLeft, colWhite, vpn[2].NormVCol);

	Misc_InterleaveVec34ToF16(topUpperRight, colWhite, vpn[3].NormVCol);
	Misc_InterleaveVec34ToF16(topUpperRight, colWhite, vpn[4].NormVCol);
	Misc_InterleaveVec34ToF16(topUpperRight, colWhite, vpn[5].NormVCol);

	Misc_InterleaveVec34ToF16(topLowerLeft, colWhite, vpn[6].NormVCol);
	Misc_InterleaveVec34ToF16(topLowerLeft, colWhite, vpn[7].NormVCol);
	Misc_InterleaveVec34ToF16(topLowerLeft, colWhite, vpn[8].NormVCol);

	Misc_InterleaveVec34ToF16(topLowerRight, colWhite, vpn[9].NormVCol);
	Misc_InterleaveVec34ToF16(topLowerRight, colWhite, vpn[10].NormVCol);
	Misc_InterleaveVec34ToF16(topLowerRight, colWhite, vpn[11].NormVCol);

	Misc_InterleaveVec34ToF16(botUpperLeft, colWhite, vpn[12].NormVCol);
	Misc_InterleaveVec34ToF16(botUpperLeft, colWhite, vpn[13].NormVCol);
	Misc_InterleaveVec34ToF16(botUpperLeft, colWhite, vpn[14].NormVCol);

	Misc_InterleaveVec34ToF16(botUpperRight, colWhite, vpn[15].NormVCol);
	Misc_InterleaveVec34ToF16(botUpperRight, colWhite, vpn[16].NormVCol);
	Misc_InterleaveVec34ToF16(botUpperRight, colWhite, vpn[17].NormVCol);

	Misc_InterleaveVec34ToF16(botLowerLeft, colWhite, vpn[18].NormVCol);
	Misc_InterleaveVec34ToF16(botLowerLeft, colWhite, vpn[19].NormVCol);
	Misc_InterleaveVec34ToF16(botLowerLeft, colWhite, vpn[20].NormVCol);

	Misc_InterleaveVec34ToF16(botLowerRight, colWhite, vpn[21].NormVCol);
	Misc_InterleaveVec34ToF16(botLowerRight, colWhite, vpn[22].NormVCol);
	Misc_InterleaveVec34ToF16(botLowerRight, colWhite, vpn[23].NormVCol);

	//top upper left face
	glm_vec3_copy(topPoint, vpn[0].PositionU);
	glm_vec3_copy(left, vpn[1].PositionU);
	glm_vec3_copy(top, vpn[2].PositionU);

	//top upper right face
	glm_vec3_copy(topPoint, vpn[3].PositionU);
	glm_vec3_copy(top, vpn[4].PositionU);
	glm_vec3_copy(right, vpn[5].PositionU);

	//top lower left face
	glm_vec3_copy(topPoint, vpn[6].PositionU);
	glm_vec3_copy(bottom, vpn[7].PositionU);
	glm_vec3_copy(left, vpn[8].PositionU);

	//top lower right face
	glm_vec3_copy(topPoint, vpn[9].PositionU);
	glm_vec3_copy(right, vpn[10].PositionU);
	glm_vec3_copy(bottom, vpn[11].PositionU);

	//bottom upper left face
	glm_vec3_copy(bottomPoint, vpn[12].PositionU);
	glm_vec3_copy(top, vpn[13].PositionU);
	glm_vec3_copy(left, vpn[14].PositionU);

	//bottom upper right face
	glm_vec3_copy(bottomPoint, vpn[15].PositionU);
	glm_vec3_copy(right, vpn[16].PositionU);
	glm_vec3_copy(top, vpn[17].PositionU);

	//bottom lower left face
	glm_vec3_copy(bottomPoint, vpn[18].PositionU);
	glm_vec3_copy(left, vpn[19].PositionU);
	glm_vec3_copy(bottom, vpn[20].PositionU);

	//bottom lower right face
	glm_vec3_copy(bottomPoint, vpn[21].PositionU);
	glm_vec3_copy(bottom, vpn[22].PositionU);
	glm_vec3_copy(right, vpn[23].PositionU);

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

	//make index buffer
	D3D11_BUFFER_DESC	bufDesc;
	sMakeIBDesc(&bufDesc, 24 * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, indexes, bufDesc.ByteWidth);

	sMakeStructuredBuffer(pGD, sizeof(VPosNormCol), 24, vpn,
							&pObj->mpVB, &pObj->mpVBSRV);

	pObj->mVertSize	=sizeof(VPosNormCol);

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

	//verts
	VPosNormCol	vpnt[18];

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
	sMulAdd2DestS(UnitX, 0.5f, UnitY, bottomTex);
	glm_vec2_scale(UnitY, 0.5f, leftTex);
	sMulAdd2DestS(UnitY, 0.5f, UnitX, rightTex);

	vec4	colWhite	={	1,1,1,1	};

	//need to have a lot of duplicates since each
	//vertex will contain a copy of the face normal
	//as we want this to be flat shaded
	Misc_InterleaveVec34ToF16(topUpperLeft, colWhite, vpnt[0].NormVCol);
	Misc_InterleaveVec34ToF16(topUpperLeft, colWhite, vpnt[1].NormVCol);
	Misc_InterleaveVec34ToF16(topUpperLeft, colWhite, vpnt[2].NormVCol);

	Misc_InterleaveVec34ToF16(topUpperRight, colWhite, vpnt[3].NormVCol);
	Misc_InterleaveVec34ToF16(topUpperRight, colWhite, vpnt[4].NormVCol);
	Misc_InterleaveVec34ToF16(topUpperRight, colWhite, vpnt[5].NormVCol);

	Misc_InterleaveVec34ToF16(topLowerLeft, colWhite, vpnt[6].NormVCol);
	Misc_InterleaveVec34ToF16(topLowerLeft, colWhite, vpnt[7].NormVCol);
	Misc_InterleaveVec34ToF16(topLowerLeft, colWhite, vpnt[8].NormVCol);

	Misc_InterleaveVec34ToF16(topLowerRight, colWhite, vpnt[9].NormVCol);
	Misc_InterleaveVec34ToF16(topLowerRight, colWhite, vpnt[10].NormVCol);
	Misc_InterleaveVec34ToF16(topLowerRight, colWhite, vpnt[11].NormVCol);

	Misc_InterleaveVec4ToF16((vec4){0, 1, 0, 1}, colWhite, vpnt[12].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){0, 1, 0, 1}, colWhite, vpnt[13].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){0, 1, 0, 1}, colWhite, vpnt[14].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){0, 1, 0, 1}, colWhite, vpnt[15].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){0, 1, 0, 1}, colWhite, vpnt[16].NormVCol);
	Misc_InterleaveVec4ToF16((vec4){0, 1, 0, 1}, colWhite, vpnt[17].NormVCol);
	
	//top upper left face
	glm_vec3_copy(topPoint, vpnt[0].PositionU);
	glm_vec3_copy(left, vpnt[2].PositionU);
	glm_vec3_copy(top, vpnt[1].PositionU);

	//top upper right face
	glm_vec3_copy(topPoint, vpnt[3].PositionU);
	glm_vec3_copy(top, vpnt[5].PositionU);
	glm_vec3_copy(right, vpnt[4].PositionU);

	//top lower left face
	glm_vec3_copy(topPoint, vpnt[6].PositionU);
	glm_vec3_copy(bottom, vpnt[8].PositionU);
	glm_vec3_copy(left, vpnt[7].PositionU);

	//top lower right face
	glm_vec3_copy(topPoint, vpnt[9].PositionU);
	glm_vec3_copy(right, vpnt[11].PositionU);
	glm_vec3_copy(bottom, vpnt[10].PositionU);

	//bottom face (2 triangles)
	glm_vec3_copy(top, vpnt[12].PositionU);
	glm_vec3_copy(bottom, vpnt[14].PositionU);
	glm_vec3_copy(right, vpnt[13].PositionU);
	glm_vec3_copy(top, vpnt[15].PositionU);
	glm_vec3_copy(left, vpnt[17].PositionU);
	glm_vec3_copy(bottom, vpnt[16].PositionU);

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

	//make index buffer
	D3D11_BUFFER_DESC	bufDesc;
	sMakeIBDesc(&bufDesc, 18 * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, indexes, bufDesc.ByteWidth);

	sMakeStructuredBuffer(pGD, sizeof(VPosNormCol), 18, vpnt,
							&pObj->mpVB, &pObj->mpVBSRV);

	pObj->mVertSize	=sizeof(VPosNormCol);

	return	pObj;
}

PrimObject	*PF_CreateSphere(vec3 center, float radius, bool bFlipped, GraphicsDevice *pGD)
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

				if(bFlipped)
				{
					pInds[iIdx++]	=curIdx;
					pInds[iIdx++]	=curIdx + 2;
					pInds[iIdx++]	=curIdx + 1;
					pInds[iIdx++]	=curIdx;
					pInds[iIdx++]	=curIdx + 3;
					pInds[iIdx++]	=curIdx + 2;
				}
				else
				{
					pInds[iIdx++]	=curIdx;
					pInds[iIdx++]	=curIdx + 1;
					pInds[iIdx++]	=curIdx + 2;
					pInds[iIdx++]	=curIdx;
					pInds[iIdx++]	=curIdx + 2;
					pInds[iIdx++]	=curIdx + 3;
				}

				curIdx	+=4;
			}
			else
			{
				if(bFlipped)
				{
					pInds[iIdx++]	=curIdx;
					pInds[iIdx++]	=curIdx + 2;
					pInds[iIdx++]	=curIdx + 1;
				}
				else
				{
					pInds[iIdx++]	=curIdx;
					pInds[iIdx++]	=curIdx + 1;
					pInds[iIdx++]	=curIdx + 2;
				}
				curIdx	+=3;
			}
		}
	}

	vec4	colWhite	={	1,1,1,1	};

	//alloc verts
	VPosNormCol	*vpn	=malloc(sizeof(VPosNormCol) * vertCount * 2);

	//copy in hemisphere
	for(int i=0;i < vertCount;i++)
	{
		vec3	norm;

		glm_vec3_normalize_to(pPoints[i], norm);

		glm_vec3_copy(center, vpn[i].PositionU);
		glm_vec3_muladds(norm, radius, vpn[i].PositionU);

		Misc_InterleaveVec34ToF16(norm, colWhite, vpn[i].NormVCol);
	}

	//dupe for other half
	int	ofs	=vertCount;
	for(int i=ofs;i < vertCount + ofs;i++)
	{
		vec3	norm;

		glm_vec3_normalize_to(pPoints[i - ofs], norm);

		glm_vec3_copy(center, vpn[i].PositionU);
		glm_vec3_muladds(norm, -radius, vpn[i].PositionU);

		glm_vec3_flipsign(norm);
		Misc_InterleaveVec34ToF16(norm, colWhite, vpn[i].NormVCol);
	}

	//index other half, flip winding
	for(int i=indCount;i < (indCount * 2);i+=3)
	{
		if(!bFlipped)
		{
			pInds[i]		=vertCount + pInds[i - indCount];
			pInds[i + 1]	=vertCount + pInds[(i + 1) - indCount];
			pInds[i + 2]	=vertCount + pInds[(i + 2) - indCount];
		}
		else
		{
			pInds[i]		=vertCount + pInds[i - indCount];
			pInds[i + 1]	=vertCount + pInds[(i + 2) - indCount];
			pInds[i + 2]	=vertCount + pInds[(i + 1) - indCount];
		}
	}

	vertCount	*=2;
	indCount	*=2;

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=vertCount;
	pObj->mIndexCount	=indCount;

	//make index buffer
	D3D11_BUFFER_DESC	bufDesc;
	sMakeIBDesc(&bufDesc, indCount * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, pInds, bufDesc.ByteWidth);

	sMakeStructuredBuffer(pGD, sizeof(VPosNormCol), vertCount, vpn,
							&pObj->mpVB, &pObj->mpVBSRV);

	//free temp buffers
	free(pPoints);
	free(pInds);
	free(vpn);

	pObj->mVertSize	=sizeof(VPosNormCol);

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

	vec4	colWhite	={	1,1,1,1	};

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
	VPosNormCol	*vpn	=malloc(sizeof(VPosNormCol) * vertCount * 2);

	//copy in hemisphere
	for(int i=0;i < vertCount;i++)
	{
		vec3	norm;

		glm_vec3_normalize_to(pPoints[i], norm);

		glm_vec3_zero(vpn[i].PositionU);
		glm_vec3_muladds(norm, radius, vpn[i].PositionU);

		Misc_InterleaveVec34ToF16(norm, colWhite, vpn[i].NormVCol);
	}

	//dupe for other half
	int	ofs	=vertCount;
	for(int i=ofs;i < vertCount + ofs;i++)
	{
		vec3	norm;

		glm_vec3_normalize_to(pPoints[i - ofs], norm);

		glm_vec3_zero(vpn[i].PositionU);
		glm_vec3_muladds(norm, -radius, vpn[i].PositionU);
		glm_vec3_muladds(UnitY, len, vpn[i].PositionU);

		glm_vec3_flipsign(norm);
		Misc_InterleaveVec34ToF16(norm, colWhite, vpn[i].NormVCol);
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

	//make index buffer
	D3D11_BUFFER_DESC	bufDesc;
	sMakeIBDesc(&bufDesc, indCount * 2 * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, pInds, bufDesc.ByteWidth);

	sMakeStructuredBuffer(pGD, sizeof(VPosNormCol), 2 * vertCount, vpn,
							&pObj->mpVB, &pObj->mpVBSRV);

	//free temp buffers
	free(pPoints);
	free(pInds);
	free(vpn);

	pObj->mVertSize	=sizeof(VPosNormCol);

	return	pObj;
}

//a single triangle
PrimObject	*PF_CreateTri(const vec3 tri[3], GraphicsDevice *pGD)
{
	VPosNormCol	verts[3];
	uint16_t	inds[3];
	vec4		colWhite	={	1,1,1,1	};

	vec4	plane;
	PM_FromTri(tri[0], tri[1], tri[2], plane);

	for(int i=0;i < 3;i++)
	{
		glm_vec3_copy(tri[i], verts[i].PositionU);

//		glm_vec4_copy(plane, verts[i].NormalV);
		Misc_InterleaveVec4ToF16(plane, colWhite, verts[i].NormVCol);

		inds[i]	=i;
	}

	//return object
	PrimObject	*pObj	=malloc(sizeof(PrimObject));

	pObj->mVertCount	=3;
	pObj->mIndexCount	=3;

	//make index buffer
	D3D11_BUFFER_DESC	bufDesc;
	sMakeIBDesc(&bufDesc, 3 * 2);
	pObj->mpIB	=GD_CreateBufferWithData(pGD, &bufDesc, inds, bufDesc.ByteWidth);

	sMakeStructuredBuffer(pGD, sizeof(VPosNormCol), 3, verts,
							&pObj->mpVB, &pObj->mpVBSRV);

	pObj->mVertSize	=sizeof(VPosNormCol);

	return	pObj;
}


//statics
static void	sMakeStructuredBuffer(GraphicsDevice *pGD,
	int structSize, int numItems, void *pVData,
	ID3D11Buffer **ppBuffer, ID3D11ShaderResourceView **ppSRV)
{
	//buffer is not optional here
	assert(ppBuffer != NULL);

	D3D11_BUFFER_DESC	bufDesc;
	memset(&bufDesc, 0, sizeof(D3D11_BUFFER_DESC));

	//particle buffer
	bufDesc.BindFlags			=D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bufDesc.ByteWidth			=structSize * numItems;
	bufDesc.CPUAccessFlags		=0;
	bufDesc.MiscFlags			=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufDesc.StructureByteStride	=structSize;
	bufDesc.Usage				=D3D11_USAGE_DEFAULT;

	*ppBuffer	=GD_CreateBufferWithData(pGD, &bufDesc,
					pVData, structSize * numItems);

	if(ppSRV != NULL)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC	srvDesc;
		memset(&srvDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

		srvDesc.Format				=DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension		=D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.ElementWidth	=numItems;

		*ppSRV	=GD_CreateSRV(pGD, (ID3D11Resource *)*ppBuffer, &srvDesc);
	}
}

static void	sMulAdd2DestS(const vec2 a, const float b, const vec2 c, vec2 dest)
{
	dest[0]	=c[0] + (a[0] * b);
	dest[1]	=c[1] + (a[1] * b);
}

static void	sMakeIBDesc(D3D11_BUFFER_DESC *pDesc, uint32_t byteSize)
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
static void	sThreeXYZ(const vec3 x, const vec3 y, const vec3 z, vec3 dest)
{
	dest[0]	=x[0];
	dest[1]	=y[1];
	dest[2]	=z[2];
}
