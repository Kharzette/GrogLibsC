#include	<stdint.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<cglm/call.h>
#include	<dirent.h>
#include	<errno.h>
#include	<sys/stat.h>
#include	<assert.h>
#include	<string.h>
#include	<FAudio.h>
#include	<F3DAudio.h>
#include	"../UtilityLib/StringStuff.h"
#include	"tinywav/tinywav.h"
#include	"SoundEffect.h"
#include	"Audio.h"


#define	SFX_NAME_LEN		32
#define	SFX_MAX_COUNT		64
#define	SRC_VOICE_CHANNELS	32


//lazy data
typedef struct
{
	char	mszName[SFX_NAME_LEN];

	//faudio stuff
	FAudioSourceVoice	*mpSrcVoice;
	FAudioBuffer		mBuffer;
	FAudioVoiceCallback	mCB;

	//3d stuff
	F3DAUDIO_EMITTER		mEmitter;
	F3DAUDIO_DSP_SETTINGS	mDSP;

	//state stuff, api is very confusing
	bool	mbPlaying;
	bool	mbPlayedOnce;
}	SoundEffect;

typedef struct
{
	SoundEffect	*mpSFX;
	bool		mbPlaying, mb3D;
}	SoundEffectInstance;


//lazy storage
SoundEffect	sSoundFX[SFX_MAX_COUNT]	={0};

//effects probably only loaded by the main thread?
uint32_t	sNumSFX	=0;


//callback forward decs
static void FAUDIOCALL sOnBufferEnd(FAudioVoiceCallback *pCB, void *pContext);
static void FAUDIOCALL sOnBufferStart(FAudioVoiceCallback *pCB, void *pContext);
static void FAUDIOCALL sOnLoopEnd(FAudioVoiceCallback *pCB, void *pContext);
static void FAUDIOCALL sOnStreamEnd(FAudioVoiceCallback *pCB);
static void FAUDIOCALL sOnVoiceError(FAudioVoiceCallback *pCB, void *pContext, uint32_t Error);
static void FAUDIOCALL sOnVoiceProcessingPassStart(FAudioVoiceCallback *pCB, uint32_t BytesRequired);
static void FAUDIOCALL sOnVoiceProcessingPassEnd(FAudioVoiceCallback *pCB);

//statics
static void	sPrintErr(int err);
static int	sGetIndex(const char *szName);
static bool	sCheckExisting(const char *szName);


