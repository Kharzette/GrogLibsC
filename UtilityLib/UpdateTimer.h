#pragma once
#include	<stdint.h>

typedef struct	UpdateTimer_t	UpdateTimer;

UpdateTimer	*UpdateTimer_Create(bool bFixed, bool bSpendRemainder);

//top and bottom of update loop
void UpdateTimer_Stamp(UpdateTimer *pUT);
void UpdateTimer_UpdateDone(UpdateTimer *pUT);

//return a time slice
uint64_t UpdateTimer_GetUpdateDeltaTics(const UpdateTimer *pUT);
float UpdateTimer_GetUpdateDeltaSeconds(const UpdateTimer *pUT);
float UpdateTimer_GetUpdateDeltaMilliSeconds(const UpdateTimer *pUT);

//return render time slice (always the entire deltaTime)
uint64_t UpdateTimer_GetRenderUpdateDeltaTics(const UpdateTimer *pUT);
float UpdateTimer_GetRenderUpdateDeltaSeconds(const UpdateTimer *pUT);
float UpdateTimer_GetRenderUpdateDeltaMilliSeconds(const UpdateTimer *pUT);

//Set fixed timeslice and max deltatime
void UpdateTimer_SetMaxDeltaTics(UpdateTimer *pUT, uint64_t tics);
void UpdateTimer_SetMaxDeltaMilliSeconds(UpdateTimer *pUT, float milliSeconds);
void UpdateTimer_SetMaxDeltaSeconds(UpdateTimer *pUT, float seconds);
void UpdateTimer_SetFixedTimeStepTics(UpdateTimer *pUT, long tics);
void UpdateTimer_SetFixedTimeStepSeconds(UpdateTimer *pUT, float seconds);
void UpdateTimer_SetFixedTimeStepMilliSeconds(UpdateTimer *pUT, float milliSeconds);