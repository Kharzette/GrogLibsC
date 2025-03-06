#pragma once
#include	"d3d11.h"

#define	EL_POSITION		0
#define	EL_NORMAL		1
#define	EL_TEXCOORD		2
#define	EL_COLOR		3
#define	EL_BINDICES		4
#define	EL_BWEIGHTS		5
#define	EL_TANGENT		6
#define	EL_TEXCOORD4	7
#define	EL_POSITION2	8
#define	EL_POSITION4	9

//forward decs
typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef	struct	DictSZ_t			DictSZ;


extern void	MakeDescs(DictSZ **ppDescs);
extern void	MakeLayouts(GraphicsDevice *pGD, const DictSZ **ppDescs,
	const DictSZ *pVSCode, DictSZ **ppLayouts, DictSZ **ppLaySizes);

extern ID3D11InputLayout	*FindMatch(const DictSZ **ppLays,
	const DictSZ **ppLayCounts, const DictSZ **ppDescs,
	int elems[], int elCount);