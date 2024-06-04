#include	<stdint.h>
#include	<stdbool.h>
#include	<assert.h>
#include	<cglm/call.h>
#include	"GameCamera.h"


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
	vec3	mVelocity;
	vec3	mCamVelocity;

	int		mMoveMethod;	//ground/fly/swim
	bool	mbMovedThisFrame;
	bool	mbOnGround;		//player starts frame on ground?
	bool	mbBadFooting;	//unstable or steep ground underfoot
	bool	mbSprint;		//sprinting?

	//single frame inputs
	bool	mbForward, mbBack;
	bool	mbLeft, mbRight;
	bool	mbUp, mbDown;
	bool	mbJump;

	GameCamera	*mpGCam;
}	BipedMover;


BipedMover	*BPM_Create(GameCamera *pGCam)
{
	BipedMover	*pRet	=malloc(sizeof(BipedMover));

	pRet->mpGCam		=pGCam;
	pRet->mMoveMethod	=MOVE_GROUND;	//default

	return	pRet;
}

void	BPM_SetMoveMethod(BipedMover *pBM, int method)
{
	pBM->mMoveMethod	=method;
}

bool    BPM_IsGoodFooting(const BipedMover *pBPM)
{
	return	(pBPM->mbOnGround && !pBPM->mbBadFooting);
}

void	BPM_SetFooting(BipedMover *pBPM, int footing)
{
	if(footing == 0)
	{
		pBPM->mbBadFooting	=false;
		pBPM->mbOnGround	=false;
	}
	else if(footing == 1)
	{
		pBPM->mbBadFooting	=false;
		pBPM->mbOnGround	=true;
	}
	else
	{
		pBPM->mbBadFooting	=true;
		pBPM->mbOnGround	=true;
	}
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
static bool	UpdateWalking(BipedMover *pBPM, float secDelta, vec3 move)
{
	bool	bGravity	=false;
	float	friction	=GROUND_FRICTION;
	bool	bJumped		=false;

	if(pBPM->mbOnGround)
	{
		if(!pBPM->mbBadFooting)
		{
			friction	=GROUND_FRICTION;
		}
		else
		{
			friction	=AIR_FRICTION;
		}
	}
	else
	{
		bGravity	=true;
		friction	=AIR_FRICTION;
	}

	if(pBPM->mbJump && pBPM->mbOnGround)
	{
		bJumped				=true;
		friction			=AIR_FRICTION;
		pBPM->mbOnGround	=false;
	}

	vec3	forward, right, up, moveVec;
	GameCam_GetForwardVec(pBPM->mpGCam, forward);
	GameCam_GetRightVec(pBPM->mpGCam, right);
	GameCam_GetUpVec(pBPM->mpGCam, up);

	GroundMove(pBPM, forward, right, up, moveVec);

	if(pBPM->mbOnGround)
	{
		if(pBPM->mbSprint)
		{
			glm_vec3_scale(moveVec, JOG_MOVE_FORCE * 2.0f * secDelta, moveVec);
		}
		else
		{
			glm_vec3_scale(moveVec, JOG_MOVE_FORCE * secDelta, moveVec);
		}
	}
	else if(pBPM->mbBadFooting)
	{
		glm_vec3_scale(moveVec, STUMBLE_MOVE_FORCE * secDelta, moveVec);
	}
	else
	{
		glm_vec3_scale(moveVec, MIDAIR_MOVE_FORCE * secDelta, moveVec);
	}

	AccumulateVelocity(pBPM, moveVec);
	ApplyFriction(pBPM, secDelta, friction);

	if(bGravity)
	{
		ApplyForce(pBPM, GRAVITY_FORCE, Down, secDelta);
	}

	if(bJumped)
	{
		ApplyForce(pBPM, JUMP_FORCE, UnitY, secDelta);

		//move vector for the frame
		//jump uses a 60fps delta time for consistency
		glm_vec3_scale(pBPM->mCamVelocity, (1.0f / 60.0f), move);
	}
	else
	{
		glm_vec3_scale(pBPM->mCamVelocity, secDelta, move);
//		glm_vec3_muladds(pBPM->mCamVelocity, secDelta, pos);
	}

	AccumulateVelocity(pBPM, moveVec);
	ApplyFriction(pBPM, secDelta, friction);
	if(bGravity)
	{
		ApplyForce(pBPM, GRAVITY_FORCE, Down, secDelta);
	}

	if(bJumped)
	{
		ApplyForce(pBPM, JUMP_FORCE, UnitY, secDelta);
	}

	return	bJumped;
}


bool	BPM_Update(BipedMover *pBPM, float secDelta, vec3 moveVec)
{
	pBPM->mbMovedThisFrame	=false;

	glm_vec3_zero(moveVec);

	bool	bJumped	=false;

	if(pBPM->mMoveMethod == MOVE_FLY)
	{
		UpdateFlying(pBPM, secDelta, moveVec);
	}
	else if(pBPM->mMoveMethod == MOVE_GROUND)
	{
		bJumped	=UpdateWalking(pBPM, secDelta, moveVec);
	}
	else if(pBPM->mMoveMethod == MOVE_SWIM)
	{
		UpdateSwimming(pBPM, secDelta, moveVec);
	}
	else
	{
		assert(false);
	}

	float	len	=glm_vec3_norm(moveVec);
	if(len <= MIN_MOVE_LENGTH && pBPM->mbOnGround)
	{
		glm_vec3_zero(moveVec);
	}
	
//	glm_vec3_muladds(pBPM->mCamVelocity, secDelta, pos);

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