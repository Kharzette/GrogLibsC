#include	<stdint.h>
#include	<stdbool.h>
#include	<string.h>
#include	<assert.h>
#include	<cglm/call.h>
#include	"GameCamera.h"


//minimum move length
//this helps to kick in idle animation
#define	MIN_MOVE_LENGTH	0.01f

//movement modes
#define	BPM_MOVE_GROUND	1
#define	BPM_MOVE_FLY	2
#define	BPM_MOVE_SWIM	4

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
	int		mMoveMethod;	//ground/fly/swim
	bool	mbMovedThisFrame;

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

	memset(pRet, 0, sizeof(BipedMover));

	pRet->mpGCam		=pGCam;
	pRet->mMoveMethod	=BPM_MOVE_GROUND;	//default

	return	pRet;
}

void	BPM_SetMoveMethod(BipedMover *pBM, int method)
{
	pBM->mMoveMethod	=method;
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

static void	UpdateFlying(BipedMover *pBPM, vec3 move)
{
	vec3	forward, right, up;
	GameCam_GetForwardVec(pBPM->mpGCam, forward);
	GameCam_GetRightVec(pBPM->mpGCam, right);
	GameCam_GetUpVec(pBPM->mpGCam, up);

	FlyMove(pBPM, forward, right, up, move);

	if(pBPM->mbJump)
	{
		move[1]	+=1.0f;
	}

	if(!glm_vec3_eq_eps(move, 0.0f))
	{
		glm_normalize(move);
	}
}

//return a bool indicating jumped
static void	UpdateWalking(BipedMover *pBPM, vec3 move)
{
	vec3	forward, right, up, moveVec;
	GameCam_GetForwardVec(pBPM->mpGCam, forward);
	GameCam_GetRightVec(pBPM->mpGCam, right);
	GameCam_GetUpVec(pBPM->mpGCam, up);

	GroundMove(pBPM, forward, right, up, move);
}


bool	BPM_Update(BipedMover *pBPM, vec3 moveVec)
{
	pBPM->mbMovedThisFrame	=false;

	glm_vec3_zero(moveVec);

	if(pBPM->mMoveMethod == BPM_MOVE_FLY
		|| pBPM->mMoveMethod == BPM_MOVE_SWIM)
	{
		UpdateFlying(pBPM, moveVec);
	}
	else if(pBPM->mMoveMethod == BPM_MOVE_GROUND)
	{
		UpdateWalking(pBPM, moveVec);
	}
	else
	{
		assert(false);
	}

	float	len	=glm_vec3_norm(moveVec);
	if(len <= MIN_MOVE_LENGTH)
	{
		glm_vec3_zero(moveVec);
	}
	else
	{
		glm_normalize(moveVec);
	}

	bool	bJumped	=pBPM->mbJump;
	
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