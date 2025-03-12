#include	<stdint.h>
#include	<stdio.h>
#include	<assert.h>
#include	"KeyFrame.h"


void	KeyFrame_Lerp(const KeyFrame *pKey0, const KeyFrame *pKey1,
				float percentage, KeyFrame *pKeyResult)
{
	glm_quat_slerp(pKey0->mRotation, pKey1->mRotation, percentage, pKeyResult->mRotation);

	glm_vec3_lerp(pKey0->mPosition, pKey1->mPosition, percentage, pKeyResult->mPosition);
	glm_vec3_lerp(pKey0->mScale, pKey1->mScale, percentage, pKeyResult->mScale);
}


void	KeyFrame_Read(FILE *f, KeyFrame *pKey)
{
	fread(pKey->mRotation, sizeof(vec4), 1, f);
	fread(pKey->mPosition, sizeof(vec3), 1, f);
	fread(pKey->mScale, sizeof(vec3), 1, f);
}

void	KeyFrame_Write(const KeyFrame *pKey, FILE *f)
{
	fwrite(pKey->mRotation, sizeof(vec4), 1, f);
	fwrite(pKey->mPosition, sizeof(vec3), 1, f);
	fwrite(pKey->mScale, sizeof(vec3), 1, f);
}

void	KeyFrame_Identity(KeyFrame *pKey)
{
	glm_vec3_zero(pKey->mPosition);
	glm_vec3_one(pKey->mScale);
	glm_quat_identity(pKey->mRotation);
}

void	KeyFrame_ConvertToLeftHanded(KeyFrame *pKey)
{
	mat4	scale, rot, pos;

	glm_scale_make(scale, pKey->mScale);

	glm_quat_mat4(pKey->mRotation, rot);

	glm_translate_make(pos, pKey->mPosition);

	mat4	xForm;
	KeyFrame_GetMatrix(pKey, xForm);

	xForm[2][1]	=-xForm[2][1];
	xForm[2][2]	=-xForm[2][2];
	xForm[2][3]	=-xForm[2][3];
	xForm[2][4]	=-xForm[2][4];

	xForm[0][3]	=-xForm[0][3];
	xForm[1][3]	=-xForm[1][3];
	xForm[2][3]	=-xForm[2][3];
	xForm[3][3]	=-xForm[3][3];

	mat4	rotMat;
	glm_decompose(xForm, pKey->mPosition, rotMat, pKey->mScale);

	glm_mat4_quat(rotMat, pKey->mRotation);
}


void	KeyFrame_GetMatrix(const KeyFrame *pKey, mat4 mat)
{
	mat4	scale, rot, pos;

	glm_scale_make(scale, pKey->mScale);

	glm_quat_mat4(pKey->mRotation, rot);

	glm_translate_make(pos, pKey->mPosition);
 
	//scale * rot * pos the directX way windows side
	//here it seems to be pos * rot * scale
	glm_mat4_mul(pos, rot, mat);
	glm_mat4_mul(scale, mat, mat);
}

void	KeyFrame_GetMatrixOtherWay(const KeyFrame *pKey, mat4 mat)
{
	mat4	scale, rot, pos;

	glm_scale_make(scale, pKey->mScale);

	glm_quat_mat4(pKey->mRotation, rot);

	glm_translate_make(pos, pKey->mPosition);
 
	glm_mat4_mul(rot, scale, mat);
	glm_mat4_mul(pos, mat, mat);
}