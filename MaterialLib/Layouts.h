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


//create vertex formats for later matching to layouts
extern void	Layouts_MakeElems(DictSZ **ppElems, DictSZ **ppLaySizes);

//returns the usual grog sizes (I squish into 16 bit floats alot)
extern void	Layouts_GetGrogSizes(const int elems[], int sizes[], int elCount);

extern void	Layouts_MakeLayouts(GraphicsDevice *pGD,
	const DictSZ *pVSCode, DictSZ **ppLayouts);

extern ID3D11InputLayout	*Layouts_FindMatch(const DictSZ *pLays,
	const DictSZ *pLayCounts, const DictSZ *pElems,
	int elems[], int elCount);