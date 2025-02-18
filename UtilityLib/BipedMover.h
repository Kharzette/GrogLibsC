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

//update
bool	BPM_Update(BipedMover *pBPM, float secDelta, vec3 moveVec);

//inputs, these are for a single frame
void	BPM_InputForward(BipedMover *pBPM);
void	BPM_InputBack(BipedMover *pBPM);
void	BPM_InputLeft(BipedMover *pBPM);
void	BPM_InputRight(BipedMover *pBPM);
void	BPM_InputUp(BipedMover *pBPM);
void	BPM_InputDown(BipedMover *pBPM);
void	BPM_InputJump(BipedMover *pBPM);
void	BPM_InputSprint(BipedMover *pBPM, bool bOn);