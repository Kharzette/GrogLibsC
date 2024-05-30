#include	<stdint.h>
#include	<assert.h>
#include	<stdio.h>
#include	<string.h>
#include	<cglm/call.h>
#include	"Mesh.h"
#include	"MeshPart.h"
#include	"MeshBound.h"
#include	"../MaterialLib/Material.h"
#include	"../MaterialLib/MaterialLib.h"
#include	"../MaterialLib/CBKeeper.h"
#include	"../UtilityLib/FileStuff.h"
#include	"../UtilityLib/StringStuff.h"
#include	"../UtilityLib/ListStuff.h"
#include	"../UtilityLib/DictionaryStuff.h"
#include	"../UtilityLib/GraphicsDevice.h"


typedef struct	Static_t
{
	mat4	mTransform;

	MeshBound	*mpBound;
	MeshPart	*mpParts;
	mat4		*mTransforms;

	int	mNumParts;

}	Static;


Static	*Static_Read(const char *szFileName)
{
	FILE	*f	=fopen(szFileName, "rb");
	if(f == NULL)
	{
		printf("Couldn't open file %s\n", szFileName);
		return	NULL;
	}

	uint32_t	magic;
	fread(&magic, sizeof(uint32_t), 1, f);
	if(magic != 0x57A71C15)
	{
		fclose(f);
		printf("Bad magic for Static_Read() %s\n", szFileName);
		return	NULL;
	}
	Static	*pRet	=malloc(sizeof(Static));

	fread(pRet->mTransform, sizeof(mat4), 1, f);

	pRet->mpBound	=MeshBound_Read(f);

	fread(&pRet->mNumParts, sizeof(int), 1, f);
	pRet->mpParts		=malloc(sizeof(MeshPart) * pRet->mNumParts);
#ifdef	__AVX__
	pRet->mTransforms	=aligned_alloc(32, sizeof(mat4) * pRet->mNumParts);
#else
	pRet->mTransforms	=aligned_alloc(16, sizeof(mat4) * pRet->mNumParts);
#endif

	for(int i=0;i < pRet->mNumParts;i++)
	{
		pRet->mpParts[i].mpPartName	=SZ_ReadString(f);

		fread(pRet->mTransforms[i], sizeof(mat4), 1, f);

		pRet->mpParts[i].mpMatName	=SZ_ReadString(f);

		fread(&pRet->mpParts[i].mMaterialID, sizeof(int), 1, f);
		fread(&pRet->mpParts[i].mbVisible, sizeof(bool), 1, f);
	}

	fclose(f);

	return	pRet;
}

void	Static_Write(const Static *pStat, const char *szFileName)
{
	FILE	*f	=fopen(szFileName, "wb");
	if(f == NULL)
	{
		printf("Couldn't open file %s for writing.\n", szFileName);
		return;
	}

	uint32_t	magic	=0x57A71C15;
	fwrite(&magic, sizeof(uint32_t), 1, f);

	fwrite(pStat->mTransform, sizeof(mat4), 1, f);

	MeshBound_Write(pStat->mpBound, f);

	fwrite(&pStat->mNumParts, sizeof(int), 1, f);

	for(int i=0;i < pStat->mNumParts;i++)
	{
		SZ_WriteString(f, pStat->mpParts[i].mpPartName);

		fwrite(pStat->mTransforms[i], sizeof(mat4), 1, f);

		SZ_WriteString(f, pStat->mpParts[i].mpMatName);

		fwrite(&pStat->mpParts[i].mMaterialID, sizeof(int), 1, f);
		fwrite(&pStat->mpParts[i].mbVisible, sizeof(bool), 1, f);
	}

	fclose(f);
}

void	Static_Draw(const Static *pStat, const DictSZ *pMeshes,
						const MaterialLib *pML,
						GraphicsDevice *pGD, CBKeeper *pCBK)
{
	for(int i=0;i < pStat->mNumParts;i++)
	{
		Mesh	*pMesh	=DictSZ_GetValue(pMeshes, pStat->mpParts[i].mpPartName);

		Material	*pMat	=MatLib_GetMaterial(pML,
								utstring_body(pStat->mpParts[i].mpMatName));

		if(pMesh != NULL && pMat != NULL)
		{
			MAT_SetWorld(pMat, pStat->mTransforms[i]);
			Mesh_DrawMat(pMesh, pGD, pCBK, pMat);
		}
	}
}

