#include	<stdint.h>
#include	<string.h>
#include	<cglm/call.h>
#include	<cglm/box.h>
#include	<assert.h>
#include	"MiscStuff.h"

#define	MINIMUM_DISTANCE	(0.001f)


typedef struct	Mover_t
{
	//passed in setup stuff
	vec4	mTargetPos;
	vec4	mStartPos;	
	float	mEaseInPercent;		//percentage of time to ease in
	float	mEaseOutPercent;	//percentage of time to ease out
	float	mTravelTime;

	//calculated stuff
	float	mStage1Accel;		//accel for easein stage
	float	mStage3Accel;		//decel for easeout stage
	float	mMaxVelocity;		//maximum velocity for stage 2
	float	mTotalDistance;
	vec4	mStage1EndPos;		//position at the end of stage 1
	vec4	mStage2EndPos;		//position at the end of stage 2

	//stuff moving on the fly
	vec4	mCurPos;
	float	mCurTime;
	bool	mbDone;

}	Mover;


Mover	*Mover_Create(void)
{
	Mover	*pRet	=malloc(sizeof(Mover));
	memset(pRet, 0, sizeof(Mover));

	pRet->mbDone	=true;	//start done

	return	pRet;
}

void	Mover_GetPos(const Mover *pMv, vec4 outPos)
{
	glm_vec4_copy(pMv->mCurPos, outPos);
}

bool	Mover_IsDone(const Mover *pMv)
{
	return	pMv->mbDone;
}

//all times in seconds
void	Mover_SetUpMove(Mover *pMv, const vec4 startPos, const vec4 endPos,
	float travelTime, float easeInPercent, float easeOutPercent)
{
	//grab movement vital stats
	glm_vec4_copy(startPos, pMv->mStartPos);
	glm_vec4_copy(endPos, pMv->mTargetPos);
	pMv->mTravelTime	=travelTime;

	//ease percentages
	pMv->mEaseInPercent		=easeInPercent;
	pMv->mEaseOutPercent	=easeOutPercent;

	//set up calculations
	glm_vec4_copy(startPos, pMv->mCurPos);
	pMv->mCurTime	=0.0f;

	vec4	distVec;
	glm_vec4_sub(endPos, startPos, distVec);

	//vector length
	pMv->mTotalDistance	=glm_vec4_norm(distVec);

	if(pMv->mTotalDistance < MINIMUM_DISTANCE)
	{
		pMv->mbDone	=true;
		return;
	}

	//normalize distvec
	glm_vec4_scale(distVec, 1.0f / pMv->mTotalDistance, distVec);

	float	timeSlice	=0.5f * easeInPercent * travelTime * travelTime
		* (2 - easeInPercent - easeOutPercent);

	pMv->mStage1Accel	=pMv->mTotalDistance / timeSlice;

	pMv->mStage3Accel	=-pMv->mStage1Accel * 0.5f * (easeInPercent / easeOutPercent);

	float	stage1Time	=(easeInPercent * travelTime);

	//figure out what the max velocity will be
	pMv->mMaxVelocity	=pMv->mStage1Accel * stage1Time;

	vec4	distScaled;
	glm_vec4_scale(distVec, 0.5f * (pMv->mStage1Accel * stage1Time * stage1Time), distScaled);
	glm_vec4_add(pMv->mStartPos, distScaled, pMv->mStage1EndPos);
						
	float	timeAtMaxVelocity	=(travelTime * (1.0f - (easeInPercent + easeOutPercent)));
	glm_vec4_scale(distVec, (pMv->mMaxVelocity * timeAtMaxVelocity), distScaled);
	glm_vec4_add(pMv->mStage1EndPos, distScaled, pMv->mStage2EndPos);

	pMv->mbDone	=false;
}

void	Mover_Update(Mover *pMv, float secDelta)
{
	assert(secDelta > 0.0f);

	//get the direction vector
	vec4	dir;
	glm_vec4_sub(pMv->mTargetPos, pMv->mStartPos, dir);

	//make unit
	glm_vec4_normalize(dir);

	//get current time
	pMv->mCurTime	+=secDelta;

	//limited movement
	if(pMv->mCurTime > pMv->mTravelTime)
	{
		glm_vec4_copy(pMv->mTargetPos, pMv->mCurPos);
		pMv->mbDone	=true;
		return;
	}

	//figure out where we are along
	//the path of motion based on time
	float	scalar	=pMv->mCurTime / pMv->mTravelTime;

	//see if our current position falls within
	//the boundaries of ease in / out
	if(scalar < pMv->mEaseInPercent)
	{
		//use the stage one accel
		float	curVelocity	=0.5f * pMv->mStage1Accel * pMv->mCurTime;

		glm_vec4_scale(dir, curVelocity * pMv->mCurTime, dir);
		glm_vec4_add(pMv->mStartPos, dir, pMv->mCurPos);
	}
	else if(scalar > (1.0f - pMv->mEaseOutPercent))
	{
		//reduce time to zero for stage
		float	timeRampingUp		=(pMv->mTravelTime * pMv->mEaseInPercent);
		float	timeAtMaxVelocity	=(pMv->mTravelTime * (1.0f - (pMv->mEaseInPercent + pMv->mEaseOutPercent)));
		float	stage3Time			=(pMv->mCurTime - timeAtMaxVelocity - timeRampingUp);

		float	curVelocity	=pMv->mMaxVelocity + (pMv->mStage3Accel * stage3Time);

		glm_vec4_scale(dir, curVelocity * stage3Time, dir);
		glm_vec4_add(pMv->mStage2EndPos, dir, pMv->mCurPos);
	}
	else
	{
		//second stage is maximum velocity
		//reduce time to zero for stage start
		float	timeRampingUp	=(pMv->mTravelTime * pMv->mEaseInPercent);

		glm_vec4_scale(dir, pMv->mMaxVelocity * (pMv->mCurTime - timeRampingUp), dir);
		glm_vec4_add(pMv->mStage1EndPos, dir, pMv->mCurPos);
	}
}
