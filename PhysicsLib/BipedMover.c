#include	<stdint.h>
#include	<stdbool.h>
#include	<string.h>
#include	<assert.h>
#include	<cglm/call.h>
#include	"../UtilityLib/GameCamera.h"
#include	"BipedMover.h"


//physical constants
#define	JOG_MOVE_FORCE		37.5f	//Fig Newtons
#define	FLY_MOVE_FORCE		18.75f	//Fig Newtons
#define	FLY_UP_MOVE_FORCE	5.625f	//Fig Newtons
#define	MIDAIR_MOVE_FORCE	1.875f	//Slight wiggle midair
#define	SWIM_MOVE_FORCE		16.875f	//Swimmery
#define	SWIM_UP_MOVE_FORCE	16.875f	//Swimmery
#define	STUMBLE_MOVE_FORCE	13.125f	//Fig Newtons
#define	JUMP_FORCE			375		//leapometers
#define	GRAVITY_FORCE		9.8f	//Gravitons
#define	BUOYANCY_FORCE		13.125f	//Gravitons
#define	GROUND_FRICTION		10.0f	//Frictols
#define	STUMBLE_FRICTION	6.0f	//Frictols
#define	AIR_FRICTION		0.1f	//Frictols
#define	FLY_FRICTION		2.0f	//Frictols
#define	SWIM_FRICTION		10.0f	//Frictols

//minimum move length
//this helps to kick in idle animation
#define	MIN_MOVE_LENGTH	0.01f

//typical character size
#define	PLAYER_RADIUS	0.5f

//movement modes
#define	MOVE_GROUND	1
#define	MOVE_FLY	2
#define	MOVE_SWIM	4

__attribute_maybe_unused__
static const	vec3	UnitX	={	1.0f, 0.0f, 0.0f	};
__attribute_maybe_unused__
static const	vec3	UnitY	={	0.0f, 1.0f, 0.0f	};
__attribute_maybe_unused__
static const	vec3	UnitZ	={	0.0f, 0.0f, 1.0f	};
__attribute_maybe_unused__
static const	vec3	Down	={	0.0f, -1.0f, 0.0f	};

typedef struct  BipedMover_t
{
	JPH_CharacterVirtual	*mpCV;

	//shapes of characters
	JPH_CapsuleShape	*mpStanding;
	JPH_CapsuleShape	*mpCrouching;

	//collision stuff
	JPH_ObjectLayer	mObjLayer;

	//jolt settings
	bool	mbCanPushOthers;
	bool	mbCanBePushed;
	float	mStepHeight;

	vec3	mVelocity;
	vec3	mCamVelocity;

	int		mMoveMethod;	//ground/fly/swim
	bool	mbMovedThisFrame;
	bool	mbSprint;		//sprinting?

	//single frame inputs
	bool	mbForward, mbBack;
	bool	mbLeft, mbRight;
	bool	mbUp, mbDown;
	bool	mbJump;

	GameCamera	*mpGCam;
}	BipedMover;


BipedMover	*BPM_Create(GameCamera *pGCam,
						JPH_PhysicsSystem *pPS,
						JPH_ObjectLayer objLayer,
						float radius, float height,
						float stepHeight,
						const vec3 initialPos)
{
	BipedMover	*pRet	=malloc(sizeof(BipedMover));

	memset(pRet, 0, sizeof(BipedMover));

	pRet->mpGCam		=pGCam;
	pRet->mMoveMethod	=MOVE_GROUND;	//default
	pRet->mStepHeight	=stepHeight;
	pRet->mObjLayer		=objLayer;

	//center of biped capsule
	JPH_Vec3	center	={	0, height * 0.5f, 0	};

	//upvec
	JPH_Vec3	up	={	0, 1, 0	};

	//any contact behind this plane will "support"
	JPH_Plane	sup	={	up, -(height * 0.5f)	};

	pRet->mpStanding	=JPH_CapsuleShape_Create(height * 0.5f, radius);
	pRet->mpCrouching	=JPH_CapsuleShape_Create(height * 0.25f, radius);

	JPH_RotatedTranslatedShape	*pRTS	=JPH_RotatedTranslatedShape_Create(
		&center, NULL, (const JPH_Shape *) pRet->mpStanding);

	JPH_CharacterVirtualSettings	cvs;
	JPH_CharacterVirtualSettings_Init(&cvs);

	cvs.base.shape				=(const JPH_Shape *)pRTS;
	cvs.base.supportingVolume	=sup;

	JPH_Vec3	pos	={	initialPos[0], initialPos[1], initialPos[2]	};

	pRet->mpCV	=JPH_CharacterVirtual_Create(&cvs, &pos, NULL, 0, pPS);

	return	pRet;
}

