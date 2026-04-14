#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<assert.h>
#include	<dirent.h>
#include	<sys/stat.h>
#include	<errno.h>
#include	<cglm/call.h>
#include	"MaterialLib/StuffKeeper.h"
#include	"MaterialLib/CBKeeper.h"
#include	"MaterialLib/PostProcess.h"
#define	CLAY_IMPLEMENTATION
#include	"UILib/UIStuff.h"
#include	"UILib/Screen.h"
#include	"UtilityLib/GraphicsDevice.h"
#include	"UtilityLib/GameCamera.h"
#include	"UtilityLib/UpdateTimer.h"
#include	"InputLib/Input.h"
#include	"AudioLib/Audio.h"
#include	"AudioLib/SoundEffect.h"
#include	"UtilityLib/ListStuff.h"	//stringlist stuff


#define	RESX				1280
#define	RESY				720
#define	SCREENWIDTH			40		//the crt width
#define	SCREENHEIGHT		30		//crt h
#define	UVSCALE_RATE		1.0f


//input context, stuff input handlers will need
typedef struct	TestStuff_t
{
	GraphicsDevice	*mpGD;
	CBKeeper		*mpCBK;
	Screen			*mpScr;

	//toggles
	bool	mbLeftMouseDown;
	bool	mbRunning;
	
	//misc data
	vec3	mLightDir;
	vec3	mEyePos;
	vec2	mMousePos;

	//screen data
	uint8_t	*mpScreenData;

}	TestStuff;


//static forward decs
static void	sSetupKeyBinds(Input *pInp);
static void	sSetupRastVP(GraphicsDevice *pGD);

//input event handlers
static void	sLeftMouseDownEH(void *pContext, const SDL_Event *pEvt);
static void	sLeftMouseUpEH(void *pContext, const SDL_Event *pEvt);
static void	sRightMouseDownEH(void *pContext, const SDL_Event *pEvt);
static void	sRightMouseUpEH(void *pContext, const SDL_Event *pEvt);
static void sMouseMoveEH(void *pContext, const SDL_Event *pEvt);
static void MouseWheelEH(void *pContext, const SDL_Event *pEvt);
static void sEscEH(void *pContext, const SDL_Event *pEvt);
static void sTabEH(void *pContext, const SDL_Event *pEvt);
static void sREH(void *pContext, const SDL_Event *pEvt);


int main(void)
{
	printf("Test Text Mode!\n");

	//store a bunch of vars in a struct
	//for ref/modifying by input handlers
	TestStuff	*pTS	=malloc(sizeof(TestStuff));
	memset(pTS, 0, sizeof(TestStuff));

	//screen data
	pTS->mpScreenData	=malloc(SCREENWIDTH * SCREENHEIGHT);

	//input and key / mouse bindings
	Input	*pInp	=INP_CreateInput();
	sSetupKeyBinds(pInp);

	bool	bGDInit	=GD_Init(&pTS->mpGD, "Text Mode Test!",
				0, 0, RESX, RESY, true, D3D_FEATURE_LEVEL_11_1);
	if(!bGDInit)
	{
		printf("Graphics init failed!\n");
		return	EXIT_FAILURE;
	}

	//turn on border
	GD_SetWindowBordered(pTS->mpGD, true);

	sSetupRastVP(pTS->mpGD);

	Audio	*pAud	=Audio_Create(0);

	StuffKeeper	*pSK	=StuffKeeper_Create(pTS->mpGD);
	if(pSK == NULL)
	{
		printf("Couldn't create StuffKeeper!\n");
		GD_Destroy(&pTS->mpGD);
		return	EXIT_FAILURE;
	}

	pTS->mpCBK			=CBK_Create(pTS->mpGD);
	PostProcess	*pPP	=PP_Create(pTS->mpGD, pSK, pTS->mpCBK);

	PP_SetTargets(pPP, pTS->mpGD, "BackColor", "BackDepth");

	//these need align
	__attribute((aligned(32)))	mat4	textProj;

	//2d projection for text
	glm_ortho(0, RESX, RESY, 0, -1.0f, 1.0f, textProj);

	//set constant buffers to shaders, think I just have to do this once
	CBK_SetCommonCBToShaders(pTS->mpCBK, pTS->mpGD);

	GD_OMSetBlendState(pTS->mpGD, StuffKeeper_GetBlendState(pSK, "NoBlending"));
	GD_OMSetDepthStencilState(pTS->mpGD, StuffKeeper_GetDepthStencilState(pSK, "EnableDepth"));
	GD_PSSetSampler(pTS->mpGD, StuffKeeper_GetSamplerState(pSK, "PointClamp"), 0);

	//set proj for 2D
	CBK_SetProjection(pTS->mpCBK, textProj);
	CBK_UpdateFrame(pTS->mpCBK, pTS->mpGD);

	GrogFont	*pCGAFont	=StuffKeeper_GetFont(pSK, "CGA");

	pTS->mpScr	=Screen_Create(pTS->mpGD, pSK, pTS->mpCBK, pCGAFont, RESX, RESY, SCREENWIDTH, SCREENHEIGHT);

	UpdateTimer	*pUT	=UpdateTimer_Create(true, false);
//	UpdateTimer_SetFixedTimeStepMilliSeconds(pUT, 6.944444f);	//144hz
	UpdateTimer_SetFixedTimeStepMilliSeconds(pUT, 16.66666f);	//60hz

	float	maxDT	=0.0f;

	pTS->mbRunning	=true;
	while(pTS->mbRunning)
	{
		UpdateTimer_Stamp(pUT);
		for(float secDelta =UpdateTimer_GetUpdateDeltaSeconds(pUT);
			secDelta > 0.0f;
			secDelta =UpdateTimer_GetUpdateDeltaSeconds(pUT))
		{
			//do input here
			//move turn etc
			INP_Update(pInp, pTS);

			UpdateTimer_UpdateDone(pUT);
		}

		//render update
		float	dt	=UpdateTimer_GetRenderUpdateDeltaSeconds(pUT);
		{
			if(dt > maxDT)
			{
				maxDT	=dt;
			}
		}

		PP_ClearDepth(pPP, pTS->mpGD, "BackDepth");
		PP_ClearTarget(pPP, pTS->mpGD, "BackColor");

		//update frame CB
		CBK_UpdateFrame(pTS->mpCBK, pTS->mpGD);

		Screen_Draw(pTS->mpScr, pTS->mpGD);
	
		GD_Present(pTS->mpGD);
	}

	Audio_Destroy(&pAud);

	GD_Destroy(&pTS->mpGD);

	return	EXIT_SUCCESS;
}


