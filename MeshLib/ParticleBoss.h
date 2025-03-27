#pragma once
#include	<cglm/call.h>

#define	EMIT_SHAPE_POINT	0
#define	EMIT_SHAPE_SPHERE	1
#define	EMIT_SHAPE_BOX		2
#define	EMIT_SHAPE_LINE		3
#define	EMIT_SHAPE_PLANE	4


//particle behaviour
//velocities in units per second
typedef struct	EmitterValues_t
{
	float	mRotationalVelocityMin, mRotationalVelocityMax;
	float	mVelocityMin, mVelocityMax;
	float	mSizeVelocityMin, mSizeVelocityMax;
	vec4	mColorVelocityMin, mColorVelocityMax;
	int		mLifeMin, mLifeMax;
	float	mVelocityCap;	//a hard cap on velocity due to gravity etc

}	EmitterValues;



typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	ParticleEmitter_t	ParticleEmitter;


ParticleEmitter	*PE_Create(GraphicsDevice *pGD, StuffKeeper *pSK);