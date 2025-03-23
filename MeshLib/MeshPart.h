#pragma once
#include	<stdbool.h>
#include	<utstring.h>

typedef struct	Mesh_t	Mesh;


typedef struct	MeshPart_t
{
	Mesh		*mpPart;
	UT_string	*mpMaterial;
	int			mMaterialID;
	bool		mbVisible;
}	MeshPart;
