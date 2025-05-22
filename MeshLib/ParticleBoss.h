#pragma once
#include	<cglm/call.h>

#define	EMIT_SHAPE_POINT	0
#define	EMIT_SHAPE_SPHERE	1
#define	EMIT_SHAPE_BOX		2
#define	EMIT_SHAPE_LINE		3
#define	EMIT_SHAPE_PLANE	4


typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	ParticleBoss_t		ParticleBoss;

ParticleBoss	*PB_Create(GraphicsDevice *pGD,
	const StuffKeeper *pSK, CBKeeper *pCBK);

void	PB_Destroy(ParticleBoss **ppPB);

//returns particle ID
uint32_t	PB_CreateEmitter(ParticleBoss *pPB, const StuffKeeper *pSK,
	const char *pTexName, uint8_t shape, float startSize, vec4 startColor,
	float emitSec, float shapeSize, vec3 lineAxis, int maxParticles,
	float rotVelMin, float rotVelMax, vec4 colVelMin, vec4 colVelMax,
	float sizeVelMin, float sizeVelMax, float velMin, float velMax,
	float lifeMin, float lifeMax, vec3 pos);

void	PB_EmitterActivate(ParticleBoss *pPB, uint32_t emitterID, bool bOn);
void	PB_SetEmitterPosition(ParticleBoss *pPB, uint32_t emitterID, vec3 pos);

void	PB_UpdateAndDraw(ParticleBoss *pPB, float secDelta);