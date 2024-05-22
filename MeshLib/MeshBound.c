#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<string.h>
#include	<cglm/call.h>


typedef struct	MeshBound_t
{
	int	mNumParts;

	vec4	mSphere;	//center + radius
	vec3	mBox[2];
	bool	mbChoice;	//true for box

	//bound for each part
	vec3	**mpPartBoxes;
	vec4	*mpPartSpheres;
	bool	*mpChoices;
}	MeshBound;


void	MeshBound_Destroy(MeshBound *pMB)
{
	for(int i=0;i < pMB->mNumParts;i++)
	{
		free(pMB->mpPartBoxes[i]);
	}

	if(pMB->mpPartBoxes != NULL)
	{
		free(pMB->mpPartBoxes);
	}
	if(pMB->mpPartSpheres != NULL)
	{
		free(pMB->mpPartSpheres);
	}
	if(pMB->mpChoices != NULL)
	{
		free(pMB->mpChoices);
	}

	free(pMB);
}

MeshBound	*MeshBound_Read(FILE *f)
{
	MeshBound	*pRet	=malloc(sizeof(MeshBound));
	memset(pRet, 0, sizeof(MeshBound));

	fread(&pRet->mSphere, sizeof(vec4), 1, f);
	fread(&pRet->mBox, sizeof(vec3), 2, f);

	uint8_t	choice;
	fread(&choice, 1, 1, f);	//not sure the size of C# bool

	pRet->mbChoice	=(choice != 0);

	fread(&pRet->mNumParts, sizeof(int), 1, f);

	if(pRet->mNumParts == 0)
	{
		return	pRet;
	}

	pRet->mpPartBoxes	=malloc(sizeof(vec3 *) * pRet->mNumParts);
	pRet->mpPartSpheres	=malloc(sizeof(vec4) * pRet->mNumParts);
	pRet->mpChoices		=malloc(sizeof(bool) * pRet->mNumParts);

	for(int i=0;i < pRet->mNumParts;i++)
	{
		pRet->mpPartBoxes[i]	=malloc(sizeof(vec3) * 2);

		fread(pRet->mpPartBoxes[i], sizeof(vec3), 2, f);
		fread(pRet->mpPartSpheres[i], sizeof(vec4), 1, f);
		fread(&pRet->mpChoices[i], sizeof(bool), 1, f);
	}

	return	pRet;
}