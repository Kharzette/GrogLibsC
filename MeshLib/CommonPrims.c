#include	"CommonPrims.h"
#include	"../UtilityLib/MiscStuff.h"
#include	"../MaterialLib/StuffKeeper.h"


typedef struct	AxisXYZ_t
{
	//top to align
	mat4	mWorld;

	PrimObject	*mpX;
	PrimObject	*mpY;
	PrimObject	*mpZ;
	PrimObject	*mpPointyEnd;

	float	mLength, mWidth;

	ID3D11InputLayout	*mpLayout;
	ID3D11VertexShader	*mpVS;
	ID3D11PixelShader	*mpPS;

}	AxisXYZ;

typedef struct	LightRay_t
{
	//top to align
	mat4	mWorld;
	mat4	mPointyOffset;

	PrimObject	*mpAxis;
	PrimObject	*mpPointyEnd;

	vec3	mPosition;

	ID3D11InputLayout	*mpLayout;
	ID3D11VertexShader	*mpVS;
	ID3D11PixelShader	*mpPS;

}	LightRay;

static const	vec3	UnitX	={	1.0f, 0.0f, 0.0f	};
static const	vec3	UnitZ	={	0.0f, 0.0f, 1.0f	};


AxisXYZ	*CP_CreateAxis(float length, float width, GraphicsDevice *pGD, const StuffKeeper *pSK)
{
#ifdef	__AVX__
	AxisXYZ	*pRet	=aligned_alloc(32, sizeof(AxisXYZ));
#else
	AxisXYZ	*pRet	=aligned_alloc(16, sizeof(AxisXYZ));
#endif

	memset(pRet, 0, sizeof(AxisXYZ));

	pRet->mLength	=length;
	pRet->mWidth	=width;

	//create axis bounds
	vec3	xMin	={	0.0f,	-width,	-width	};
	vec3	yMin	={	-width,	0.0f,	-width	};
	vec3	zMin	={	-width,	-width,	0.0f	};
	vec3	xMax	={	length,	width,	width	};
	vec3	yMax	={	width,	length,	width	};
	vec3	zMax	={	width,	width,	length	};

	pRet->mpX			=PF_CreateCubeFromBounds(xMin, xMax, pGD);
	pRet->mpY			=PF_CreateCubeFromBounds(yMin, yMax, pGD);
	pRet->mpZ			=PF_CreateCubeFromBounds(zMin, zMax, pGD);
	pRet->mpPointyEnd	=PF_CreateHalfPrism(width * 2.0f, width * 4.0f, pGD);

	glm_mat4_identity(pRet->mWorld);

	pRet->mpLayout	=StuffKeeper_GetInputLayout(pSK, "VPosNorm");
	pRet->mpVS		=StuffKeeper_GetVertexShader(pSK, "WNormWPosVS");
	pRet->mpPS		=StuffKeeper_GetPixelShader(pSK, "TriSolidSpecPS");

	return	pRet;
}

LightRay	*CP_CreateLightRay(float length, float width, GraphicsDevice *pGD, const StuffKeeper *pSK)
{
#ifdef	__AVX__
	LightRay	*pRet	=aligned_alloc(32, sizeof(LightRay));
#else
	LightRay	*pRet	=aligned_alloc(16, sizeof(LightRay));
#endif

	memset(pRet, 0, sizeof(LightRay));

	//create ray bound
	vec3	min, max;

	Misc_MakeBaseZOrgBound(width, width, length, min, max);

	pRet->mpAxis		=PF_CreateCubeFromBounds(min, max, pGD);
	pRet->mpPointyEnd	=PF_CreateHalfPrism(width * 2.0f, width * 2.0f, pGD);

	glm_mat4_identity(pRet->mWorld);
	glm_mat4_identity(pRet->mPointyOffset);

	vec3	pointyPos;

	glm_vec3_zero(pointyPos);
	glm_vec3_zero(pRet->mPosition);

	pointyPos[2]	=length + (width * 2.0f);

	glm_translate(pRet->mPointyOffset, pointyPos);

	glm_rotate(pRet->mPointyOffset, -GLM_PI_2, UnitX);

	pRet->mpLayout	=StuffKeeper_GetInputLayout(pSK, "VPosNorm");
	pRet->mpVS		=StuffKeeper_GetVertexShader(pSK, "WNormWPosVS");
	pRet->mpPS		=StuffKeeper_GetPixelShader(pSK, "TriSolidSpecPS");

	return	pRet;
}

