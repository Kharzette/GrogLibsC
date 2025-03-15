#include	<stdint.h>
#include	<stdio.h>
#include	<string.h>
#include	<cglm/call.h>
#include	"Skeleton.h"
#include	"Skin.h"	//shape defines


//should match CommonFunctions.hlsli
#define	MAX_BONES			55

typedef struct	Skin_t
{
	mat4	*mpInverseBindPoses;

	mat4	mScaleMat, mInvScaleMat;	//scale to grog/quake/whateva
										//units in meters by default

	//indexes from vert bone idx to skeleton node idx I think
	int	*mpJoints;

	int	mNumBinds;	//num ibp/bp above
	int	mNumShapes;	//num col shapes below
										
	//malloc'd collision shapes for bones
	vec3	*mpBoneBoxes;		//double num for min/max
	vec4	*mpBoneSpheres;		//xyz center, w radius
	vec2	*mpBoneCapsules;	//x radius, y len

	int	*mpBoneColShapes;	//which shape chosen for each bone in CC

	mat4	mRootTransform;	//art prog -> grogspace
	mat4	mScaledRoot;	//scaleMat * root
}	Skin;


Skin	*Skin_Create(mat4 *pIBPs, int joints[], int numBinds)
{
#ifdef __AVX__
	Skin	*pRet	=aligned_alloc(32, sizeof(Skin));
	memset(pRet, 0, sizeof(Skin));

	pRet->mpInverseBindPoses	=aligned_alloc(32, sizeof(mat4) * numBinds);	
#else
	Skin	*pRet	=aligned_alloc(16, sizeof(Skin));
	memset(pRet, 0, sizeof(Skin));

	pRet->mpInverseBindPoses	=aligned_alloc(16, sizeof(mat4) * numBinds);	
#endif

	memcpy(pRet->mpInverseBindPoses, pIBPs, sizeof(mat4) * numBinds);

	pRet->mNumShapes	=pRet->mNumBinds	=numBinds;

	pRet->mpJoints	=malloc(sizeof(int) * numBinds);

	memcpy(pRet->mpJoints, joints, sizeof(int) * numBinds);

	glm_mat4_identity(pRet->mRootTransform);
	glm_mat4_identity(pRet->mScaledRoot);
	glm_mat4_identity(pRet->mScaleMat);
	glm_mat4_identity(pRet->mInvScaleMat);

	pRet->mpBoneBoxes		=malloc(sizeof(vec3) * 2 * pRet->mNumShapes);
	pRet->mpBoneSpheres		=malloc(sizeof(vec4) * pRet->mNumShapes);
	pRet->mpBoneCapsules	=malloc(sizeof(vec2) * pRet->mNumShapes);
	pRet->mpBoneColShapes	=malloc(sizeof(int) * pRet->mNumShapes);

	//some default shapes
	vec3	bxMax	={	.1, .1, .1		};
	vec3	bxMin	={	-.1, -.1, -.1	};
	vec4	sphere	={	0, 0, 0, 0.1	};
	vec2	cap		={	0.1, 0.3		};

	for(int i=0;i < numBinds;i++)
	{
		pRet->mpBoneColShapes[i]	=BONE_COL_SHAPE_INVALID;

		//box
		glm_vec3_copy(bxMin, pRet->mpBoneBoxes[i * 2]);
		glm_vec3_copy(bxMax, pRet->mpBoneBoxes[i * 2 + 1]);

		//sphere
		glm_vec4_copy(sphere, pRet->mpBoneSpheres[i]);

		//capsule
		glm_vec2_copy(cap, pRet->mpBoneCapsules[i]);
	}

	glm_scale_make(pRet->mRootTransform, (vec3){-1, 1, 1});

//	glm_rotate_x(pRet->mRootTransform, -GLM_PI_2f, pRet->mRootTransform);

	//set up the root transform to do right to left
//	pRet->mRootTransform[2][0]	=-pRet->mRootTransform[2][0];
//	pRet->mRootTransform[2][1]	=-pRet->mRootTransform[2][1];
//	pRet->mRootTransform[2][2]	=-pRet->mRootTransform[2][2];
//	pRet->mRootTransform[2][3]	=-pRet->mRootTransform[2][3];

//	pRet->mRootTransform[0][2]	=-pRet->mRootTransform[0][2];
//	pRet->mRootTransform[1][2]	=-pRet->mRootTransform[1][2];
//	pRet->mRootTransform[2][2]	=-pRet->mRootTransform[2][2];
//	pRet->mRootTransform[3][2]	=-pRet->mRootTransform[3][2];

	return	pRet;
}


