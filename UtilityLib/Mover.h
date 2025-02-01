#pragma once
#include	<cglm/call.h>

typedef struct	Mover_t	Mover;

Mover	*Mover_Create(void);

//all times in seconds
void	Mover_SetUpMove(Mover *pMv, const vec4 startPos, const vec4 endPos,
	float travelTime, float easeInPercent, float easeOutPercent);

void	Mover_Update(Mover *pMv, float secDelta);
void	Mover_GetPos(const Mover *pMv, vec4 outPos);
bool	Mover_IsDone(const Mover *pMv);