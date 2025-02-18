#pragma once
#include	<cglm/call.h>

#ifdef	__cplusplus
extern "C"	{
#endif

//c defines for layers
#define	LAY_NON_MOVING					0
#define	LAY_MOVING_FRIENDLY				1
#define	LAY_MOVING_ENEMY				2
#define	LAY_MOVING_FRIENDLY_PROJECTILE	3
#define	LAY_MOVING_ENEMY_PROJECTILE		4

typedef struct	PhysicsStuff_t	PhysicsStuff;


//creation
PhysicsStuff	*Phys_Create(void);
void			Phys_Destroy(PhysicsStuff **ppPS);

//update
void	Phys_Update(PhysicsStuff *pPS, float secDelta);

//add bodies
uint32_t	Phys_CreateAndAddSphere(PhysicsStuff *pPS, float radius,
				const vec3 pos, uint16_t layer);
uint32_t	Phys_CreateAndAddHeightField(PhysicsStuff *pPS,
	const float *pHeights, const vec3 org, uint32_t squareSize);

//nuke bodies	
void	Phys_RemoveAndDestroyBody(PhysicsStuff *pPS, uint32_t bodyID);

//get body data
void	Phys_GetBodyPos(const PhysicsStuff *pPS, uint32_t bodyID, vec3 pos);
void	Phys_GetBodyLayer(const PhysicsStuff *pPS, uint32_t bodyID, uint16_t *pLayer);

//set body data
void	Phys_SetRestitution(PhysicsStuff *pPS, uint32_t bodyID, float resti);

#ifdef	__cplusplus
}
#endif