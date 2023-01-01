#pragma once

typedef struct	AnimLib_t	AnimLib;

extern AnimLib	*AnimLib_Read(const char *fileName);
extern void		Animate(AnimLib *pAL, const char *szAnimName, float time);