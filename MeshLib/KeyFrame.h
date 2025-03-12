#pragma once
#include	<stdint.h>
#include	<cglm/call.h>


//this is one of my few public structs
typedef struct	KeyFrame_t
{
	vec4	mRotation;
	vec3	mPosition;
	vec3	mScale;
}	KeyFrame;


void	KeyFrame_Read(FILE *f, KeyFrame *pKey);
void	KeyFrame_Write(const KeyFrame *pKey, FILE *f);

void	KeyFrame_Identity(KeyFrame *pKey);
void	KeyFrame_ConvertToLeftHanded(KeyFrame *pKey);

void	KeyFrame_GetMatrix(const KeyFrame *pKey, mat4 mat);
void	KeyFrame_GetMatrixOtherWay(const KeyFrame *pKey, mat4 mat);
void	KeyFrame_Lerp(const KeyFrame *pKey0, const KeyFrame *pKey1,
				float percentage, KeyFrame *pKeyResult);
