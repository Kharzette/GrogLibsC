#include	<stdint.h>
#include	<stdbool.h>
#include	<assert.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<x86intrin.h>
#include	<cglm/call.h>


typedef struct GameCamera_t
{
	//view quaternion
	vec4	mView;
	vec3	mPos;	//worldspace position

	//distance from the attached object if any (for 3rd person)
	float	mMinDist, mMaxDist;
	float	mCurDistance;

	//pitch angle limits, good for grounded bipedal stuff
	float	mMinPitch, mMaxPitch;

	//projection matrix
	//contains clip distances, fov, and aspect ratio
	mat4	mProjection;

	//set for a spacelike no fixed up/forward etc
	bool	mbSpaceMode;

}	GameCamera;


//fov is VERTICAL fov
GameCamera	*GameCam_Create(bool bSpaceMode, float near, float far,
			float fov, float aspect, float minDist, float maxDist)
{
	GameCamera	*pRet	=malloc(sizeof(GameCamera));

	memset(pRet, 0, sizeof(GameCamera));

	assert(minDist > 0.0f && minDist < maxDist);

	pRet->mCurDistance	=(minDist + maxDist) * 0.5f;
	pRet->mMinDist		=minDist;
	pRet->mMaxDist		=maxDist;

	glmc_perspective(fov, aspect, near, far, pRet->mProjection);

	return	pRet;
}

static void	RotationAxis(const vec3 axis, float angle, vec4 dest)
{
	dest[0]	=axis[0];
	dest[1]	=axis[1];
	dest[2]	=axis[2];
	dest[3]	=angle;
}

//standard fps update
void	GameCam_Update(GameCamera *pCam, vec3 pos, float deltaPitch, float deltaYaw, float deltaRoll)
{
	vec4	accum, rotX, rotY;
	vec3	up		={	0.0f, 1.0f, 0.0f	};
	vec3	side	={	1.0f, 0.0f, 0.0f	};

	//space has no consistent up
	if(pCam->mbSpaceMode)
	{
		glm_quat_rotatev(pCam->mView, up, up);
	}
	
	glm_quat_rotatev(pCam->mView, side, side);

	glm_quat(rotX, deltaPitch, side[0], side[1], side[2]);	//Note negation!
	glm_quat(rotY, deltaYaw, up[0], up[1], up[2]);

	glm_quat_mul_sse2(rotY, rotX, accum);
	glm_quat_mul_sse2(accum, pCam->mView, accum);

	glm_quat_normalize_to(accum, pCam->mView);
}

//annoying array goblinry
void	GameCam_GetProjection(const GameCamera *pCam, mat4 outProjection)
{
	memcpy(outProjection, pCam->mProjection, sizeof(mat4));
}

void	GameCam_GetForwardVec(const GameCamera *pCam, vec3 outForward)
{
	vec3	forward	={	0.0f, 0.0f, 1.0f	};

	glm_quat_rotatev(pCam->mView, forward, outForward);
}

void	GameCam_GetRightVec(const GameCamera *pCam, vec3 outRight)
{
	vec3	right	={	1.0f, 0.0f, 0.0f	};

	glm_quat_rotatev(pCam->mView, right, outRight);
}

void	GameCam_GetUpVec(const GameCamera *pCam, vec3 outUp)
{
	vec3	up	={	0.0f, 1.0f, 0.0f	};

	glm_quat_rotatev(pCam->mView, up, outUp);
}

//third person cam, rotate with tracked object
void	GameCam_GetViewMatrixThird(const GameCamera *pCam,
			const vec3 trackedPos,
			const vec4 attachedRot,
			mat4 mat, vec3 eyePos,
			vec4 centeredView)
{
	vec3	translation	={	0.0f, 0.0f, 1.0f	};

	//combine tracking object + view rotation
	glm_quat_mul_sse2(pCam->mView, attachedRot, centeredView);

	//get combined forward vector
	glm_quat_rotatev(centeredView, translation, translation);

	//translate along the forward vector to cam distance
	glm_vec3_scale(translation, pCam->mCurDistance, translation);

	//add object position
	glm_vec3_add(translation, trackedPos, translation);

	//make cam matrix
	glm_quat_mat4(centeredView, mat);

	//translate
	glm_translate(mat, translation);

	//invert for camera matrix
	glm_mat4_inv_fast_sse2(mat, mat);

	//position of the viewer
	eyePos	=translation;
}

//fly cam, mat is out, cv is out
void	GameCam_GetViewMatrixFly(const GameCamera *pCam,
			mat4 mat, const vec3 eyePos, vec4 centeredView)
{
	mat4	temp;

	//translate
	glm_translate_make(temp, eyePos);

	//make cam matrix
	glm_quat_mat4(pCam->mView, mat);

	//mul together
	glm_mul_sse2(temp, mat, mat);

	//invert for camera matrix
	glm_mat4_inv_fast_sse2(mat, mat);
}
