#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<errno.h>
#include	<assert.h>
#include	<cglm/call.h>
#include	"DictionaryStuff.h"
#include	"StringStuff.h"

#define	MouseTurnMultiplier		=0.8f
#define	AnalogTurnMultiplier	=300f
#define	KeyTurnMultiplier		=300f

typedef struct	UserSettings_t
{
	float	mTurnSensitivity;
	bool	mbXAxisInverted;
	bool	mbYAxisInverted;
	bool	mbMultiSampling;
	bool	mbFullScreen;
	bool	mbESDF;

	int		mNumPositions;
	DictSZ	*mPositions;
}	UserSettings;


UserSettings	*UserSettings_Create(void)
{
	UserSettings	*pRet	=malloc(sizeof(UserSettings));

	memset(pRet, 0, sizeof(UserSettings));

	//defaults
	pRet->mTurnSensitivity	=1.0f;

	DictSZ_New(&pRet->mPositions);

	return	pRet;
}

void	UserSettings_Destroy(UserSettings **ppUS)
{
	UserSettings	*pUS	=*ppUS;

	DictSZ_Clear(&pUS->mPositions);

	free(pUS);

	*ppUS	=NULL;
}


void	UserSettings_Load(UserSettings *pUS)
{
	FILE	*f	=fopen("ControlSettings.sav", "rb");
	if(f == NULL)
	{
		return;
	}

	//read the control stuff
	fread(pUS, sizeof(UserSettings) - sizeof(pUS->mPositions), 1, f);

	for(int i=0;i < pUS->mNumPositions;i++)
	{
		UT_string	*pKey	=SZ_ReadString(f);

		//need a real chunk of ram for this
		vec2	*pVal	=malloc(sizeof(vec2));
		fread(pVal, sizeof(vec2), 1, f);

		DictSZ_Add(&pUS->mPositions, pKey, pVal);

		utstring_free(pKey);
	}

	fclose(f);
}

void	SaveCB(const UT_string *pKey, const void *pValue, void *pContext)
{
	FILE	*f		=(FILE *)pContext;
	vec2	*pVec	=(vec2 *)pValue;
	if(f == NULL || pVec == NULL)
	{
		return;
	}

	SZ_WriteString(f, pKey);

	fwrite(pVec, sizeof(vec2), 1, f);
}

void	UserSettings_Save(const UserSettings *pUS)
{
	FILE	*f	=fopen("ControlSettings.sav", "wb");
	if(f == NULL)
	{
		printf("Goblinry with config file: %s\n", strerror(errno));
		return;
	}

	fwrite(pUS, sizeof(UserSettings) - sizeof(pUS->mPositions), 1, f);

	DictSZ_ForEach(pUS->mPositions, SaveCB, f);

	fclose(f);
}

void	UserSettings_AddPosition(UserSettings *pUS, const char *pKey, float x, float y)
{
	vec2	*pVec;

	if(DictSZ_ContainsKeyccp(pUS->mPositions, pKey))
	{
		pVec	=DictSZ_GetValueccp(pUS->mPositions, pKey);

		pVec[0][0]	=x;
		pVec[0][1]	=y;
	}
	else
	{
		pVec	=malloc(sizeof(vec2));

		pVec[0][0]	=x;
		pVec[0][1]	=y;

		DictSZ_Addccp(&pUS->mPositions, pKey, pVec);
		pUS->mNumPositions++;
	}
}

void	UserSettings_GetPosition(const UserSettings *pUS, const char *pKey, vec2 pos)
{
	if(!DictSZ_ContainsKeyccp(pUS->mPositions, pKey))
	{
		pos[0]	=pos[1]	=0.0f;
		return;
	}

	vec2	*pVec	=DictSZ_GetValueccp(pUS->mPositions, pKey);

	glm_vec2_copy(*pVec, pos);
}