#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<x86intrin.h>
#include	<string.h>
#include	<assert.h>
#include	<SDL3/SDL.h>
#include	<SDL3/SDL_keycode.h>
#include	"utlist.h"
#include	"Input.h"

//call event callback with either update context or stored in bind
#define	EVENTCB(pIB,pEvt,pContext)	if(pIB->mpContext != NULL) pIB->EventCB(pIB->mpContext, pEvt); else pIB->EventCB(pContext, pEvt);

//binding types...
//Hold down for a constant effect per frame
//	Stuff like key turn
//Button event for a function or toggle (down+up)
//	Stuff like hit m for a map popup or L to toggle flashlight on/off
//Movement like mouse or stick movement
//specific button down/up events
typedef struct	InputBinding_t
{
	void	*mpContext;
	int		mBindType;
	bool	mbActive;	//for hold type, held last check

	uint32_t	mCode;	//key or mouse button or whatever

	void	(*EventCB)(void *pContext, const SDL_Event *pEvt);

	struct	InputBinding_t	*next;	//for utlist
}	InputBinding;

typedef struct	Input_t
{
	InputBinding	*mpBindings;
}	Input;


Input	*INP_CreateInput(void)
{
	Input	*pRet	=malloc(sizeof(Input));

	memset(pRet, 0, sizeof(Input));

	return	pRet;
}


//for a context related to the binding
void	INP_MakeBindingCTX(Input *pInp, int bindType, uint32_t code, InputCB cb, void *pContext)
{
	assert(bindType >= INP_BIND_TYPE_HELD
		&& bindType <= INP_BIND_TYPE_RELEASE);

	InputBinding	*pBind	=malloc(sizeof(InputBinding));
	memset(pBind, 0, sizeof(InputBinding));

	pBind->mBindType	=bindType;
	pBind->mCode		=code;
	pBind->EventCB		=cb;
	pBind->mpContext	=pContext;

	LL_APPEND(pInp->mpBindings, pBind);
}

//will use context from update function
void	INP_MakeBinding(Input *pInp, int bindType, uint32_t code, InputCB cb)
{
	INP_MakeBindingCTX(pInp, bindType, code, cb, NULL);
}


void	INP_Update(Input *pInp, void *pContext)
{
	//process sdl input events
	SDL_Event	evt;
	while(SDL_PollEvent(&evt))
	{
		if(evt.type == SDL_EVENT_KEY_UP)
		{
			InputBinding	*pBind;
			LL_FOREACH(pInp->mpBindings, pBind)
			{
				if(pBind->mBindType == INP_BIND_TYPE_EVENT)
				{
					if(pBind->mCode == evt.key.key)
					{
						EVENTCB(pBind, &evt, pContext);
					}
				}
				else if(pBind->mBindType == INP_BIND_TYPE_HELD)
				{
					if(pBind->mCode == evt.key.key)
					{
						pBind->mbActive	=false;
					}
				}
				else if(pBind->mBindType == INP_BIND_TYPE_RELEASE)
				{
					if(pBind->mCode == evt.key.key)
					{
						EVENTCB(pBind, &evt, pContext);
					}
				}
			}
		}
		else if(evt.type == SDL_EVENT_KEY_DOWN)
		{
			InputBinding	*pBind;
			LL_FOREACH(pInp->mpBindings, pBind)
			{
				if(pBind->mBindType == INP_BIND_TYPE_EVENT)
				{
					continue;
				}
				else if(pBind->mBindType == INP_BIND_TYPE_HELD)
				{
					if(pBind->mCode == evt.key.key)
					{
						pBind->mbActive	=true;
					}
				}
				else if(pBind->mBindType == INP_BIND_TYPE_PRESS)
				{
					if(pBind->mCode == evt.key.key)
					{
						EVENTCB(pBind, &evt, pContext);
					}
				}
			}
		}
		else if(evt.type == SDL_EVENT_MOUSE_BUTTON_UP)
		{
			InputBinding	*pBind;
			LL_FOREACH(pInp->mpBindings, pBind)
			{
				if(pBind->mBindType == INP_BIND_TYPE_EVENT)
				{
					if(pBind->mCode == evt.button.button)
					{
						EVENTCB(pBind, &evt, pContext);
					}
				}
				else if(pBind->mBindType == INP_BIND_TYPE_HELD)
				{
					if(pBind->mCode == evt.button.button)
					{
						pBind->mbActive	=false;
					}
				}
				else if(pBind->mBindType == INP_BIND_TYPE_RELEASE)
				{
					if(pBind->mCode == evt.button.button)
					{
						EVENTCB(pBind, &evt, pContext);
					}
				}
			}
		}
		else if(evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
		{
			InputBinding	*pBind;
			LL_FOREACH(pInp->mpBindings, pBind)
			{
				if(pBind->mBindType == INP_BIND_TYPE_EVENT)
				{
					if(pBind->mCode == evt.button.button)
					{
						continue;
					}
				}
				else if(pBind->mBindType == INP_BIND_TYPE_HELD)
				{
					if(pBind->mCode == evt.button.button)
					{
						pBind->mbActive	=true;
					}
				}
				else if(pBind->mBindType == INP_BIND_TYPE_PRESS)
				{
					if(pBind->mCode == evt.button.button)
					{
						EVENTCB(pBind, &evt, pContext);
					}
				}
			}
		}
		else if(evt.type == SDL_EVENT_MOUSE_MOTION || evt.type == SDL_EVENT_MOUSE_WHEEL)
		{
			InputBinding	*pBind;
			LL_FOREACH(pInp->mpBindings, pBind)
			{
				if(pBind->mBindType != INP_BIND_TYPE_MOVE)
				{
					continue;
				}

				if(pBind->mCode == evt.type)
				{
					EVENTCB(pBind, &evt, pContext);
				}
			}
		}
	}

	//now fire events for held binds still active
	InputBinding	*pBind;
	LL_FOREACH(pInp->mpBindings, pBind)
	{
		if(pBind->mBindType != INP_BIND_TYPE_HELD)
		{
			continue;
		}

		if(pBind->mbActive)
		{
			EVENTCB(pBind, NULL, pContext);
		}
	}
}
