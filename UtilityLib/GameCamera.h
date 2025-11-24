#pragma once
#include	<cglm/call.h>

typedef struct	GameCamera_t	GameCamera;


GameCamera	*GameCam_Create(bool bSpaceMode, float near, float far,
				float fov, float aspect, float minDist, float maxDist);
void		GameCam_Destroy(GameCamera **ppCam);

void	GameCam_UpdateRotation(GameCamera *pCam, vec3 pos, float deltaPitch,
								float deltaYaw, float deltaRoll);

void	GameCam_UpdateRotationSecondary(GameCamera *pCam, vec3 objectPos, float deltaSeconds,
										float deltaPitch, float deltaYaw, float deltaRoll,
										bool bMoving);

//third person cam, rotate with tracked object
void	GameCam_GetViewMatrixThird(const GameCamera *pCam, mat4 mat, vec3 eyePos);
void	GameCam_GetViewMatrixFly(const GameCamera *pCam, mat4 mat, vec3 eyePos);
void	GameCam_GetLookMatrix(const GameCamera *pCam, mat4 mat);
void	GameCam_GetFlatLookMatrix(const GameCamera *pCam, mat4 mat);

void	GameCam_GetProjection(const GameCamera *pCam, mat4 outProjection);

void	GameCam_GetForwardVec(const GameCamera *pCam, vec3 outForward);
void	GameCam_GetRightVec(const GameCamera *pCam, vec3 outRight);
void	GameCam_GetUpVec(const GameCamera *pCam, vec3 outUp);