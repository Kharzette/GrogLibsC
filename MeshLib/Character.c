#include	<stdint.h>
#include	<stdio.h>
#include	<string.h>
#include	<cglm/call.h>
#include	"Skin.h"
#include	"Mesh.h"
#include	"MeshBound.h"
#include	"../UtilityLib/FileStuff.h"


typedef struct	Character_t
{
	mat4	mTransform;

	Skin	*mpSkin;
	Mesh	*mpMesh;

	MeshBound	*mpBound;

}	Character;


Character	*Character_Read(const char *szFileName)
{
	FILE	*f	=fopen(szFileName, "rb");
	if(f == NULL)
	{
		return	NULL;
	}

	uint32_t	magic;
	fread(&magic, sizeof(uint32_t), 1, f);
	if(magic != 0xCA1EC7BE)
	{
		fclose(f);
		return	NULL;
	}
	Character	*pRet	=malloc(sizeof(Character));

	fread(pRet->mTransform, sizeof(mat4), 1, f);

	pRet->mpBound	=MeshBound_Read(f);

	pRet->mpSkin	=Skin_Read(f);

	//TODO: read rest
	return	pRet;
}


void	Character_FillBoneArray(const Character *pChar, const Skeleton *pSkel, mat4 *pBones)
{
	Skin_FillBoneArray(pChar->mpSkin, pSkel, pBones);
}
