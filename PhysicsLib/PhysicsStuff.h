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

//movement modes
#define	MOVE_WALK	1
#define	MOVE_RUN	2
#define	MOVE_FLY	4
#define	MOVE_SWIM	8

typedef struct	PhysicsStuff_t		PhysicsStuff;
typedef struct	PhysCharacter_t		PhysCharacter;
typedef struct	PhysVCharacter_t	PhysVCharacter;


//creation
PhysicsStuff	*Phys_Create(void);
void			Phys_Destroy(PhysicsStuff **ppPS);

//character stuff
PhysCharacter	*Phys_CreateCharacter(PhysicsStuff *pPS,
	float radius, float height,
	const vec3 org, uint16_t layer);
PhysVCharacter	*Phys_CreateVCharacter(PhysicsStuff *pPS,
		float radius, float height,
		const vec3 org);
void	Phys_CharacterDestroy(PhysicsStuff *pPS, PhysCharacter *pChar);
void	Phys_VCharacterDestroy(PhysicsStuff *pPS, PhysVCharacter *pChar);
void	Phys_CharacterMove(PhysicsStuff *pPS, PhysCharacter *pChar,
			const vec3 move, bool bJump, bool bStanceSwitch, float secDelta);
void	Phys_VCharacterMove(PhysicsStuff *pPS, PhysVCharacter *pChar,
			const vec3 move, float secDelta, vec3 resultVelocity);
void	Phys_CharacterGetPos(const PhysCharacter *pChar, vec3 pos);
void	Phys_VCharacterGetGroundNormal(const PhysVCharacter *pChar, vec3 normal);
void	Phys_VCharacterGetPos(const PhysVCharacter *pChar, vec3 pos);
bool	Phys_CharacterIsSupported(const PhysCharacter *pChar);
bool	Phys_VCharacterIsSupported(const PhysVCharacter *pChar);

//update
void	Phys_Update(PhysicsStuff *pPS, float secDelta);

//add bodies
uint32_t	Phys_CreateAndAddBox(PhysicsStuff *pPS, float height,
				float width, float depth, const vec3 org, uint16_t layer);
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

//queries
bool	Phys_CastRayAtBodyBroad(const PhysicsStuff *pPS, vec3 org, uint32_t bodyID);
bool	Phys_CastRayAtBodyNarrow(const PhysicsStuff *pPS, vec3 org, uint32_t bodyID);

#ifdef	__cplusplus
}
#endif