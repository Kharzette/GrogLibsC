#pragma once
#include	<stdint.h>

typedef struct	UpdateTimer_t	UpdateTimer;

UpdateTimer	*UpdateTimer_Create(bool bFixed, bool bSpendRemainder);
uint64_t UpdateTimer_GetRenderUpdateDeltaTics(UpdateTimer *pUT);
float UpdateTimer_GetRenderUpdateDeltaSeconds(UpdateTimer *pUT);
float UpdateTimer_GetRenderUpdateDeltaMilliSeconds(UpdateTimer *pUT);
uint64_t UpdateTimer_GetUpdateDeltaTics(UpdateTimer *pUT);
float UpdateTimer_GetUpdateDeltaSeconds(UpdateTimer *pUT);

float UpdateTimer_GetUpdateDeltaMilliSeconds(UpdateTimer *pUT);
void UpdateTimer_UpdateDone(UpdateTimer *pUT);

//allows a user preference on the maximum
//deltatime allowed (for game stability)
void UpdateTimer_SetMaxDeltaTics(UpdateTimer *pUT, uint64_t tics);
void UpdateTimer_SetMaxDeltaMilliSeconds(UpdateTimer *pUT, float milliSeconds);
void UpdateTimer_SetMaxDeltaSeconds(UpdateTimer *pUT, float seconds);
void UpdateTimer_SetFixedTimeStepTics(UpdateTimer *pUT, long tics);
void UpdateTimer_SetFixedTimeStepSeconds(UpdateTimer *pUT, float seconds);
void UpdateTimer_SetFixedTimeStepMilliSeconds(UpdateTimer *pUT, float milliSeconds);
void UpdateTimer_Stamp(UpdateTimer *pUT);
uint64_t UpdateTimer_DeltaTics(UpdateTimer *pUT);
//deltas are reasonably safe in floats
float UpdateTimer_DeltaSeconds(UpdateTimer *pUT);
//deltas are reasonably safe in floats
float UpdateTimer_DeltaMilliSeconds(UpdateTimer *pUT);
