#pragma once
#include	<cglm/call.h>

//movement modes
#define	BPM_MOVE_GROUND	1
#define	BPM_MOVE_FLY	2
#define	BPM_MOVE_SWIM	4

typedef struct  BipedMover_t    BipedMover;
typedef struct  GameCamera_t    GameCamera;


//creation
BipedMover	*BPM_Create(GameCamera *pGC);

//move method
void	BPM_SetMoveMethod(BipedMover *pBM, int method);
void	BPM_SetVelocity(BipedMover *pBM, const vec3 vel);
void	BPM_GetVelocity(const BipedMover *pBPM, vec3 vel);

//update, return true if jumped
bool	BPM_Update(BipedMover *pBPM, bool bSupported,
					float secDelta, vec3 moveVec);

//inputs, these are for a single frame
void	BPM_InputForward(BipedMover *pBPM);
void	BPM_InputBack(BipedMover *pBPM);
void	BPM_InputLeft(BipedMover *pBPM);
void	BPM_InputRight(BipedMover *pBPM);
void	BPM_InputUp(BipedMover *pBPM);
void	BPM_InputDown(BipedMover *pBPM);
void	BPM_InputJump(BipedMover *pBPM);
void	BPM_InputSprint(BipedMover *pBPM, bool bOn);