//loadup a sound effect (non streamed)
bool    SoundEffect_Create(const char *szName, const char *szPath,
							FAudio *pFA, uint32_t numChannels, int *pIdx)
{
	if(sNumSFX == SFX_MAX_COUNT)
	{
		return	false;	//too many loaded
	}

	TinyWav	tw;

	//check for existing?
	if(sCheckExisting(szName))
	{
		*pIdx	=sGetIndex(szName);
		return	true;	//already loaded?
	}

	int	iRes	=tinywav_open_read(&tw, szPath, TW_SPLIT);
	if(iRes)
	{
		printf("Error loading %s\n", szPath);
		return	false;
	}
	
	FAudioSourceVoice	*pFASV;
	
	FAudioWaveFormatEx	wfx;
	wfx.cbSize			=0;
	wfx.nBlockAlign		=tw.numChannels * (tw.h.BitsPerSample / 8);
	wfx.nChannels		=tw.numChannels;
	wfx.nSamplesPerSec	=tw.h.SampleRate;
	wfx.wFormatTag		=tw.h.AudioFormat;
	wfx.wBitsPerSample	=tw.sampFmt * 8;	//the enum is also the size in bytes
	wfx.nAvgBytesPerSec	=tw.h.SampleRate * wfx.nBlockAlign;

	//grab pointer to current voice data chunk off static array
	SoundEffect	*pCur	=&sSoundFX[sNumSFX];
	
	//structs passed to create voice, aim at the current static array spot
	FAudioBuffer		*pAB	=&pCur->mBuffer;
	FAudioVoiceCallback	*pCB	=&pCur->mCB;

	//callbacks!
	pCB->OnBufferEnd				=sOnBufferEnd;
	pCB->OnBufferStart				=sOnBufferStart;
	pCB->OnLoopEnd					=sOnLoopEnd;
	pCB->OnStreamEnd				=sOnStreamEnd;
	pCB->OnVoiceError				=sOnVoiceError;
	pCB->OnVoiceProcessingPassEnd	=sOnVoiceProcessingPassEnd;
	pCB->OnVoiceProcessingPassStart	=sOnVoiceProcessingPassStart;
	
	uint32_t	res	=FAudio_CreateSourceVoice(
		pFA, &pFASV, &wfx, 0, 1.0f, pCB, NULL, NULL);
	if(res)
	{
		printf("Error creating source voice for %s\n", szPath);
		return	false;
	}

	//init emitter
	memset(&pCur->mEmitter, 0, sizeof(F3DAUDIO_EMITTER));

	//always mono for 3d sounds
	pCur->mEmitter.OrientFront.z	=1;
	pCur->mEmitter.OrientTop.y		=1;
	pCur->mEmitter.ChannelCount		=1;

	//init dsp stuff
	memset(&pCur->mDSP, 0, sizeof(F3DAUDIO_DSP_SETTINGS));

	pCur->mDSP.pMatrixCoefficients	=malloc(sizeof(float) * numChannels);
	pCur->mDSP.SrcChannelCount		=1;	//default
	pCur->mDSP.DstChannelCount		=numChannels;

	//for the callbacks
	pAB->pContext	=&sSoundFX[sNumSFX];
	
	pAB->AudioBytes	=tw.h.Subchunk2Size;
	pAB->pAudioData	=malloc(tw.h.Subchunk2Size);

	if(tw.sampFmt == TW_INT16)
	{
		iRes	=tinywav_read_16(&tw, (void *)pAB->pAudioData, tw.numFramesInHeader);
	}
	else
	{
		iRes	=tinywav_read_f(&tw, (void *)pAB->pAudioData, tw.numFramesInHeader);
	}
	if(iRes <= 0)
	{
		printf("Error reading data for %s\n", szPath);
		FAudioVoice_DestroyVoice(pFASV);
		free((void *)pAB->pAudioData);
		return	false;
	}
	tinywav_close_read(&tw);
	
	res	=FAudioSourceVoice_SubmitSourceBuffer(pFASV, pAB, NULL);
	if(res)
	{
		printf("Error submitting buffer to source voice for %s\n", szPath);
		FAudioVoice_DestroyVoice(pFASV);
		return	false;
	}

	//copy to static array
	pCur->mpSrcVoice	=pFASV;
	strncpy(pCur->mszName, szName, SFX_NAME_LEN);

	sNumSFX++;

	return	true;
}

void	SoundEffect_DestroyAll(void)
{
	for(int i=0;i < sNumSFX;i++)
	{
		//free voice
		FAudioVoice_DestroyVoice(sSoundFX[i].mpSrcVoice);
		sSoundFX[i].mpSrcVoice	=NULL;

		//free buffer data
		free((void *)sSoundFX[i].mBuffer.pAudioData);
		memset(&sSoundFX[i].mBuffer, 0, sizeof(FAudioBuffer));

		//zero out name
		memset(sSoundFX[i].mszName, 0, SFX_NAME_LEN);
	}

	sNumSFX	=0;
}

//return how many sounds loaded
//numChannels is how many speakers on the output device
int	SoundEffect_LoadAllInPath(const char *szDir, FAudio *pFA, uint32_t numChannels)
{
	DIR	*pDir	=opendir(szDir);
	if(pDir == NULL)
	{
		return	0;
	}

	//lazy
	char	pathBuf[256];
	int		count	=0;
	for(;;)
	{
		struct dirent	*pEnt	=readdir(pDir);

		if(pEnt == NULL)
		{
			break;
		}

		struct stat	fileStuff;

		strncpy(pathBuf, szDir, 255);
		strncat(pathBuf, "/", 255);
		strncat(pathBuf, pEnt->d_name, 255);

		int	res	=stat(pathBuf, &fileStuff);
		if(res)
		{
			sPrintErr(res);
			continue;
		}

		//regular file?
		if(S_ISREG(fileStuff.st_mode))
		{
			UT_string	*pFName	=SZ_StripExtension(pEnt->d_name);

			int	idx	=0;
			if(SoundEffect_Create(utstring_body(pFName), pathBuf, pFA, numChannels, &idx))
			{
				count++;
			}
			utstring_free(pFName);
		}
	}
	return	count;
}


const char	*SoundEffect_GetIndexName(int idx)
{
	if(idx < 0 || idx >= sNumSFX)
	{
		return	NULL;
	}

	return	sSoundFX[idx].mszName;
}

int	SoundEffect_GetSFXCount(void)
{
	return	sNumSFX;
}