void	CP_DrawLightRay(LightRay *pRay, const vec3 lightDir, const vec4 rayColour,
						const vec3 location, CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//set gpu stuff
	GD_IASetVertexBuffers(pGD, pRay->mpAxis->mpVB, pRay->mpAxis->mVertSize, 0);
	GD_IASetIndexBuffers(pGD, pRay->mpAxis->mpIB, DXGI_FORMAT_R16_UINT, 0);
	GD_VSSetShader(pGD, pRay->mpVS);
	GD_PSSetShader(pGD, pRay->mpPS);
	GD_IASetInputLayout(pGD, pRay->mpLayout);

	//get a good side perp vec
	vec3	lightX, lightY, lightZ;
	Misc_BuildBasisVecsFromDirection(lightDir, lightX, lightY, lightZ);

	mat3	rayMat;

	//build a rotation from the basis vectors
	glm_vec3_copy(lightX, rayMat[0]);
	glm_vec3_copy(lightY, rayMat[1]);
	glm_vec3_copy(lightZ, rayMat[2]);

	glm_translate_make(pRay->mWorld, location);
	glm_mat4_ins3(rayMat, pRay->mWorld);

	//materialish stuff
	CBK_SetSolidColour(pCBK, rayColour);

	vec3	lightColour0	={	1,		1,		1		};
	vec3	lightColour1	={	0.8f,	0.8f,	0.8f	};
	vec3	lightColour2	={	0.6f,	0.6f,	0.6f	};
	vec3	specColour		={	1,		1,		1		};

	CBK_SetTrilights3(pCBK, lightColour0, lightColour1, lightColour2, lightDir);
	CBK_SetSpecular(pCBK, specColour, 1.0f);

	CBK_SetWorldMat(pCBK, pRay->mWorld);
	CBK_UpdateObject(pCBK, pGD);

	GD_DrawIndexed(pGD, pRay->mpAxis->mIndexCount, 0, 0);

	//set pointy end VB/IB
	GD_IASetVertexBuffers(pGD, pRay->mpPointyEnd->mpVB, pRay->mpPointyEnd->mVertSize, 0);
	GD_IASetIndexBuffers(pGD, pRay->mpPointyEnd->mpIB, DXGI_FORMAT_R16_UINT, 0);

	mat4	pointy;
	glm_mat4_mul(pRay->mWorld, pRay->mPointyOffset, pointy);

	CBK_SetWorldMat(pCBK, pointy);
	CBK_UpdateObject(pCBK, pGD);

	GD_DrawIndexed(pGD, pRay->mpPointyEnd->mIndexCount, 0, 0);
}

