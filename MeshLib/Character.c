#include	<stdint.h>
#include	<assert.h>
#include	<stdio.h>
#include	<string.h>
#include	<cglm/call.h>
#include	"MeshPart.h"
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


typedef struct	Character_t
{
	mat4	mTransform;

	Skin	*mpSkin;

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
		printf("Couldn't open file %s\n", szFileName);
		return	NULL;
	}

	uint32_t	magic;
	fread(&magic, sizeof(uint32_t), 1, f);
	if(magic != 0xCA1EC7BE)
	{
		fclose(f);
		printf("Bad magic for Character_Read() %s\n", szFileName);
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

void	Character_Write(const Character *pChar, const char *szFileName)
{
	FILE	*f	=fopen(szFileName, "wb");
	if(f == NULL)
	{
		printf("Couldn't open file %s for writing.\n", szFileName);
		return;
	}

	uint32_t	magic	=0xCA1EC7BE;
	fwrite(&magic, sizeof(uint32_t), 1, f);

	fwrite(pChar->mTransform, sizeof(mat4), 1, f);

	MeshBound_Write(pChar->mpBound, f);
	Skin_Write(pChar->mpSkin, f);

	fwrite(&pChar->mNumParts, sizeof(int), 1, f);

	for(int i=0;i < pChar->mNumParts;i++)
	{
		SZ_WriteString(f, pChar->mpParts[i].mpPartName);
		SZ_WriteString(f, pChar->mpParts[i].mpMatName);

		fwrite(&pChar->mpParts[i].mMaterialID, sizeof(int), 1, f);
		fwrite(&pChar->mpParts[i].mbVisible, sizeof(bool), 1, f);
	}

	fclose(f);
}

void	Character_Draw(const Character *pChar, const DictSZ *pMeshes,
						const MaterialLib *pML, const AnimLib *pAL,
						GraphicsDevice *pGD, CBKeeper *pCBK)
{
	assert(pChar != NULL);

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
	assert(pChar != NULL);

	return	pChar->mNumParts;
}

StringList	*Character_GetPartList(const Character *pChar)
{
	assert(pChar != NULL);

	StringList	*pRet	=SZList_New();

	for(int i=0;i < pChar->mNumParts;i++)
	{
		SZList_AddUT(&pRet, pChar->mpParts[i].mpPartName);
	}

	return	pRet;
}

void	Character_ReNamePart(Character *pChar, const char *pOldName, const char *pNewName)
{
	assert(pChar != NULL);

	for(int i=0;i < pChar->mNumParts;i++)
	{
		int	res	=strcmp(utstring_body(pChar->mpParts[i].mpPartName), pOldName);
		if(res == 0)
		{
			utstring_clear(pChar->mpParts[i].mpPartName);
			utstring_printf(pChar->mpParts[i].mpPartName, "%s", pNewName);
		}
	}
}

void	Character_DeletePartIndex(Character *pChar, int idx)
{
	assert(pChar != NULL);
	assert(idx >= 0 && idx < pChar->mNumParts);

	if(pChar->mNumParts > 1)
	{
		MeshPart	*newParts	=malloc(sizeof(MeshPart) * pChar->mNumParts - 1);

		int	destIdx	=0;
		for(int i=0;i < pChar->mNumParts;i++)
		{
			if(i == idx)
			{
				utstring_done(pChar->mpParts[i].mpMatName);
				utstring_done(pChar->mpParts[i].mpPartName);
				continue;
			}

			memcpy(&newParts[destIdx], &pChar->mpParts[i], sizeof(MeshPart));
			destIdx++;
		}

		free(pChar->mpParts);
		pChar->mpParts	=newParts;
	}
	else
	{
		utstring_done(pChar->mpParts[0].mpMatName);
		utstring_done(pChar->mpParts[0].mpPartName);
		free(pChar->mpParts);
		pChar->mpParts	=NULL;
	}
}

void	Character_DeletePart(Character *pChar, const char *szName)
{
	assert(pChar != NULL);

	for(int i=0;i < pChar->mNumParts;i++)
	{
		int	res	=strcmp(utstring_body(pChar->mpParts[i].mpPartName), szName);
		if(res == 0)
		{
			Character_DeletePartIndex(pChar, i);
			pChar->mNumParts--;
			return;
		}
	}
}

void	Character_Destroy(Character *pChar)
{
	assert(pChar != NULL);

	free(pChar->mBones);

	for(int i=0;i < pChar->mNumParts;i++)
	{
		utstring_done(pChar->mpParts[i].mpMatName);
		utstring_done(pChar->mpParts[i].mpPartName);
	}

	if(pChar->mpParts != NULL)
	{
		free(pChar->mpParts);
	}

	MeshBound_Destroy(pChar->mpBound);
	Skin_Destroy(pChar->mpSkin);

	free(pChar);
}

void	Character_AssignMaterial(Character *pChar, int partIndex, const char *pMatName)
{
	assert(pChar != NULL);
	assert(partIndex < pChar->mNumParts);

	utstring_clear(pChar->mpParts[partIndex].mpMatName);

	utstring_printf(pChar->mpParts[partIndex].mpMatName, "%s", pMatName);
}

const char	*Character_GetMaterialForPart(const Character *pChar, const char *szPartName)
{
	assert(pChar != NULL);

	for(int i=0;i < pChar->mNumParts;i++)
	{
		int	res	=strcmp(utstring_body(pChar->mpParts[i].mpPartName), szPartName);
		if(res == 0)
		{
			return	utstring_body(pChar->mpParts[i].mpMatName);
		}
	}
	return	NULL;
}

const Skin	*Character_GetConstSkin(const Character *pChar)
{
	assert(pChar != NULL);
	
	return	pChar->mpSkin;
}

Skin	*Character_GetSkin(const Character *pChar)
{
	assert(pChar != NULL);
	
	return	pChar->mpSkin;
}