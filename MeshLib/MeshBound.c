#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<string.h>
#include	<cglm/call.h>


typedef struct	MeshBound_t
{
	vec4	mSphere;	//center + radius
	vec3	mBox[2];
	bool	mbChoice;	//true for box

	//bound for each part
	vec3	**mpPartBoxes;
	vec4	*mpPartSpheres;
	bool	*mpChoices;
}	MeshBound;


MeshBound	*MeshBound_Read(FILE *f)
{
	MeshBound	*pRet	=malloc(sizeof(MeshBound));

	fread(&pRet->mSphere, sizeof(vec4), 1, f);
	fread(&pRet->mBox, sizeof(vec3), 2, f);

	uint8_t	choice;
	fread(&choice, 1, 1, f);	//not sure the size of C# bool

	pRet->mbChoice	=(choice != 0);

	int	numParts;
	fread(&numParts, sizeof(int), 1, f);

	if(numParts == 0)
	{
		return	pRet;
	}

	pRet->mpPartBoxes	=malloc(sizeof(vec3 *) * numParts);
	pRet->mpPartSpheres	=malloc(sizeof(vec4) * numParts);
	pRet->mpChoices		=malloc(sizeof(bool) * numParts);

	for(int i=0;i < numParts;i++)
	{
		pRet->mpPartBoxes[i]	=malloc(sizeof(vec3) * 2);

		fread(pRet->mpPartBoxes[i], sizeof(vec3), 2, f);
		fread(pRet->mpPartSpheres[i], sizeof(vec4), 1, f);
		fread(&pRet->mpChoices[i], sizeof(bool), 1, f);
	}

	return	pRet;
}