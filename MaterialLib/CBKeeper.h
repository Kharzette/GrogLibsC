#pragma once

typedef struct	CBKeeper_t			CBKeeper;
typedef struct	GraphicsDevice_t	GraphicsDevice;

extern CBKeeper	*CBK_Create(GraphicsDevice *pGD);

//assign buffers to shaders
void	CBK_SetCommonCBToShaders(CBKeeper *pCBK, GraphicsDevice *pGD);
void	CBK_Set2DCBToShaders(CBKeeper *pCBK, GraphicsDevice *pGD);
void	CBK_SetCharacterToShaders(CBKeeper *pCBK, GraphicsDevice *pGD);
void	CBK_SetBSPToShaders(CBKeeper *pCBK, GraphicsDevice *pGD);
void	CBK_SetPostToShaders(CBKeeper *pCBK, GraphicsDevice *pGD);
void	CBK_SetPerShadowToShaders(CBKeeper *pCBK, GraphicsDevice *pGD);
void	CBK_SetTextModeToShaders(CBKeeper *pCBK, GraphicsDevice *pGD);

//push changed cpu structs onto gpu
void	CBK_UpdateFrame(CBKeeper *pCBK, GraphicsDevice *pGD);
void	CBK_UpdateObject(CBKeeper *pCBK, GraphicsDevice *pGD);
void	CBK_UpdateTwoD(CBKeeper *pCBK, GraphicsDevice *pGD);
void	CBK_UpdateCharacter(CBKeeper *pCBK, GraphicsDevice *pGD);
void	CBK_UpdateBSP(CBKeeper *pCBK, GraphicsDevice *pGD);
void	CBK_UpdatePost(CBKeeper *pCBK, GraphicsDevice *pGD);
void	CBK_UpdatePerShadow(CBKeeper *pCBK, GraphicsDevice *pGD);
void	CBK_UpdateTextMode(CBKeeper *pCBK, GraphicsDevice *pGD);

//set values in the CPU structs
//perframe stuff
void CBK_SetView(CBKeeper *pCBK, const mat4 view, const vec3 eyePos);
void CBK_SetTransposedView(CBKeeper *pCBK, const mat4 view, const vec3 eyePos);
void CBK_SetTransposedLightViewProj(CBKeeper *pCBK, const mat4 lvp);
void CBK_SetLightViewProj(CBKeeper *pCBK, const mat4 lvp);
void CBK_SetTransposedProjection(CBKeeper *pCBK, const mat4 proj);
void CBK_SetProjection(CBKeeper *pCBK, const mat4 proj);

//per object stuff
void CBK_SetTrilights(CBKeeper *pCBK, const vec4 L0, const vec4 L1, const vec4 L2, const vec3 lightDir);
void CBK_SetTrilights3(CBKeeper *pCBK, const vec3 L0, const vec3 L1, const vec3 L2, const vec3 lightDir);
void CBK_SetSolidColour(CBKeeper *pCBK, const vec4 sc);
void CBK_SetSpecular(CBKeeper *pCBK, const vec4 specColour, float specPow);
void CBK_SetSpecularPower(CBKeeper *pCBK, float specPow);
void CBK_SetWorldMat(CBKeeper *pCBK, const mat4 world);
void CBK_SetTransposedWorldMat(CBKeeper *pCBK, const mat4 world);
void CBK_SetMaterialID(CBKeeper *pCBK, int matID);
void CBK_SetDanglyForce(CBKeeper *pCBK, const vec3 force);

//2d stuff
void CBK_SetTextTransform(CBKeeper *pCBK, const vec2 textPos, const vec2 textScale);
void CBK_SetSecondLayerOffset(CBKeeper *pCBK, const vec2 ofs);
void CBK_SetTextColor(CBKeeper *pCBK, const vec4 col);

//BSP Stuff
void CBK_SetTextureEnabled(CBKeeper *pCBK, bool bOn);
void CBK_SetTexSize(CBKeeper *pCBK, const vec2 size);
void CBK_SetAniIntensities(CBKeeper *pCBK, const float *pAni);

//character bones
void CBK_SetBones(CBKeeper *pCBK, const mat4 *pBones);
void CBK_SetBonesWithTranspose(CBKeeper *pCBK, const mat4 *pBones);

//postprocessing
void CBK_SetInvViewPort(CBKeeper *pCBK, const vec2 port);
void CBK_SetOutlinerVars(CBKeeper *pCBK, const vec2 size, float texelSteps, float threshold);
void CBK_SetBilateralBlurVars(CBKeeper *pCBK, float fallOff, float sharpness, float opacity);
void CBK_SetBloomVars(CBKeeper *pCBK, float thresh, float intensity,
	float sat, float baseIntensity, float baseSat);
void CBK_SetWeightsOffsets(CBKeeper *pCBK,
	const float *pWX, const float *pWY,
	const float *pOffx, const float *pOffy);

//shadows
void CBK_SetPerShadow(CBKeeper *pCBK, const vec3 shadowLightPos, bool bDirectional, float shadowAtten);
void CBK_SetPerShadowDirectional(CBKeeper *pCBK, bool bDirectional);
void CBK_SetPerShadowLightPos(CBKeeper *pCBK, const vec3 pos);

//Text Mode
void CBK_SetTextModeScreenSize(CBKeeper *pCBK, uint32_t width, uint32_t height, uint32_t cwidth, uint32_t cheight);
void CBK_SetTextModeFontInfo(CBKeeper *pCBK, uint32_t startChar, uint32_t numColumns, uint32_t charWidth, uint32_t charHeight);