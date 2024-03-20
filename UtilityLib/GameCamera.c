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
	//projection matrix (top to align)
	//contains clip distances, fov, and aspect ratio
	mat4	mProjection;

	//primary view quaternion
	//This is the first person view direction, or the look
	//direction for the object the third person camera is looking at.
	vec4	mView;

	//this is a snapshot of the view quaternion before lerping began
	vec4	mViewSlerp;

	//secondary view quaternion
	//This one is used if a third person camera can spin
	//around the object the third person camera is focused on.
	vec4	mCamQuat;

	//this is the location of the first person camera, or the object
	//the third person camera is tracking
	vec3	mTrackingPosition;

	//distance from the attached object if any (for 3rd person)
	float	mMinDist, mMaxDist;
	float	mCurDistance;

	//pitch angle limits, good for grounded bipedal stuff
	float	mMinPitch, mMaxPitch;

	//time to slerp from 3rd person quat to object quat
	float	mSlerpTime;

	//set for a spacelike camera with no fixed up/forward etc
	bool	mbSpaceMode;

}	GameCamera;


//fov is VERTICAL fov
GameCamera	*GameCam_Create(bool bSpaceMode, float near, float far,
			float fov, float aspect, float minDist, float maxDist)
{
#ifdef	__AVX__
	GameCamera	*pRet	=aligned_alloc(32, sizeof(GameCamera));
#else
	GameCamera	*pRet	=aligned_alloc(16, sizeof(GameCamera));
#endif

	memset(pRet, 0, sizeof(GameCamera));

	assert(minDist > 0.0f && minDist < maxDist);

	pRet->mCurDistance	=(minDist + maxDist) * 0.5f;
	pRet->mMinDist		=minDist;
	pRet->mMaxDist		=maxDist;

	glm_perspective(fov, aspect, near, far, pRet->mProjection);

	return	pRet;
}

//standard fps update
void	GameCam_UpdateRotation(GameCamera *pCam, vec3 objectPos, float deltaPitch, float deltaYaw, float deltaRoll)
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

	glm_quat(rotX, deltaPitch, side[0], side[1], side[2]);
	glm_quat(rotY, deltaYaw, up[0], up[1], up[2]);

	glm_quat_mul(rotY, rotX, accum);
	glm_quat_mul(accum, pCam->mView, accum);

	glm_quat_normalize_to(accum, pCam->mView);

	glm_vec3_copy(objectPos, pCam->mTrackingPosition);
}

void	GameCam_UpdateRotationSecondary(GameCamera *pCam, vec3 objectPos, float deltaSeconds,
										float deltaPitch, float deltaYaw, float deltaRoll, bool bMoving)
{
	vec4	accum, rotX, rotY;
	vec3	up		={	0.0f, 1.0f, 0.0f	};
	vec3	side	={	1.0f, 0.0f, 0.0f	};

	//space has no consistent up
	if(pCam->mbSpaceMode)
	{
		glm_quat_rotatev(pCam->mCamQuat, up, up);
	}
	
	glm_quat_rotatev(pCam->mCamQuat, side, side);

	glm_quat(rotX, deltaPitch, side[0], side[1], side[2]);
	glm_quat(rotY, deltaYaw, up[0], up[1], up[2]);

	glm_quat_mul(rotY, rotX, accum);
	glm_quat_mul(accum, pCam->mCamQuat, accum);

	glm_quat_normalize_to(accum, pCam->mCamQuat);

	glm_vec3_copy(objectPos, pCam->mTrackingPosition);

	//slerp to object quat?
	if(bMoving)
	{
		pCam->mSlerpTime	+=deltaSeconds;

		if(pCam->mSlerpTime < 0.1f)
		{
			glm_quat_slerp(pCam->mViewSlerp, pCam->mCamQuat, pCam->mSlerpTime * 10.0f, pCam->mView);
		}
		else
		{
			glm_vec4_copy(pCam->mCamQuat, pCam->mView);
			glm_vec4_copy(pCam->mView, pCam->mViewSlerp);
		}
	}
	else
	{
		pCam->mSlerpTime	=0.0f;
		glm_vec4_copy(pCam->mView, pCam->mViewSlerp);
	}
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
void	GameCam_GetViewMatrixThird(const GameCamera *pCam, mat4 mat, vec3 eyePos)
{
	vec3	forward	={	0.0f, 0.0f, 1.0f	};

	//get forward vector
	glm_quat_rotatev(pCam->mCamQuat, forward, forward);

	//translate along the back vector to cam distance
	vec3	translation;
	glm_vec3_scale(forward, -pCam->mCurDistance, translation);

	//add tracked object position
	glm_vec3_add(translation, pCam->mTrackingPosition, translation);

	//make cam matrix
	glm_translate_make(mat, translation);

	//rotate
	glm_quat_rotate(mat, pCam->mCamQuat, mat);

	//invert for camera matrix
	glm_mat4_inv_fast_sse2(mat, mat);

	//position of the viewer
	glm_vec3_copy(translation, eyePos);
}

//fly cam, mat is out, eyePos is out
void	GameCam_GetViewMatrixFly(const GameCamera *pCam, mat4 mat, vec3 eyePos)
{
	mat4	temp;

	//translate
	glm_translate_make(temp, pCam->mTrackingPosition);

	//make cam matrix
	glm_quat_mat4(pCam->mView, mat);

	//mul together
	glm_mul_sse2(temp, mat, mat);

	//invert for camera matrix
	glm_mat4_inv_fast_sse2(mat, mat);

	glm_vec3_copy(pCam->mTrackingPosition, eyePos);
}

//returns an object matrix
void	GameCam_GetLookMatrix(const GameCamera *pCam, mat4 mat)
{
	//make cam matrix
	glm_translate_make(mat, pCam->mTrackingPosition);

	//rotate
	glm_quat_rotate(mat, pCam->mView, mat);
}

//keeps the matrix upright and avoids any pitch or roll
void	GameCam_GetFlatLookMatrix(const GameCamera *pCam, mat4 mat)
{
	//make cam matrix
	glm_translate_make(mat, pCam->mTrackingPosition);

	vec4	flatQuat	={	0.0f, pCam->mView[1], 0.0f, pCam->mView[3]	};

	glm_quat_normalize(flatQuat);

	//rotate
	glm_quat_rotate(mat, flatQuat, mat);
}