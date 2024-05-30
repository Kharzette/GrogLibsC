#pragma once
#include	<stdbool.h>
#include	<utstring.h>


typedef struct	MeshPart_t
{
	UT_string	*mpPartName;
	UT_string	*mpMatName;
	int			mMaterialID;
	bool		mbVisible;
}	MeshPart;
