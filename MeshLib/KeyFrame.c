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
