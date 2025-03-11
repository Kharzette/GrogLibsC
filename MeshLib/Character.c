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


Character	*Character_Create(Skin *pSkin, Mesh *pMeshes[], int numMeshes)
{
#ifdef __AVX__
	Character	*pRet	=aligned_alloc(32, sizeof(Character));
#else
	Character	*pRet	=aligned_alloc(16, sizeof(Character));
#endif

	memset(pRet, 0, sizeof(Character));

	glm_mat4_identity(pRet->mTransform);

	pRet->mpSkin	=pSkin;

	pRet->mpParts	=malloc(sizeof(MeshPart) * numMeshes);

	for(int i=0;i < numMeshes;i++)
	{
		pRet->mpParts[i].mpPart			=pMeshes[i];
		pRet->mpParts[i].mbVisible		=true;
		pRet->mpParts[i].mMaterialID	=0;

		utstring_new(pRet->mpParts[i].mpMaterial);

		utstring_printf(pRet->mpParts[i].mpMaterial, "default");
	}
	pRet->mNumParts	=numMeshes;

	int	numBones	=Skin_GetNumBones(pSkin);

#ifdef __AVX__
	pRet->mBones	=aligned_alloc(32, sizeof(mat4) * numBones);
#else
	pRet->mBones	=aligned_alloc(16, sizeof(mat4) * numBones);
#endif

	return	pRet;
}


Character	*Character_Read(GraphicsDevice *pGD, StuffKeeper *pSK,
							const char *szFileName, bool bEditor)
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
		pRet->mpParts[i].mpPart	=Mesh_Read(pGD, pSK, f, bEditor);

		pRet->mpParts[i].mpMaterial	=SZ_ReadString(f);

		fread(&pRet->mpParts[i].mMaterialID, sizeof(int), 1, f);

		uint8_t	visible;

		fread(&visible, sizeof(uint8_t), 1, f);

		pRet->mpParts[i].mbVisible	=(visible != 0);
	}

	fclose(f);

	int	numBones	=Skin_GetNumBones(pRet->mpSkin);

	//bone array alloc
#ifdef __AVX__
	pRet->mBones	=aligned_alloc(32, sizeof(mat4) * numBones);
#else
	pRet->mBones	=aligned_alloc(16, sizeof(mat4) * numBones);
#endif

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
		Mesh_Write(pChar->mpParts[i].mpPart, f);

		SZ_WriteString(f, pChar->mpParts[i].mpMaterial);

		fwrite(&pChar->mpParts[i].mMaterialID, sizeof(int), 1, f);
		fwrite(&pChar->mpParts[i].mbVisible, sizeof(bool), 1, f);
	}

	fclose(f);
}

void	Character_Draw(const Character *pChar,
						const MaterialLib *pML, const AnimLib *pAL,
						GraphicsDevice *pGD, CBKeeper *pCBK)
{
	assert(pChar != NULL);

	//set bones from anim lib / skin
	Skin_FillBoneArray(pChar->mpSkin, AnimLib_GetSkeleton(pAL), pChar->mBones);

	CBK_SetBonesWithTranspose(pCBK, pChar->mBones, Skin_GetNumBones(pChar->mpSkin));
	CBK_UpdateCharacter(pCBK, pGD);
	CBK_SetCharacterToShaders(pCBK, pGD);

	for(int i=0;i < pChar->mNumParts;i++)
	{
		const Material	*pMat	=MatLib_GetConstMaterial(pML,
									utstring_body(pChar->mpParts[i].mpMaterial));

		if(pMat != NULL)
		{
			Mesh_DrawMat(pChar->mpParts[i].mpPart, pGD, pCBK, pMat);
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
		SZList_AddUT(&pRet, Mesh_GetName(pChar->mpParts[i].mpPart));
	}

	return	pRet;
}

bool	Character_RayIntersectBones(const Character *pChar, const vec3 startPos, const vec3 endPos,
									int *pHitIndex, vec3 hitPos, vec3 hitNorm)
{
	assert(pChar != NULL);

	for(int i=0;i < MAX_BONES;i++)
	{
		mat4	boneToWorld;

		glm_mat4_mul(pChar->mBones[i], pChar->mTransform, boneToWorld);

		int	choice	=Skin_GetBoundChoice(pChar->mpSkin, i);

		if(choice == BONE_COL_SHAPE_BOX)
		{
		}
		else if(choice == BONE_COL_SHAPE_CAPSULE)
		{
		}
		else if(choice == BONE_COL_SHAPE_SPHERE)
		{
		}
		else if(choice == BONE_COL_SHAPE_INVALID)
		{
			continue;
		}
	}


}

void	Character_ReNamePart(Character *pChar, const char *pOldName, const char *pNewName)
{
	assert(pChar != NULL);

	for(int i=0;i < pChar->mNumParts;i++)
	{
		const UT_string	*pPartName	=Mesh_GetName(pChar->mpParts[i].mpPart);

		int	res	=strcmp(utstring_body(pPartName), pOldName);
		if(res == 0)
		{
			Mesh_SetName(pChar->mpParts[i].mpPart, pNewName);
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
				utstring_done(pChar->mpParts[i].mpMaterial);
				Mesh_Destroy(pChar->mpParts[i].mpPart);
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
		utstring_done(pChar->mpParts[0].mpMaterial);
		Mesh_Destroy(pChar->mpParts[0].mpPart);
		free(pChar->mpParts);
		pChar->mpParts	=NULL;
	}
}

void	Character_DeletePart(Character *pChar, const char *szName)
{
	assert(pChar != NULL);

	for(int i=0;i < pChar->mNumParts;i++)
	{
		const UT_string	*pPartName	=Mesh_GetName(pChar->mpParts[i].mpPart);
		int	res	=strcmp(utstring_body(pPartName), szName);
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
		utstring_done(pChar->mpParts[i].mpMaterial);
		Mesh_Destroy(pChar->mpParts[i].mpPart);
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

	utstring_clear(pChar->mpParts[partIndex].mpMaterial);

	utstring_printf(pChar->mpParts[partIndex].mpMaterial, "%s", pMatName);
}

const char	*Character_GetMaterialForPart(const Character *pChar, const char *szPartName)
{
	assert(pChar != NULL);

	for(int i=0;i < pChar->mNumParts;i++)
	{
		const UT_string	*pPartName	=Mesh_GetName(pChar->mpParts[i].mpPart);
		int	res	=strcmp(utstring_body(pPartName), szPartName);
		if(res == 0)
		{
			return	utstring_body(pChar->mpParts[i].mpMaterial);
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