#pragma once
#include	<cglm/call.h>

#ifdef	__cplusplus
extern "C"	{
#endif

typedef struct	PhysicsStuff_t	PhysicsStuff;


//creation
PhysicsStuff	*Phys_Create(void);
void			Phys_Destroy(PhysicsStuff **ppPS);

//update
void	Phys_Update(PhysicsStuff *pPS, float secDelta);

//add bodies
uint32_t	Phys_CreateAndAddSphere(PhysicsStuff *pPS, float radius, const vec3 pos);
uint32_t	Phys_CreateAndAddHeightField(PhysicsStuff *pPS,
	const float *pHeights, const vec3 org, uint32_t squareSize);

//nuke bodies	
void	Phys_RemoveAndDestroyBody(PhysicsStuff *pPS, uint32_t bodyID);

//get body data
void	Phys_GetBodyPos(const PhysicsStuff *pPS, uint32_t bodyID, vec3 pos);

//set body data
void	Phys_SetRestitution(PhysicsStuff *pPS, uint32_t bodyID, float resti);

#ifdef	__cplusplus
}
#endif