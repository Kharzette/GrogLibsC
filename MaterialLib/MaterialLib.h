#pragma once
#include	<stdint.h>
#include	<cglm/call.h>

typedef struct	MaterialLib_t	MaterialLib;
typedef struct	StuffKeeper_t	StuffKeeper;
typedef struct	StringList_t	StringList;

MaterialLib		*MatLib_Create(StuffKeeper *pSK);
MaterialLib		*MatLib_Read(const char *pFileName, StuffKeeper *pSK);
void			MatLib_Write(const MaterialLib *pML, const char *szFileName);
void			MatLib_Add(MaterialLib *pML, const char *szName, Material *pMat);
int				MatLib_GetNumMats(const MaterialLib *pML);
StringList		*MatLib_GetMatList(const MaterialLib *pML);
Material		*MatLib_GetMaterial(MaterialLib *pML, const char *szMat);
const Material	*MatLib_GetConstMaterial(const MaterialLib *pML, const char *szMat);
void			MatLib_Remove(MaterialLib *pML, const char *szName);
void			MatLib_ReName(MaterialLib *pML, const char *szMatName, const char *szNewName);