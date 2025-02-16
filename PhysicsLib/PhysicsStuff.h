#pragma once
#include	<cglm/call.h>

#ifdef	__cplusplus
extern "C"	{
#endif

typedef struct	PhysicsStuff_t	PhysicsStuff;


//creation
PhysicsStuff	*Phys_Create(void);
void			Phys_Destroy(PhysicsStuff **ppPS);

#ifdef	__cplusplus
}
#endif