bool	SoundEffectPlayIdx(int idx, vec3 position)
{
	idx	%=sNumSFX;

	SoundEffect	*pSFX	=&sSoundFX[idx];

	if(pSFX->mbPlaying)
	{
		return	false;
	}
	
	uint32_t	res;
	if(!pSFX->mbPlayedOnce)
	{
		res	=FAudioSourceVoice_Start(pSFX->mpSrcVoice, 0, FAUDIO_COMMIT_NOW);
	}
	else
	{
		res	=FAudioSourceVoice_SubmitSourceBuffer(pSFX->mpSrcVoice, &pSFX->mBuffer, NULL);
	}

	pSFX->mbPlaying		=true;
	pSFX->mbPlayedOnce	=true;

	pSFX->mEmitter.Position.x	=position[0];
	pSFX->mEmitter.Position.y	=position[1];
	pSFX->mEmitter.Position.z	=position[2];

	return	(res == 0);
}

//think about returning indexes to save on string compares?
bool	SoundEffectPlay(const char *szName, vec3 position)
{
	int	idx	=sGetIndex(szName);
	if(idx == -1)
	{
		return	false;
	}
	return	SoundEffectPlayIdx(idx, position);
}


void	SoundEffect_UpdateEmitters(F3DAUDIO_HANDLE h3d,
			const F3DAUDIO_LISTENER *pList, FAudioVoice *pMaster)
{
	return;

	//none of this crap works right now and I don't have time to fix it
	for(int i=0;i < sNumSFX;i++)
	{
		SoundEffect	*pSFX	=&sSoundFX[i];

		if(!pSFX->mbPlaying)
		{
			continue;
		}

		F3DAudioCalculate(h3d, pList, &pSFX->mEmitter,
				F3DAUDIO_CALCULATE_MATRIX | F3DAUDIO_CALCULATE_DOPPLER |
				F3DAUDIO_CALCULATE_LPF_DIRECT | F3DAUDIO_CALCULATE_REVERB,
				&pSFX->mDSP);

		FAudioVoice_SetOutputMatrix(pSFX->mpSrcVoice, pMaster, 1,
			pSFX->mDSP.DstChannelCount,
			pSFX->mDSP.pMatrixCoefficients, FAUDIO_COMMIT_NOW);

		FAudioSourceVoice_SetFrequencyRatio(pSFX->mpSrcVoice,
			pSFX->mDSP.DopplerFactor, FAUDIO_COMMIT_NOW);
	}
}


//callbacks, not doing anything with these yet
static void FAUDIOCALL sOnBufferEnd(FAudioVoiceCallback *pCB, void *pContext)
{
	SoundEffect	*pSFX	=(SoundEffect *)pContext;

//	printf("Buffer end!\n");

	pSFX->mbPlaying	=false;
}

static void FAUDIOCALL sOnBufferStart(FAudioVoiceCallback *pCB, void *pContext)
{
}

static void FAUDIOCALL sOnLoopEnd(FAudioVoiceCallback *pCB, void *pContext)
{
}

static void FAUDIOCALL sOnStreamEnd(FAudioVoiceCallback *pCB)
{
	printf("Stream end!\n");
}

static void FAUDIOCALL sOnVoiceError(FAudioVoiceCallback *pCB, void *pContext, uint32_t Error)
{
}

static void FAUDIOCALL sOnVoiceProcessingPassStart(FAudioVoiceCallback *pCB, uint32_t BytesRequired)
{
}

static void FAUDIOCALL sOnVoiceProcessingPassEnd(FAudioVoiceCallback *pCB)
{
}


//other statics
static void	sPrintErr(int err)
{
	if(err == 0)
	{
		return;	//success!
	}

	switch(err)
	{
		case	EACCES:
			printf("EACCESS\n");
			break;
		case	EIO:
			printf("EI EI O (EIO)\n");
			break;
		case	ELOOP:
			printf("ELOOP\n");
			break;
		case	ENAMETOOLONG:
			printf("ENAMETOOLONG\n");
			break;
		case	ENOENT:
			printf("ENOENT\n");
			break;
		case	ENOTDIR:
			printf("ENOTDIR\n");
			break;
		case	EOVERFLOW:
			printf("EOVERFLOW\n");
			break;
		default:
			printf("This: %d happened ¯\\_(ツ)_/¯\n", err);
	}
}

static int	sGetIndex(const char *szName)
{
	assert(szName != NULL);

	for(int i=0;i < sNumSFX;i++)
	{
		if(0 == strncmp(sSoundFX[i].mszName, szName, SFX_NAME_LEN))
		{
			return	i;
		}
	}
	return	-1;
}

static bool	sCheckExisting(const char *szName)
{
	return	(sGetIndex(szName) != -1);
}