//event handlers (eh)
static void	sLeftMouseDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbLeftMouseDown	=true;
}

static void	sLeftMouseUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbLeftMouseDown	=false;
}

static void	sRightMouseDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);
}

static void	sRightMouseUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);
}

static void	sMouseMoveEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mMousePos[0]	=pEvt->motion.x;
	pTS->mMousePos[1]	=pEvt->motion.y;
}

static void	MouseWheelEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);
}

static void	sEscEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbRunning	=false;
}

static void	sTabEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);
}

static void	sREH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	//randomize screen contents
	for(int y=0;y < SCREENHEIGHT;y++)
	{
		for(int x=0;x < SCREENWIDTH;x++)
		{
			pTS->mpScreenData[(y * SCREENWIDTH) + x]	=SDL_rand(256);
		}
	}

	Screen_SetContents(pTS->mpScr, pTS->mpGD, pTS->mpCBK, pTS->mpScreenData);
}


static void	sSetupKeyBinds(Input *pInp)
{
	//event style bindings
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_ESCAPE, sEscEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_TAB, sTabEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_R, sREH);

	//held bindings
	//move data events
	INP_MakeBinding(pInp, INP_BIND_TYPE_MOVE, SDL_EVENT_MOUSE_MOTION, sMouseMoveEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_MOVE, SDL_EVENT_MOUSE_WHEEL, MouseWheelEH);

	//down/up events
	INP_MakeBinding(pInp, INP_BIND_TYPE_PRESS, SDL_BUTTON_RIGHT, sRightMouseDownEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_RELEASE, SDL_BUTTON_RIGHT, sRightMouseUpEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_PRESS, SDL_BUTTON_LEFT, sLeftMouseDownEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_RELEASE, SDL_BUTTON_LEFT, sLeftMouseUpEH);
}

static void	sSetupRastVP(GraphicsDevice *pGD)
{
	D3D11_RASTERIZER_DESC	rastDesc;
	rastDesc.AntialiasedLineEnable	=false;
	rastDesc.CullMode				=D3D11_CULL_BACK;
	rastDesc.FillMode				=D3D11_FILL_SOLID;
	rastDesc.FrontCounterClockwise	=true;
	rastDesc.MultisampleEnable		=false;
	rastDesc.DepthBias				=0;
	rastDesc.DepthBiasClamp			=0;
	rastDesc.DepthClipEnable		=true;
	rastDesc.ScissorEnable			=false;
	rastDesc.SlopeScaledDepthBias	=0;
	ID3D11RasterizerState	*pRast	=GD_CreateRasterizerState(pGD, &rastDesc);

	D3D11_VIEWPORT	vp;

	vp.Width	=RESX;
	vp.Height	=RESY;
	vp.MaxDepth	=1.0f;
	vp.MinDepth	=0.0f;
	vp.TopLeftX	=0;
	vp.TopLeftY	=0;

	GD_RSSetViewPort(pGD, &vp);
	GD_RSSetState(pGD, pRast);
	GD_IASetPrimitiveTopology(pGD, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}