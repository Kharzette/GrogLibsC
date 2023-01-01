#include	<stdint.h>
#include	<stdio.h>
#include	<assert.h>
#include	"KeyFrame.h"


void	KeyFrame_Lerp(const KeyFrame *pKey0, const KeyFrame *pKey1,
				float percentage, KeyFrame *pKeyResult)
{
	glmc_quat_slerp(pKey0->mRotation, pKey1->mRotation, percentage, pKeyResult->mRotation);
}


void	KeyFrame_Read(FILE *f, KeyFrame *pKey)
{
	fread(pKey->mRotation, sizeof(vec4), 1, f);
	fread(pKey->mPosition, sizeof(vec3), 1, f);
	fread(pKey->mScale, sizeof(vec3), 1, f);
}


void	KeyFrame_GetMatrix(const KeyFrame *pKey, mat4 mat)
{
	mat4	scale, rot, pos, temp;

	glmc_scale_make(scale, pKey->mScale);

	glmc_quat_mat4(pKey->mRotation, rot);

	glmc_translate_make(pos, pKey->mPosition);

	//scale * rot * pos the directX way
	glmc_mul(scale, rot, temp);
	glmc_mul(temp, pos, mat);

	//pos * rot * scale
//	glmc_mul(pos, rot, temp);
//	glmc_mul(temp, scale, mat);

	//rot * scale * pos
//	glmc_mul(rot, scale, temp);
//	glmc_mul(temp, pos, mat);

	//pos * scale * rot
//	glmc_mul(pos, scale, temp);
//	glmc_mul(temp, rot, mat);

	//scale * pos * rot
//	glmc_mul(scale, pos, temp);
//	glmc_mul(temp, rot, mat);

	//rot * pos * scale
//	glmc_mul(rot, pos, temp);
//	glmc_mul(temp, scale, mat);
}
