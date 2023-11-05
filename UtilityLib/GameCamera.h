#pragma once
#include	<cglm/call.h>

typedef struct	GameCamera_t	GameCamera;


GameCamera	*GameCam_Create(bool bSpaceMode, float near, float far,
			float fov, float aspect, float minDist, float maxDist);

void	GameCam_Update(GameCamera *pCam, vec3 pos, float deltaPitch,
						float deltaYaw, float deltaRoll);

//third person cam, rotate with tracked object
void	GameCam_GetViewMatrixThird(const GameCamera *pCam,
			const vec3 trackedPos,
			const vec4 attachedRot,
			mat4 mat, vec3 eyePos,
			vec4 centeredView);

void	GameCam_GetViewMatrixFly(const GameCamera *pCam,
			mat4 mat, const vec3 eyePos, vec4 centeredView);

const mat4	*GameCam_GetProjection(const GameCamera *pCam);

void	GameCam_GetForwardVec(const GameCamera *pCam, vec3 outForward);
void	GameCam_GetRightVec(const GameCamera *pCam, vec3 outRight);
void	GameCam_GetUpVec(const GameCamera *pCam, vec3 outUp);