#include	<stdio.h>
#include	"Material.h"
#include	"d3d11.h"
#include	<cglm/call.h>
#include	"CBKeeper.h"
#include	"StuffKeeper.h"
#include	"../UtilityLib/GraphicsDevice.h"
#include	"../UtilityLib/StringStuff.h"


typedef struct	Material_t
{
	mat4	mWorld;

	ID3D11InputLayout			*mpLayout;
	ID3D11VertexShader			*mpVShader;
	ID3D11PixelShader			*mpPShader;
	ID3D11ShaderResourceView	*mpSRV0;
	ID3D11ShaderResourceView	*mpSRV1;

	vec3	mTrilight0;
	vec3	mTrilight1;
	vec3	mTrilight2;
	vec3	mLightDirection;
	vec4	mSolidColour;
	vec3	mSpecular;
	vec3	mDanglyForce;
	int		mMaterialID;
	float	mSpecPower;

}	Material;


Material	*MAT_Create(GraphicsDevice *pGD)
{
#ifdef	__AVX__
	Material	*pRet	=aligned_alloc(32, sizeof(Material));
#else
	Material	*pRet	=aligned_alloc(16, sizeof(Material));
#endif

	memset(pRet, 0, sizeof(Material));

	//some basic defaults
	glm_vec3_fill(pRet->mTrilight0, 1.0f);
	glm_vec3_fill(pRet->mTrilight1, 0.2f);
	glm_vec3_fill(pRet->mTrilight2, 0.1f);

	pRet->mLightDirection[0]	=1.0f;
	pRet->mLightDirection[1]	=-5.0f;
	pRet->mLightDirection[2]	=3.0f;
	glm_vec3_normalize(pRet->mLightDirection);

	glm_vec3_copy(GLM_VEC3_ONE, pRet->mSpecular);
	glm_vec4_copy(GLM_VEC4_ONE, pRet->mSolidColour);

	pRet->mSpecPower	=3.0f;

	return	pRet;
}


void	MAT_Apply(const Material *pMat, CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//gpu stuff
	GD_IASetInputLayout(pGD, pMat->mpLayout);
	GD_VSSetShader(pGD, pMat->mpVShader);
	GD_PSSetShader(pGD, pMat->mpPShader);
	GD_PSSetSRV(pGD, pMat->mpSRV0, 0);
	GD_PSSetSRV(pGD, pMat->mpSRV0, 1);

	//materialish shtuff
	CBK_SetTrilights3(pCBK, pMat->mTrilight0, pMat->mTrilight1,
		pMat->mTrilight2, pMat->mLightDirection);

	CBK_SetSolidColour(pCBK, pMat->mSolidColour);

	CBK_SetSpecular(pCBK, pMat->mSpecular, pMat->mSpecPower);

	CBK_SetDanglyForce(pCBK, pMat->mDanglyForce);

	CBK_SetMaterialID(pCBK, pMat->mMaterialID);

	CBK_SetWorldMat(pCBK, pMat->mWorld);

	CBK_UpdateObject(pCBK, pGD);
}


bool	MAT_SetLayout(Material *pMat, const char *szLayName, const StuffKeeper *pSK)
{
	pMat->mpLayout	=StuffKeeper_GetInputLayout(pSK, szLayName);

	return	(pMat->mpLayout != NULL);
}

bool	MAT_SetVShader(Material *pMat, const char *szVSName, const StuffKeeper *pSK)
{
	pMat->mpVShader	=StuffKeeper_GetVertexShader(pSK, szVSName);

	return	(pMat->mpVShader != NULL);
}

bool	MAT_SetPShader(Material *pMat, const char *szPSName, const StuffKeeper *pSK)
{
	pMat->mpPShader	=StuffKeeper_GetPixelShader(pSK, szPSName);

	return	(pMat->mpPShader != NULL);
}

bool	MAT_SetSRV0(Material *pMat, const char *szTex, const StuffKeeper *pSK)
{
	pMat->mpSRV0	=StuffKeeper_GetSRV(pSK, szTex);

	return	(pMat->mpSRV0 != NULL);
}

bool	MAT_SetSRV1(Material *pMat, const char *szTex, const StuffKeeper *pSK)
{
	pMat->mpSRV1	=StuffKeeper_GetSRV(pSK, szTex);

	return	(pMat->mpSRV1 != NULL);
}