void	Skin_Destroy(Skin *pSkin)
{
	free(pSkin->mpInverseBindPoses);
	free(pSkin->mpJoints);
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

	fread(&pRet->mNumBinds, sizeof(int), 1, f);

#ifdef __AVX__
	pRet->mpInverseBindPoses	=aligned_alloc(32, sizeof(mat4) * pRet->mNumBinds);	
#else
	pRet->mpInverseBindPoses	=aligned_alloc(16, sizeof(mat4) * pRet->mNumBinds);	
#endif

	for(int i=0;i < pRet->mNumBinds;i++)
	{
		int	idx;
		fread(&idx, sizeof(int), 1, f);

		fread(pRet->mpInverseBindPoses[idx], sizeof(mat4), 1, f);
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

	glm_mat4_mul(pRet->mScaleMat, pRet->mRootTransform, pRet->mScaledRoot);

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

		fwrite(pSkin->mpInverseBindPoses[i], sizeof(mat4), 1, f);
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
	Skeleton_FillBoneArray(pSkel, pSkin->mpJoints, pBones, pSkin->mNumBinds);

	for(int i=0;i < pSkin->mNumBinds;i++)
	{
		//On windows side this is: bone	=ibp * bone * rootXForm * scale
		//here it seems to be root * bone * ibp * scale
		glm_mat4_mul(pSkin->mRootTransform, pBones[i], pBones[i]);
		glm_mat4_mul(pBones[i], pSkin->mpInverseBindPoses[i], pBones[i]);
		glm_mat4_mul(pSkin->mScaleMat, pBones[i], pBones[i]);
	}
}

int	Skin_GetNumBones(const Skin *pSkin)
{
	return	pSkin->mNumBinds;
}

int		Skin_GetBoundChoice(const Skin *pSkin, int boneIdx)
{
	assert(pSkin != NULL);

	//can have bones that aren't listed for bounds
	//like maybe root bones?
	if(boneIdx < 0 || boneIdx >= pSkin->mNumShapes)
	{
		return	BONE_COL_SHAPE_INVALID;
	}
	
	return	pSkin->mpBoneColShapes[boneIdx];
}

void	Skin_GetBoxBoundSize(const Skin *pSkin, int boneIdx, vec3 min, vec3 max)
{
	assert(pSkin != NULL);

	//can have bones that aren't listed for bounds
	//like maybe root bones?
	if(boneIdx < 0 || boneIdx >= pSkin->mNumShapes)
	{
		glm_vec3_zero(min);
		glm_vec3_zero(max);
		return;
	}

	glm_vec3_copy(pSkin->mpBoneBoxes[boneIdx * 2], min);
	glm_vec3_copy(pSkin->mpBoneBoxes[(boneIdx * 2) + 1], max);
}

void	Skin_GetCapsuleBoundSize(const Skin *pSkin, int boneIdx, vec2 size)
{
	assert(pSkin != NULL);

	//can have bones that aren't listed for bounds
	//like maybe root bones?
	if(boneIdx < 0 || boneIdx >= pSkin->mNumShapes)
	{
		glm_vec2_zero(size);
		return;
	}

	glm_vec2_copy(pSkin->mpBoneCapsules[boneIdx], size);
}

void	Skin_GetSphereBoundSize(const Skin *pSkin, int boneIdx, vec4 size)
{
	assert(pSkin != NULL);

	//can have bones that aren't listed for bounds
	//like maybe root bones?
	if(boneIdx < 0 || boneIdx >= pSkin->mNumShapes)
	{
		glm_vec4_zero(size);
		return;
	}

	glm_vec4_copy(pSkin->mpBoneSpheres[boneIdx], size);
}

void	Skin_GetBoneByIndex(const Skin *pSkin, const Skeleton *pSkel,
									int boneIdx, mat4 outMat)
{
	if(!Skeleton_GetMatrixForBoneIndex(pSkel, boneIdx, outMat))
	{
		glm_mat4_identity(outMat);
	}

	//On windows side this is: bone	=ibp * bone * rootXForm * scale
	//here it seems to be root * bone * ibp * scale
	glm_mat4_mul(pSkin->mRootTransform, outMat, outMat);
	glm_mat4_mul(outMat, pSkin->mpInverseBindPoses[boneIdx], outMat);
	glm_mat4_mul(pSkin->mScaleMat, outMat, outMat);
}

void	Skin_GetBoneByIndexNoBind(const Skin *pSkin, const Skeleton *pSkel,
									int boneIdx, mat4 outMat)
{
	if(!Skeleton_GetMatrixForBoneIndex(pSkel, boneIdx, outMat))
	{
		glm_mat4_identity(outMat);
	}

	glm_mat4_mul(pSkin->mScaledRoot, outMat, outMat);
}


void	Skin_SetBoundChoice(Skin *pSkin, int boneIdx, int choice)
{
	assert(pSkin != NULL);

	//can have bones that aren't listed for bounds
	//like maybe root bones?
	if(boneIdx < 0 || boneIdx >= pSkin->mNumShapes)
	{
		printf("Invalid index in Skin_SetBoundChoice()!\n");
		return;
	}

	assert(choice >= BONE_COL_SHAPE_BOX && choice <= BONE_COL_SHAPE_INVALID);

	pSkin->mpBoneColShapes[boneIdx]	=choice;
}

void	Skin_SetBoxBoundSize(Skin *pSkin, int boneIdx, const vec3 min, const vec3 max)
{
	assert(pSkin != NULL);

	//can have bones that aren't listed for bounds
	//like maybe root bones?
	if(boneIdx < 0 || boneIdx >= pSkin->mNumShapes)
	{
		return;
	}

	glm_vec3_copy(min, pSkin->mpBoneBoxes[boneIdx * 2]);
	glm_vec3_copy(max, pSkin->mpBoneBoxes[(boneIdx * 2) + 1]);
}

void	Skin_SetCapsuleBoundSize(Skin *pSkin, int boneIdx, const vec2 size)
{
	assert(pSkin != NULL);

	//can have bones that aren't listed for bounds
	//like maybe root bones?
	if(boneIdx < 0 || boneIdx >= pSkin->mNumShapes)
	{
		return;
	}

	glm_vec2_copy(size, pSkin->mpBoneCapsules[boneIdx]);
}

void	Skin_SetSphereBoundSize(Skin *pSkin, int boneIdx, const vec4 size)
{
	assert(pSkin != NULL);

	//can have bones that aren't listed for bounds
	//like maybe root bones?
	if(boneIdx < 0 || boneIdx >= pSkin->mNumShapes)
	{
		return;
	}

	glm_vec4_copy(size, pSkin->mpBoneSpheres[boneIdx]);
}