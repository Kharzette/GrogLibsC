#pragma once
#include	<stdio.h>


typedef struct	MeshBound_t	MeshBound;

MeshBound	*MeshBound_Read(FILE *f);
void		MeshBound_Destroy(MeshBound *pMB);