void	BPM_SetMoveMethod(BipedMover *pBM, int method)
{
	pBM->mMoveMethod	=method;
}

bool    BPM_IsGoodFooting(const BipedMover *pBPM)
{
//	JPH_GroundState	gs	=JPH_CharacterBase_GetGroundState(pBPM->mpCV);

//	return	(gs == JPH_GroundState_OnGround);
	return	true;
}


static void AccumulateVelocity(BipedMover *pBPM, vec3 moveVec)
{
	glm_vec3_muladds(moveVec, 0.5f, pBPM->mCamVelocity);
}

static void	ApplyFriction(BipedMover *pBPM, float secDelta, float friction)
{
	glm_vec3_muladds(pBPM->mCamVelocity, -friction * secDelta * 0.5f, pBPM->mCamVelocity);
}

static void	ApplyForce(BipedMover *pBPM, float force, const vec3 direction, float secDelta)
{
	glm_vec3_muladds(direction, force * (secDelta * 0.5f), pBPM->mCamVelocity);
}

static void	FlyMove(BipedMover *pBPM, const vec3 forward,
					const vec3 right, const vec3 up, vec3 move)
{
	glm_vec3_zero(move);

	if(pBPM->mbForward)
	{
		pBPM->mbMovedThisFrame	=true;
		glm_vec3_add(move, forward, move);
	}
	if(pBPM->mbBack)
	{
		pBPM->mbMovedThisFrame	=true;
		glm_vec3_sub(move, forward, move);
	}

	if(pBPM->mbRight)
	{
		pBPM->mbMovedThisFrame	=true;
		glm_vec3_add(move, right, move);
	}
	if(pBPM->mbLeft)
	{
		pBPM->mbMovedThisFrame	=true;
		glm_vec3_sub(move, right, move);
	}

	if(pBPM->mbUp)
	{
		pBPM->mbMovedThisFrame	=true;
		glm_vec3_add(move, up, move);
	}
	if(pBPM->mbDown)
	{
		pBPM->mbMovedThisFrame	=true;
		glm_vec3_sub(move, up, move);
	}
}

static void	GroundMove(BipedMover *pBPM, const vec3 forward,
					   const vec3 right, const vec3 up, vec3 move)
{
	FlyMove(pBPM, forward, right, up, move);

	//flatten movement
	move[1]	=0.0f;
}

static void	UpdateSwimming(BipedMover *pBPM, float secDelta, vec3 move)
{
	vec3	forward, right, up, moveVec;
	GameCam_GetForwardVec(pBPM->mpGCam, forward);
	GameCam_GetRightVec(pBPM->mpGCam, right);
	GameCam_GetUpVec(pBPM->mpGCam, up);

	FlyMove(pBPM, forward, right, up, moveVec);

	glm_vec3_scale(moveVec, SWIM_MOVE_FORCE * secDelta, moveVec);

	AccumulateVelocity(pBPM, moveVec);

	if(pBPM->mbJump)
	{
//		if(bCanClimbOut())
//		{
//			ApplyForce(pBPM, JUMP_FORCE, UnitY, secDelta);
//		}
//		else
		{
			ApplyForce(pBPM, SWIM_MOVE_FORCE, UnitY, secDelta);
		}
	}

	//friction / gravity / bouyancy
	ApplyFriction(pBPM, secDelta, SWIM_FRICTION);
	ApplyForce(pBPM, GRAVITY_FORCE, Down, secDelta);
	ApplyForce(pBPM, BUOYANCY_FORCE, UnitY, secDelta);

	//move vector for the frame
	glm_vec3_scale(pBPM->mCamVelocity, secDelta, move);

	if(pBPM->mbJump)
	{
		ApplyForce(pBPM, SWIM_UP_MOVE_FORCE, UnitY, secDelta);
	}

	//friction / gravity / bouyancy
	ApplyFriction(pBPM, secDelta, SWIM_FRICTION);
	ApplyForce(pBPM, GRAVITY_FORCE, Down, secDelta);
	ApplyForce(pBPM, BUOYANCY_FORCE, UnitY, secDelta);
}

