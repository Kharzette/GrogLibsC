#pragma once
#include	<cglm/call.h>
#include	"joltc.h"

//movement modes
#define	MOVE_GROUND	1
#define	MOVE_FLY	2
#define	MOVE_SWIM	4

typedef struct  BipedMover_t    BipedMover;
typedef struct  GameCamera_t    GameCamera;


//creation
BipedMover	*BPM_Create(GameCamera *pGCam,
	JPH_PhysicsSystem *pPS,
	JPH_ObjectLayer objLayer,
	float radius, float height,
	float stepHeight,
	const vec3 initialPos);

//move method
void	BPM_SetMoveMethod(BipedMover *pBM, int method);

//update
bool	BPM_Update(BipedMover *pBPM, JPH_PhysicsSystem *pPS, float secDelta);

//set footing
void    BPM_SetFooting(BipedMover *pBPM, int footing);

//get footing
bool    BPM_IsGoodFooting(const BipedMover *pBPM);

//inputs, these are for a single frame
void	BPM_InputForward(BipedMover *pBPM);
void	BPM_InputBack(BipedMover *pBPM);
void	BPM_InputLeft(BipedMover *pBPM);
void	BPM_InputRight(BipedMover *pBPM);
void	BPM_InputUp(BipedMover *pBPM);
void	BPM_InputDown(BipedMover *pBPM);
void	BPM_InputJump(BipedMover *pBPM);
void	BPM_InputSprint(BipedMover *pBPM, bool bOn);