void	MAT_SetLights(Material *pMat, const vec3 tri0, const vec3 tri1,
					const vec3 tri2, const vec3 lightDir)
{
	glm_vec3_copy(tri0, pMat->mTrilight0);
	glm_vec3_copy(tri1, pMat->mTrilight1);
	glm_vec3_copy(tri2, pMat->mTrilight2);
	glm_vec3_copy(lightDir, pMat->mLightDirection);
}

void	MAT_SetLightDirection(Material *pMat, const vec3 lightDir)
{
	glm_vec3_copy(lightDir, pMat->mLightDirection);
}

void	MAT_SetSolidColour(Material *pMat, const vec4 col)
{
	glm_vec4_copy(col, pMat->mSolidColour);
}

void	MAT_SetSpecular(Material *pMat, const vec3 spec, float specPower)
{
	glm_vec3_copy(spec, pMat->mSpecular);

	pMat->mSpecPower	=specPower;
}

void	MAT_SetDanglyForce(Material *pMat, const vec3 force)
{
	glm_vec3_copy(force, pMat->mDanglyForce);
}

void	MAT_SetMaterialID(Material *pMat, int id)
{
	pMat->mMaterialID	=id;
}

void	MAT_SetWorld(Material *pMat, const mat4 world)
{
	glm_mat4_copy(world, pMat->mWorld);
}

Material	*MAT_Read(FILE *f, const StuffKeeper *pSK)
{
	Material	*pRet	=malloc(sizeof(Material));

	//vertex shader
	UT_string	*szStuff	=SZ_ReadString(f);
	pRet->mpVShader	=StuffKeeper_GetVertexShader(pSK, utstring_body(szStuff));
	utstring_clear(szStuff);

	//pixel shader
	szStuff			=SZ_ReadString(f);
	pRet->mpPShader	=StuffKeeper_GetPixelShader(pSK, utstring_body(szStuff));
	utstring_clear(szStuff);

	//haven't done these in C yet
	//depth stencil state
	szStuff	=SZ_ReadString(f);
	printf("Depth stencil state in matlib: %s", utstring_body(szStuff));
	utstring_clear(szStuff);

	szStuff	=SZ_ReadString(f);
	printf("Blend state in matlib: %s", utstring_body(szStuff));
	utstring_clear(szStuff);

	szStuff	=SZ_ReadString(f);
	printf("Samp0 state in matlib: %s", utstring_body(szStuff));
	utstring_clear(szStuff);

	szStuff	=SZ_ReadString(f);
	printf("Samp1 state in matlib: %s", utstring_body(szStuff));
	utstring_clear(szStuff);

	fread(&pRet->mMaterialID, sizeof(int), 1, f);

	uint8_t	bsp;
	fread(&bsp, sizeof(uint8_t), 1, f);

//	bool	bBSP;
//	fread(&bBSP, sizeof(bool), 1, f);
	if(bsp)
	{
		printf("Uh oh, a bsp material.  I haven't done these yet!\n");
		assert(!bsp);
	}

	uint8_t	mesh;
	fread(&mesh, sizeof(uint8_t), 1, f);
	assert(mesh);

//	bool	bMesh;
//	fread(&bMesh, sizeof(int), 1, f);
//	assert(bMesh);

	fread(pRet->mSolidColour, sizeof(vec4), 1, f);

	vec4	tmp;
	fread(tmp, sizeof(vec4), 1, f);
	glm_vec4_copy3(tmp, pRet->mSpecular);

	fread(tmp, sizeof(vec4), 1, f);
	glm_vec4_copy3(tmp, pRet->mTrilight0);

	fread(tmp, sizeof(vec4), 1, f);
	glm_vec4_copy3(tmp, pRet->mTrilight1);

	fread(tmp, sizeof(vec4), 1, f);
	glm_vec4_copy3(tmp, pRet->mTrilight2);

	fread(pRet->mLightDirection, sizeof(vec3), 1, f);
	fread(pRet->mDanglyForce, sizeof(vec3), 1, f);
	fread(&pRet->mSpecPower, sizeof(float), 1, f);

	//these can sometimes have \\ style paths from windows
	szStuff				=SZ_ReadString(f);
	UT_string	*szConv	=SZ_ConvertPathSlashesUT(szStuff);
	pRet->mpSRV0		=StuffKeeper_GetSRV(pSK, utstring_body(szConv));
	utstring_clear(szStuff);
	utstring_clear(szConv);

	szStuff			=SZ_ReadString(f);
	szConv			=SZ_ConvertPathSlashesUT(szStuff);
	pRet->mpSRV1	=StuffKeeper_GetSRV(pSK, utstring_body(szConv));

	utstring_done(szStuff);
	utstring_done(szConv);

	return	pRet;
}