static void	UpdateFlying(BipedMover *pBPM, float secDelta, vec3 move)
{
	vec3	forward, right, up, moveVec;
	GameCam_GetForwardVec(pBPM->mpGCam, forward);
	GameCam_GetRightVec(pBPM->mpGCam, right);
	GameCam_GetUpVec(pBPM->mpGCam, up);

	FlyMove(pBPM, forward, right, up, moveVec);

	glm_vec3_scale(moveVec, FLY_MOVE_FORCE * secDelta, moveVec);

	AccumulateVelocity(pBPM, moveVec);
	ApplyFriction(pBPM, secDelta, FLY_FRICTION);

	if(pBPM->mbJump)
	{
		ApplyForce(pBPM, FLY_UP_MOVE_FORCE, UnitY, secDelta);
	}

	//move vector for the frame
	glm_vec3_scale(pBPM->mCamVelocity, secDelta, move);
//	glm_vec3_muladds(pBPM->mCamVelocity, secDelta, pos);

	AccumulateVelocity(pBPM, moveVec);
	ApplyFriction(pBPM, secDelta, FLY_FRICTION);

	if(pBPM->mbJump)
	{
		ApplyForce(pBPM, FLY_UP_MOVE_FORCE, UnitY, secDelta);
	}
}

//return a bool indicating jumped
static bool	UpdateWalking(BipedMover *pBPM,
	JPH_PhysicsSystem *pPS,
	float secDelta)
{
	JPH_ExtendedUpdateSettings	eus	={0};

	eus.stickToFloorStepDown.y	=-pBPM->mStepHeight;
	eus.walkStairsStepUp.y		=pBPM->mStepHeight;

	JPH_CharacterVirtual_ExtendedUpdate(pBPM->mpCV,
		secDelta, &eus, pBPM->mObjLayer, pPS, NULL, NULL);

	return	false;
}


bool	BPM_Update(BipedMover *pBPM, JPH_PhysicsSystem *pPS, float secDelta)
{
	pBPM->mbMovedThisFrame	=false;

	bool	bJumped	=false;

//	if(pBPM->mMoveMethod == MOVE_FLY)
//	{
//		UpdateFlying(pBPM, secDelta, moveVec);
//	}
//	else if(pBPM->mMoveMethod == MOVE_GROUND)
	{
		bJumped	=UpdateWalking(pBPM, pPS, secDelta);
	}
//	else if(pBPM->mMoveMethod == MOVE_SWIM)
//	{
//		UpdateSwimming(pBPM, secDelta, moveVec);
//	}
//	else
//	{
//		assert(false);
//	}


	//reset variables for next frame
	pBPM->mbForward	=false;
	pBPM->mbBack	=false;
	pBPM->mbLeft	=false;
	pBPM->mbRight	=false;
	pBPM->mbUp		=false;
	pBPM->mbDown	=false;
	pBPM->mbJump	=false;

	pBPM->mbMovedThisFrame	=false;

	return	bJumped;
}


//input handlers
void	BPM_InputForward(BipedMover *pBPM)
{
	pBPM->mbForward	=true;
}

void	BPM_InputBack(BipedMover *pBPM)
{
	pBPM->mbBack	=true;
}

void	BPM_InputUp(BipedMover *pBPM)
{
	pBPM->mbUp	=true;
}

void	BPM_InputDown(BipedMover *pBPM)
{
	pBPM->mbDown	=true;
}

void	BPM_InputLeft(BipedMover *pBPM)
{
	pBPM->mbLeft	=true;
}

void	BPM_InputRight(BipedMover *pBPM)
{
	pBPM->mbRight	=true;
}

void	BPM_InputJump(BipedMover *pBPM)
{
	pBPM->mbJump	=true;
}

void	BPM_InputSprint(BipedMover *pBPM, bool bOn)
{
	pBPM->mbSprint	=bOn;
}