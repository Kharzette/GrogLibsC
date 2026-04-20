#pragma once
#include	<cglm/call.h>

typedef struct	GameCamera_t	GameCamera;

//creation destruction
GameCamera	*GameCam_Create(bool bSpaceMode, float near, float far,
				float fov, float aspect, float minDist, float maxDist);
void		GameCam_Destroy(GameCamera **ppCam);

//per frame updates for controls
void	GameCam_UpdateRotationFPS(GameCamera *pCam, vec3 pos, float deltaPitch,
									float deltaYaw, float deltaRoll);
void	GameCam_UpdateTopDown(GameCamera *pCam, vec3 objectPos, float deltaYaw);
void	GameCam_UpdateRotationSecondary(GameCamera *pCam, vec3 objectPos, float deltaSeconds,
										float deltaPitch, float deltaYaw, float deltaRoll,
										bool bMoving);

//matrix gets
void	GameCam_GetViewMatrixThird(const GameCamera *pCam, mat4 mat, vec3 eyePos);
void	GameCam_GetViewMatrixFly(const GameCamera *pCam, mat4 mat, vec3 eyePos);
void	GameCam_GetLookMatrix(const GameCamera *pCam, mat4 mat);
void	GameCam_GetFlatLookMatrix(const GameCamera *pCam, mat4 mat);
void	GameCam_GetProjection(const GameCamera *pCam, mat4 outProjection);

//gets for orientation and backprojection
void	GameCam_GetForwardVec(const GameCamera *pCam, bool bThird, vec3 outForward);
void	GameCam_GetRightVec(const GameCamera *pCam, bool bThird, vec3 outRight);
void	GameCam_GetUpVec(const GameCamera *pCam, bool bThird, vec3 outUp);
void	GameCam_UnProjectAngry(const GameCamera *pCam, bool bThird,
								const vec4 viewPort, float minZ, float maxZ,
								const vec2 screenPoint,
								vec3 viewPointNear, vec3 viewPointFar);

//setup top down initial state
void	GameCam_SetTopDown(GameCamera *pCam, const vec3 eyePos,
	float yawDegrees, float pitchDegrees, float rollDegrees);