int	Static_GetNumParts(const Static *pStat)
{
	return	pStat->mNumParts;
}

StringList	*Static_GetPartList(const Static *pStat)
{
	StringList	*pRet	=SZList_New();

	for(int i=0;i < pStat->mNumParts;i++)
	{
		SZList_AddUT(&pRet, pStat->mpParts[i].mpPartName);
	}

	return	pRet;
}

void	Static_ReNamePart(Static *pStat, const char *pOldName, const char *pNewName)
{
	for(int i=0;i < pStat->mNumParts;i++)
	{
		int	res	=strcmp(utstring_body(pStat->mpParts[i].mpPartName), pOldName);
		if(res == 0)
		{
			utstring_clear(pStat->mpParts[i].mpPartName);
			utstring_printf(pStat->mpParts[i].mpPartName, "%s", pNewName);
		}
	}
}

void	Static_DeletePartIndex(Static *pStat, int idx)
{
	assert(idx >= 0 && idx < pStat->mNumParts);

	if(pStat->mNumParts > 1)
	{
		MeshPart	*newParts	=malloc(sizeof(MeshPart) * pStat->mNumParts - 1);
#ifdef	__AVX__
		mat4		*newXForms	=aligned_alloc(32, sizeof(mat4) * pStat->mNumParts - 1);
#else
		mat4		*newXForms	=aligned_alloc(16, sizeof(mat4) * pStat->mNumParts - 1);
#endif

		int	destIdx	=0;
		for(int i=0;i < pStat->mNumParts;i++)
		{
			if(i == idx)
			{
				utstring_done(pStat->mpParts[i].mpMatName);
				utstring_done(pStat->mpParts[i].mpPartName);
				continue;
			}

			memcpy(&newParts[destIdx], &pStat->mpParts[i], sizeof(MeshPart));
			memcpy(newXForms[destIdx], pStat->mTransforms[i], sizeof(mat4));
			destIdx++;
		}

		free(pStat->mpParts);
		free(pStat->mTransforms);

		pStat->mpParts		=newParts;
		pStat->mTransforms	=newXForms;
	}
	else
	{
		utstring_done(pStat->mpParts[0].mpMatName);
		utstring_done(pStat->mpParts[0].mpPartName);
		free(pStat->mpParts);
		pStat->mpParts	=NULL;
	}
}

void	Static_DeletePart(Static *pStat, const char *szName)
{
	for(int i=0;i < pStat->mNumParts;i++)
	{
		int	res	=strcmp(utstring_body(pStat->mpParts[i].mpPartName), szName);
		if(res == 0)
		{
			Static_DeletePartIndex(pStat, i);
			pStat->mNumParts--;
			return;
		}
	}
}

void	Static_Destroy(Static *pStat)
{
	for(int i=0;i < pStat->mNumParts;i++)
	{
		utstring_done(pStat->mpParts[i].mpMatName);
		utstring_done(pStat->mpParts[i].mpPartName);
	}

	if(pStat->mpParts != NULL)
	{
		free(pStat->mpParts);
	}
	if(pStat->mTransforms != NULL)
	{
		free(pStat->mTransforms);
	}

	MeshBound_Destroy(pStat->mpBound);

	free(pStat);
}

void	Static_AssignMaterial(Static *pStat, int partIndex, const char *pMatName)
{
	assert(partIndex < pStat->mNumParts);

	utstring_clear(pStat->mpParts[partIndex].mpMatName);

	utstring_printf(pStat->mpParts[partIndex].mpMatName, "%s", pMatName);
}

const char	*Static_GetMaterialForPart(const Static *pStat, const char *szPartName)
{
	for(int i=0;i < pStat->mNumParts;i++)
	{
		int	res	=strcmp(utstring_body(pStat->mpParts[i].mpPartName), szPartName);
		if(res == 0)
		{
			return	utstring_body(pStat->mpParts[i].mpMatName);
		}
	}
	return	NULL;
}