#pragma once
#include	<stdint.h>
#include	<utstring.h>
#include	<cglm/call.h>

typedef struct	UserSettings_t	UserSettings;

UserSettings	*UserSettings_Create(void);

void	UserSettings_Load(UserSettings *pUS);
void	UserSettings_Save(const UserSettings *pUS);
void	UserSettings_AddPosition(UserSettings *pUS, const char *pKey, float x, float y);
void	UserSettings_GetPosition(const UserSettings *pUS, const char *pKey, vec2 pos);