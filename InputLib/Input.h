#pragma once
#include	<stdint.h>
#include	<stdbool.h>
#include	<SDL2/SDL.h>

#define	INP_BIND_TYPE_HELD		1
#define	INP_BIND_TYPE_EVENT		2
#define	INP_BIND_TYPE_MOVE		3
#define	INP_BIND_TYPE_PRESS		4
#define	INP_BIND_TYPE_RELEASE	5


typedef struct	Input_t	Input;

//input handler callback
typedef void (*InputCB)(void *pContext, const SDL_Event *pEvt);


extern Input	*INP_CreateInput(void);

extern void	INP_MakeBinding(Input *pInp, int bindType, uint32_t code, InputCB cb);

extern void	INP_Update(Input *pInp, void *pContext);
