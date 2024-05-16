#pragma once
#include	<stdint.h>
#include	<cglm/call.h>

typedef struct	MaterialLib_t		MaterialLib;
typedef struct	StuffKeeper_t		StuffKeeper;

MaterialLib	*MatLib_Read(const char *pFileName, const StuffKeeper *pSK);