//assumes view, proj, input layout, and shaders set
void	CP_DrawAxis(AxisXYZ *pAxis, const vec3 lightDir,
					const vec4 xCol, const vec4 yCol, const vec4 zCol,
					CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//set X VB/IB
	GD_IASetVertexBuffers(pGD, pAxis->mpX->mpVB, pAxis->mpX->mVertSize, 0);
	GD_IASetIndexBuffers(pGD, pAxis->mpX->mpIB, DXGI_FORMAT_R16_UINT, 0);
	GD_VSSetShader(pGD, pAxis->mpVS);
	GD_PSSetShader(pGD, pAxis->mpPS);
	GD_IASetInputLayout(pGD, pAxis->mpLayout);

	//materialish stuff
	CBK_SetSolidColour(pCBK, xCol);

	vec3	lightColour0	={	1,		1,		1		};
	vec3	lightColour1	={	0.8f,	0.8f,	0.8f	};
	vec3	lightColour2	={	0.6f,	0.6f,	0.6f	};
	vec3	specColour		={	1,		1,		1		};
	vec3	localScale		={	1.0f,	1.0f,	1.0f	};

	CBK_SetLocalScale(pCBK, localScale);

	CBK_SetTrilights3(pCBK, lightColour0, lightColour1, lightColour2, lightDir);
	CBK_SetSpecular(pCBK, specColour, 1.0f);

	CBK_SetWorldMat(pCBK, pAxis->mWorld);
	CBK_UpdateObject(pCBK, pGD);

	GD_DrawIndexed(pGD, pAxis->mpX->mIndexCount, 0, 0);

	//set pointy end VB/IB for X
	GD_IASetVertexBuffers(pGD, pAxis->mpPointyEnd->mpVB, pAxis->mpPointyEnd->mVertSize, 0);
	GD_IASetIndexBuffers(pGD, pAxis->mpPointyEnd->mpIB, DXGI_FORMAT_R16_UINT, 0);

	mat4	pointyRot;
	glm_rotate_make(pointyRot, GLM_PI_2, UnitZ);

	vec3	pointyPos	={	pAxis->mLength + (pAxis->mWidth * 4.0f), 0.0f, 0.0f	};	

	mat4	pointyTrans;
	glm_translate_make(pointyTrans, pointyPos);

	mat4	pointyMat;
	glm_mat4_mul(pointyTrans, pointyRot, pointyMat);

	glm_mat4_mul(pAxis->mWorld, pointyMat, pointyMat);

	CBK_SetWorldMat(pCBK, pointyMat);
	CBK_UpdateObject(pCBK, pGD);

	GD_DrawIndexed(pGD, pAxis->mpPointyEnd->mIndexCount, 0, 0);

	//set Y VB/IB
	GD_IASetVertexBuffers(pGD, pAxis->mpY->mpVB, pAxis->mpY->mVertSize, 0);
	GD_IASetIndexBuffers(pGD, pAxis->mpY->mpIB, DXGI_FORMAT_R16_UINT, 0);

	//materialish stuff
	CBK_SetSolidColour(pCBK, yCol);
	CBK_SetWorldMat(pCBK, pAxis->mWorld);
	CBK_UpdateObject(pCBK, pGD);

	GD_DrawIndexed(pGD, pAxis->mpY->mIndexCount, 0, 0);

	//set pointy end VB/IB for Y
	GD_IASetVertexBuffers(pGD, pAxis->mpPointyEnd->mpVB, pAxis->mpPointyEnd->mVertSize, 0);
	GD_IASetIndexBuffers(pGD, pAxis->mpPointyEnd->mpIB, DXGI_FORMAT_R16_UINT, 0);

	pointyPos[0]	=0.0f;
	pointyPos[1]	=pAxis->mLength + (pAxis->mWidth * 4.0f);
	pointyPos[2]	=0.0f;

	glm_rotate_make(pointyRot, GLM_PI, UnitX);
	glm_translate_make(pointyTrans, pointyPos);
	glm_mat4_mul(pointyTrans, pointyRot, pointyMat);

	glm_mat4_mul(pAxis->mWorld, pointyMat, pointyMat);

	CBK_SetWorldMat(pCBK, pointyMat);
	CBK_UpdateObject(pCBK, pGD);

	GD_DrawIndexed(pGD, pAxis->mpPointyEnd->mIndexCount, 0, 0);

	//set Z VB/IB
	GD_IASetVertexBuffers(pGD, pAxis->mpZ->mpVB, pAxis->mpZ->mVertSize, 0);
	GD_IASetIndexBuffers(pGD, pAxis->mpZ->mpIB, DXGI_FORMAT_R16_UINT, 0);

	//materialish stuff
	CBK_SetSolidColour(pCBK, zCol);
	CBK_SetWorldMat(pCBK, pAxis->mWorld);
	CBK_UpdateObject(pCBK, pGD);

	GD_DrawIndexed(pGD, pAxis->mpZ->mIndexCount, 0, 0);

	//set pointy end VB/IB for Z
	GD_IASetVertexBuffers(pGD, pAxis->mpPointyEnd->mpVB, pAxis->mpPointyEnd->mVertSize, 0);
	GD_IASetIndexBuffers(pGD, pAxis->mpPointyEnd->mpIB, DXGI_FORMAT_R16_UINT, 0);

	pointyPos[0]	=0.0f;
	pointyPos[1]	=0.0f;
	pointyPos[2]	=pAxis->mLength + (pAxis->mWidth * 4.0f);

	glm_rotate_make(pointyRot, -GLM_PI_2, UnitX);
	glm_translate_make(pointyTrans, pointyPos);
	glm_mat4_mul(pointyTrans, pointyRot, pointyMat);

	glm_mat4_mul(pAxis->mWorld, pointyMat, pointyMat);

	CBK_SetWorldMat(pCBK, pointyMat);
	CBK_UpdateObject(pCBK, pGD);

	GD_DrawIndexed(pGD, pAxis->mpPointyEnd->mIndexCount, 0, 0);
}