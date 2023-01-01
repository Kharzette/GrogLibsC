#pragma once
#include	<stdint.h>
#include	<stdio.h>
#include	<utstring.h>
#include	"../UtilityLib/StringStuff.h"
#include	"KeyFrame.h"


typedef struct	GSNode_t
{
	UT_string	*szName;
	int			mIndex;

	struct GSNode_t	**mpChildren;	//list of children
	int				mNumChildren;

	//current pos / rot / scale
	KeyFrame	mKeyValue;
}	GSNode;


GSNode	*GSNode_Read(FILE *f);