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
#include	"UtilityLib/GraphicsDevice.h"
#include	"UtilityLib/GameCamera.h"
#include	"UtilityLib/UpdateTimer.h"
#include	"UtilityLib/FileStuff.h"
#include	"InputLib/Input.h"


#define	RESX				1280
#define	RESY				720
#define	UVSCALE_RATE		1.0f
#define	MAX_UI_VERTS		(8192)


//input context, stuff input handlers will need
typedef struct	TestStuff_t
{
	GraphicsDevice	*mpGD;
	GameCamera		*mpCam;
	UIStuff			*mpUI;

	//toggles
	bool	mbLeftMouseDown;
	bool	mbRunning;
	
	//clay pointer stuff
	Clay_Vector2	mScrollDelta;
	Clay_Vector2	mMousePos;

	//misc data
	vec3	mLightDir;
	vec3	mEyePos;

}	TestStuff;

//static forward decs
static void	sSetupKeyBinds(Input *pInp);
static void	sSetupRastVP(GraphicsDevice *pGD);
static void sFindFonts(const TestStuff *pTS);

//clay stuff
static const Clay_RenderCommandArray sCreateLayout(const TestStuff *pTS);
static void sHandleClayErrors(Clay_ErrorData errorData);

//input event handlers
static void	sLeftMouseDownEH(void *pContext, const SDL_Event *pEvt);
static void	sLeftMouseUpEH(void *pContext, const SDL_Event *pEvt);
static void	sRightMouseDownEH(void *pContext, const SDL_Event *pEvt);
static void	sRightMouseUpEH(void *pContext, const SDL_Event *pEvt);
static void sMouseMoveEH(void *pContext, const SDL_Event *pEvt);
static void sEscEH(void *pContext, const SDL_Event *pEvt);


int main(void)
{
	printf("Test UI!\n");

	//store a bunch of vars in a struct
	//for ref/modifying by input handlers
	TestStuff	*pTS	=malloc(sizeof(TestStuff));
	memset(pTS, 0, sizeof(TestStuff));

	sFindFonts(pTS);

	//input and key / mouse bindings
	Input	*pInp	=INP_CreateInput();
	sSetupKeyBinds(pInp);

	bool	bGDInit	=GD_Init(&pTS->mpGD, "UI Test!",
				0, 0, RESX, RESY, true, D3D_FEATURE_LEVEL_11_1);
	if(!bGDInit)
	{
		printf("Graphics init failed!\n");
		return	EXIT_FAILURE;
	}

	//turn on border
	GD_SetWindowBordered(pTS->mpGD, true);

	sSetupRastVP(pTS->mpGD);

	StuffKeeper	*pSK	=StuffKeeper_Create(pTS->mpGD);
	if(pSK == NULL)
	{
		printf("Couldn't create StuffKeeper!\n");
		GD_Destroy(&pTS->mpGD);
		return	EXIT_FAILURE;
	}

	CBKeeper	*pCBK	=CBK_Create(pTS->mpGD);
	PostProcess	*pPP	=PP_Create(pTS->mpGD, pSK, pCBK);

	//set sky gradient
	{
		vec3	skyHorizon	={	0.0f, 0.5f, 1.0f	};
		vec3	skyHigh		={	0.0f, 0.25f, 1.0f	};

		CBK_SetSky(pCBK, skyHorizon, skyHigh);
		CBK_SetFogVars(pCBK, 50.0f, 300.0f, true);
	}

	PP_SetTargets(pPP, pTS->mpGD, "BackColor", "BackDepth");

	float	aspect	=(float)RESX / (float)RESY;
	
	//these need align
	__attribute((aligned(32)))	mat4	camProj, textProj, viewMat;

	pTS->mEyePos[0] =185.0f;
	pTS->mEyePos[1] =36.0f;
	pTS->mEyePos[2] =180.0f;

	//game camera
	pTS->mpCam	=GameCam_Create(false, 0.1f, 2000.0f, GLM_PI_4f, aspect, 1.0f, 10.0f);

	//2d projection for text
	glm_ortho(0, RESX, RESY, 0, -1.0f, 1.0f, textProj);

	//set constant buffers to shaders, think I just have to do this once
	CBK_SetCommonCBToShaders(pCBK, pTS->mpGD);

	pTS->mpUI	=UI_Create(pTS->mpGD, pSK, MAX_UI_VERTS);

	UI_AddFont(pTS->mpUI, "MeiryoUI26", 0);

	//clay init
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
    Clay_Initialize(clayMemory, (Clay_Dimensions) { (float)RESX, (float)RESY }, (Clay_ErrorHandler) { sHandleClayErrors });
    Clay_SetMeasureTextFunction(UI_MeasureText, pTS->mpUI);

	Clay_SetDebugModeEnabled(false);


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

		//set no blend, I think post processing turns it on maybe
		GD_OMSetBlendState(pTS->mpGD, StuffKeeper_GetBlendState(pSK, "NoBlending"));
		GD_PSSetSampler(pTS->mpGD, StuffKeeper_GetSamplerState(pSK, "PointWrap"), 0);

		//set CB view
		{
			GameCam_GetViewMatrixFly(pTS->mpCam, viewMat, pTS->mEyePos);
			CBK_SetView(pCBK, viewMat, pTS->mEyePos);

			//set the skybox world mat to match eye pos
			glm_translate_make(viewMat, pTS->mEyePos);
		}

		PP_ClearDepth(pPP, pTS->mpGD, "BackDepth");
		PP_ClearTarget(pPP, pTS->mpGD, "BackColor");

		//update frame CB
		CBK_UpdateFrame(pCBK, pTS->mpGD);

		//turn depth off for sky
		GD_OMSetDepthStencilState(pTS->mpGD, StuffKeeper_GetDepthStencilState(pSK, "DisableDepth"));

		//turn depth back on
		GD_OMSetDepthStencilState(pTS->mpGD, StuffKeeper_GetDepthStencilState(pSK, "EnableDepth"));

		GD_PSSetSampler(pTS->mpGD, StuffKeeper_GetSamplerState(pSK, "PointClamp"), 0);

		//set proj for 2D
		CBK_SetProjection(pCBK, textProj);
		CBK_UpdateFrame(pCBK, pTS->mpGD);

		Clay_UpdateScrollContainers(true, pTS->mScrollDelta, dt);

		pTS->mScrollDelta.x	=pTS->mScrollDelta.y	=0.0f;
	
		Clay_RenderCommandArray renderCommands = sCreateLayout(pTS);
	
		UI_BeginDraw(pTS->mpUI);
	
		UI_ClayRender(pTS->mpUI, renderCommands);
	
		UI_EndDraw(pTS->mpUI);
	
		//change back to 3D
		CBK_SetProjection(pCBK, camProj);
		CBK_UpdateFrame(pCBK, pTS->mpGD);

		GD_Present(pTS->mpGD);
	}

	GD_Destroy(&pTS->mpGD);

	return	EXIT_SUCCESS;
}


