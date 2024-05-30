#include	<stdint.h>
#include	<stdio.h>
#include	<string.h>
#include	<cglm/call.h>
#include	"Skeleton.h"


//should match CommonFunctions.hlsli
#define	MAX_BONES			55

typedef struct	Skin_t
{
	mat4	mInverseBindPoses[MAX_BONES];
	mat4	mBindPoses[MAX_BONES];	

	mat4	mScaleMat, mInvScaleMat;	//scale to grog/quake/whateva
										//units in meters by default
	int	mNumBinds;	//num ibp/bp above
	int	mNumShapes;	//num col shapes below
										
	//malloc'd collision shapes for bones
	vec3	*mpBoneBoxes;		//double num for min/max
	vec4	*mpBoneSpheres;		//xyz center, w radius
	vec2	*mpBoneCapsules;	//x radius, y len

	int	*mpBoneColShapes;	//which shape chosen for each bone in CC

	mat4	mRootTransform;	//art prog -> grogspace
}	Skin;


void	Skin_Destroy(Skin *pSkin)
{
	free(pSkin->mpBoneBoxes);
	free(pSkin->mpBoneSpheres);
	free(pSkin->mpBoneCapsules);
	free(pSkin->mpBoneColShapes);

	free(pSkin);
}


Skin	*Skin_Read(FILE *f)
{
#ifdef __AVX__
	Skin	*pRet	=aligned_alloc(32, sizeof(Skin));
#else
	Skin	*pRet	=aligned_alloc(16, sizeof(Skin));
#endif

	glm_mat4_identity_array(pRet->mInverseBindPoses, MAX_BONES);
	glm_mat4_identity_array(pRet->mBindPoses, MAX_BONES);

	fread(&pRet->mNumBinds, sizeof(int), 1, f);

	for(int i=0;i < pRet->mNumBinds;i++)
	{
		int	idx;
		fread(&idx, sizeof(int), 1, f);

		fread(pRet->mInverseBindPoses[idx], sizeof(mat4), 1, f);

//		glm_mat4_inv(pRet->mInverseBindPoses[i], pRet->mBindPoses[i]);

		glm_mat4_transpose_to(pRet->mInverseBindPoses[i], pRet->mBindPoses[i]);
	}

	fread(&pRet->mNumShapes, sizeof(int), 1, f);

	pRet->mpBoneBoxes		=malloc(sizeof(vec3) * 2 * pRet->mNumShapes);
	pRet->mpBoneSpheres		=malloc(sizeof(vec4) * pRet->mNumShapes);
	pRet->mpBoneCapsules	=malloc(sizeof(vec2) * pRet->mNumShapes);
	pRet->mpBoneColShapes	=malloc(sizeof(int) * pRet->mNumShapes);

	fread(pRet->mpBoneBoxes, sizeof(vec3), 2 * pRet->mNumShapes, f);
	fread(pRet->mpBoneSpheres, sizeof(vec4), pRet->mNumShapes, f);
	fread(pRet->mpBoneCapsules, sizeof(vec2), pRet->mNumShapes, f);
	fread(pRet->mpBoneColShapes, sizeof(int), pRet->mNumShapes, f);

	float	scaleFactor;
	fread(&scaleFactor, sizeof(float), 1, f);
	vec3	scaleVec	={ scaleFactor, scaleFactor, scaleFactor	};
	vec3	scaleVecInv	={ 1.0f / scaleFactor, 1.0f / scaleFactor, 1.0f / scaleFactor	};

	glm_scale_make(pRet->mScaleMat, scaleVec);
	glm_scale_make(pRet->mInvScaleMat, scaleVecInv);

	fread(pRet->mRootTransform, sizeof(mat4), 1, f);

	return	pRet;
}

void	Skin_Write(const Skin *pSkin, FILE *f)
{
	fwrite(&pSkin->mNumBinds, sizeof(int), 1, f);

	for(int i=0;i < pSkin->mNumBinds;i++)
	{
		//this index write looks goofy, but it
		//is a int dictionary in C# out of order
		fwrite(&i, sizeof(int), 1, f);

		fwrite(pSkin->mInverseBindPoses[i], sizeof(mat4), 1, f);
	}

	fwrite(&pSkin->mNumShapes, sizeof(int), 1, f);

	fwrite(pSkin->mpBoneBoxes, sizeof(vec3), 2 * pSkin->mNumShapes, f);
	fwrite(pSkin->mpBoneSpheres, sizeof(vec4), pSkin->mNumShapes, f);
	fwrite(pSkin->mpBoneCapsules, sizeof(vec2), pSkin->mNumShapes, f);
	fwrite(pSkin->mpBoneColShapes, sizeof(int), pSkin->mNumShapes, f);

	//scale factor
	vec4	scaley;
	glm_sphere_transform(GLM_VEC4_ONE, pSkin->mScaleMat, scaley);
	fwrite(&scaley, sizeof(float), 1, f);

	fwrite(&pSkin->mRootTransform, sizeof(mat4), 1, f);	
}


void	Skin_FillBoneArray(const Skin *pSkin, const Skeleton *pSkel, mat4 *pBones)
{
	Skeleton_FillBoneArray(pSkel, pBones);

	for(int i=0;i < MAX_BONES;i++)
	{
		//On windows side this is: bone	=ibp * bone * rootXForm * scale
		//here it seems to be root * bone * ibp * scale
		glm_mat4_mul(pSkin->mRootTransform, pBones[i], pBones[i]);
		glm_mat4_mul(pBones[i], pSkin->mInverseBindPoses[i], pBones[i]);
		glm_mat4_mul(pSkin->mScaleMat, pBones[i], pBones[i]);
	}
}
