#include	<stdint.h>
#include	<stdio.h>
#include	<string.h>
#include	<cglm/call.h>
#include	"Skin.h"
#include	"Mesh.h"
#include	"MeshBound.h"
#include	"AnimLib.h"
#include	"../MaterialLib/MaterialLib.h"
#include	"../MaterialLib/CBKeeper.h"
#include	"../UtilityLib/FileStuff.h"
#include	"../UtilityLib/StringStuff.h"
#include	"../UtilityLib/ListStuff.h"
#include	"../UtilityLib/DictionaryStuff.h"
#include	"../UtilityLib/GraphicsDevice.h"

//should match CommonFunctions.hlsli
#define	MAX_BONES		55

typedef struct	MeshPart_t
{
	UT_string	*mpPartName;
	UT_string	*mpMatName;
	int			mMaterialID;
	bool		mbVisible;
}	MeshPart;

typedef struct	Character_t
{
	mat4	mTransform;

	Skin	*mpSkin;
	Mesh	*mpMesh;

	MeshBound	*mpBound;
	MeshPart	*mpParts;

	int	mNumParts;

	//bones for animation
	mat4	*mBones;

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

	fread(&pRet->mNumParts, sizeof(int), 1, f);
	pRet->mpParts	=malloc(sizeof(MeshPart) * pRet->mNumParts);

	for(int i=0;i < pRet->mNumParts;i++)
	{
		pRet->mpParts[i].mpPartName	=SZ_ReadString(f);
		pRet->mpParts[i].mpMatName	=SZ_ReadString(f);

		fread(&pRet->mpParts[i].mMaterialID, sizeof(int), 1, f);

		uint8_t	visible;

		fread(&visible, sizeof(uint8_t), 1, f);

		pRet->mpParts[i].mbVisible	=(visible != 0);
	}

	fclose(f);

	//bone array alloc
	pRet->mBones	=aligned_alloc(32, sizeof(mat4) * MAX_BONES);

	return	pRet;
}

void	Character_Draw(const Character *pChar, const DictSZ *pMeshes,
						const MaterialLib *pML, const AnimLib *pAL,
						GraphicsDevice *pGD, CBKeeper *pCBK)
{
	//set bones from anim lib / skin
	Skin_FillBoneArray(pChar->mpSkin, AnimLib_GetSkeleton(pAL), pChar->mBones);

	CBK_SetBonesWithTranspose(pCBK, pChar->mBones);
	CBK_UpdateCharacter(pCBK, pGD);
	CBK_SetCharacterToShaders(pCBK, pGD);

	for(int i=0;i < pChar->mNumParts;i++)
	{
		Mesh	*pMesh	=DictSZ_GetValue(pMeshes, pChar->mpParts[i].mpPartName);

		const Material	*pMat	=MatLib_GetConstMaterial(pML,
								utstring_body(pChar->mpParts[i].mpMatName));

		if(pMesh != NULL && pMat != NULL)
		{
			Mesh_DrawMat(pMesh, pGD, pCBK, pMat);
		}
	}
}

int	Character_GetNumParts(const Character *pChar)
{
	return	pChar->mNumParts;
}

const StringList	*Character_GetPartList(const Character *pChar)
{
	StringList	*pRet	=SZList_New();

	for(int i=0;i < pChar->mNumParts;i++)
	{
		SZList_AddUT(&pRet, pChar->mpParts[i].mpPartName);
	}

	return	pRet;
}

void	Character_AssignMaterial(Character *pChar, int partIndex, const char *pMatName)
{
	assert(partIndex < pChar->mNumParts);

	utstring_clear(pChar->mpParts[partIndex].mpMatName);

	utstring_printf(pChar->mpParts[partIndex].mpMatName, "%s", pMatName);
}