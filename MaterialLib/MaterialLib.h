#pragma once
#include	<stdint.h>
#include	<cglm/call.h>

typedef struct	MaterialLib_t	MaterialLib;
typedef struct	StuffKeeper_t	StuffKeeper;
typedef struct	StringList_t	StringList;

MaterialLib			*MatLib_Create(StuffKeeper *pSK);
MaterialLib			*MatLib_Read(const char *pFileName, StuffKeeper *pSK);
void				MatLib_Add(MaterialLib *pML, const char *szName, Material *pMat);
int					MatLib_GetNumMats(const MaterialLib *pML);
const StringList	*MatLib_GetMatList(const MaterialLib *pML);
Material			*MatLib_GetMaterial(MaterialLib *pML, const char *szMat);
const Material		*MatLib_GetConstMaterial(const MaterialLib *pML, const char *szMat);