//event handlers (eh)
static void	sLeftMouseDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbLeftMouseDown	=true;

	Clay_SetPointerState(pTS->mMousePos, true);
}

static void	sLeftMouseUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbLeftMouseDown	=false;

	Clay_SetPointerState(pTS->mMousePos, false);
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

	pTS->mMousePos.x	=pEvt->motion.x;
	pTS->mMousePos.y	=pEvt->motion.y;

	Clay_SetPointerState(pTS->mMousePos, pTS->mbLeftMouseDown);
}

static void	sEscEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbRunning	=false;
}


static void sFindFonts(const TestStuff *pTS)
{
	DIR	*pDir	=opendir("Fonts");
	for(;;)
	{
		struct dirent	*pDE	=readdir(pDir);
		if(pDE == NULL)
		{
			break;
		}

		struct stat	fileStuff;
		int	res	=stat(pDE->d_name, &fileStuff);
		if(res)
		{
			FileStuff_PrintErrno(res);
			continue;
		}

		//regular file?
		if(S_ISREG(fileStuff.st_mode))
		{
			printf("Font: %s\n", pDE->d_name);
		}
	}
	closedir(pDir);
}

static void	sSetupKeyBinds(Input *pInp)
{
	//event style bindings
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_ESCAPE, sEscEH);

	//held bindings
	//move data events
	INP_MakeBinding(pInp, INP_BIND_TYPE_MOVE, SDL_EVENT_MOUSE_MOTION, sMouseMoveEH);

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


//some ideas for stuff to display
//files in the game dir
//time related stuff
//some kind of fake rpg gump with random stats
static Clay_RenderCommandArray	sCreateLayout(const TestStuff *pTS)
{
	Clay_BeginLayout();

//	sprintf(sVelString, "Velocity: %f, %f, %f", velocity[0], velocity[1], velocity[2]);

//	Clay_String	velInfo;

//	velInfo.chars	=sVelString;
//	velInfo.length	=strlen(sVelString);

	CLAY(CLAY_ID("OuterContainer"), { .layout =
		{
			.layoutDirection = CLAY_TOP_TO_BOTTOM,
			.sizing =
			{
				.width = CLAY_SIZING_GROW(0),
				.height = CLAY_SIZING_GROW(0)
			},
			.padding = { 8, 8, 8, 8 },
			.childGap = 8
		}})
	{
//		CLAY_TEXT(velInfo, CLAY_TEXT_CONFIG({ .fontSize = 26, .textColor = {0, 70, 70, 155} }));

//		sCheckLOS(pTS);
	}

	return	Clay_EndLayout();
}

static bool reinitializeClay = false;

static void sHandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
    if (errorData.errorType == CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED) {
        reinitializeClay = true;
        Clay_SetMaxElementCount(Clay_GetMaxElementCount() * 2);
    } else if (errorData.errorType == CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED) {
        reinitializeClay = true;
        Clay_SetMaxMeasureTextCacheWordCount(Clay_GetMaxMeasureTextCacheWordCount() * 2);
    }
}