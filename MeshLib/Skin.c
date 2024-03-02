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

	//malloc'd collision shapes for bones
	vec3	*mpBoneBoxes;		//double num for min/max
	vec4	*mpBoneSpheres;		//xyz center, w radius
	vec2	*mpBoneCapsules;	//x radius, y len

	int	*mpBoneColShapes;	//which shape chosen for each bone in CC

	mat4	mScaleMat, mInvScaleMat;	//scale to grog/quake/whateva
										//units in meters by default

	mat4	mRootTransform;	//art prog -> grogspace
}	Skin;


Skin	*Skin_Read(FILE *f)
{
	Skin	*pRet	=malloc(sizeof(Skin));

	glm_mat4_identity_array(pRet->mInverseBindPoses, MAX_BONES);
	glm_mat4_identity_array(pRet->mBindPoses, MAX_BONES);

	int	numIBP;
	fread(&numIBP, sizeof(int), 1, f);

	for(int i=0;i < numIBP;i++)
	{
		int	idx;
		fread(&idx, sizeof(int), 1, f);

		fread(pRet->mInverseBindPoses[idx], sizeof(mat4), 1, f);

//		glm_mat4_inv(pRet->mInverseBindPoses[i], pRet->mBindPoses[i]);

		glm_mat4_transpose_to(pRet->mInverseBindPoses[i], pRet->mBindPoses[i]);
	}

	int	numBoxen;
	fread(&numBoxen, sizeof(int), 1, f);

	pRet->mpBoneBoxes		=malloc(sizeof(vec3) * 2 * numBoxen);
	pRet->mpBoneSpheres		=malloc(sizeof(vec4) * numBoxen);
	pRet->mpBoneCapsules	=malloc(sizeof(vec2) * numBoxen);
	pRet->mpBoneColShapes	=malloc(sizeof(int) * numBoxen);

	fread(pRet->mpBoneBoxes, sizeof(vec3), 2 * numBoxen, f);
	fread(pRet->mpBoneSpheres, sizeof(vec4), numBoxen, f);
	fread(pRet->mpBoneCapsules, sizeof(vec2), numBoxen, f);
	fread(pRet->mpBoneColShapes, sizeof(int), numBoxen, f);

	float	scaleFactor;
	fread(&scaleFactor, sizeof(float), 1, f);
	vec3	scaleVec	={ scaleFactor, scaleFactor, scaleFactor	};
	vec3	scaleVecInv	={ 1.0f / scaleFactor, 1.0f / scaleFactor, 1.0f / scaleFactor	};

	glm_scale_make(pRet->mScaleMat, scaleVec);
	glm_scale_make(pRet->mInvScaleMat, scaleVecInv);

	fread(pRet->mRootTransform, sizeof(mat4), 1, f);

	return	pRet;
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
