#include	<stdint.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<dirent.h>
#include	<errno.h>
#include	<sys/stat.h>
#include	<assert.h>
#include	<string.h>
#include	<FAudio.h>
#include	"tinywav/tinywav.h"
#include	"SoundEffect.h"


#define	SFX_NAME_LEN	32
#define	SFX_MAX_COUNT	64

//lazy data
typedef struct
{
	FAudioSourceVoice	*mpSrcVoice;
	FAudioBuffer		mBuffer;
	FAudioVoiceCallback	mCB;
	char				mszName[SFX_NAME_LEN];
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


//callbacks, not doing anything with these yet

static void FAUDIOCALL OnBufferEnd(FAudioVoiceCallback *pCB, void *pContext)
{
}

static void FAUDIOCALL OnBufferStart(FAudioVoiceCallback *pCB, void *pContext)
{
}

static void FAUDIOCALL OnLoopEnd(FAudioVoiceCallback *pCB, void *pContext)
{
}

static void FAUDIOCALL OnStreamEnd(FAudioVoiceCallback *pCB)
{
}

static void FAUDIOCALL OnVoiceError(FAudioVoiceCallback *pCB, void *pContext, uint32_t Error)
{
}

static void FAUDIOCALL OnVoiceProcessingPassStart(FAudioVoiceCallback *pCB, uint32_t BytesRequired)
{
}

static void FAUDIOCALL OnVoiceProcessingPassEnd(FAudioVoiceCallback *pCB)
{
}


static void	PrintErr(int err)
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


static int	GetIndex(const char *szName)
{
	assert(szName != NULL);

	for(int i=0;i < sNumSFX;i++)
	{
		if(sSoundFX[i].mszName == NULL)
		{
			continue;
		}
		if(0 == strncmp(sSoundFX[i].mszName, szName, SFX_NAME_LEN))
		{
			return	i;
		}
	}
	return	-1;
}

static bool	CheckExisting(const char *szName)
{
	return	(GetIndex(szName) != -1);
}


void	SoundEffectDestroyAll(void)
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


//loadup a sound effect (non streamed)
bool    SoundEffectCreate(const char *szName, const char *szPath, FAudio *pFA)
{
	if(sNumSFX == SFX_MAX_COUNT)
	{
		return	false;	//too many loaded
	}

	TinyWav	tw;

	//check for existing?
	if(CheckExisting(szName))
	{
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
	pCB->OnBufferEnd				=OnBufferEnd;
	pCB->OnBufferStart				=OnBufferStart;
	pCB->OnLoopEnd					=OnLoopEnd;
	pCB->OnStreamEnd				=OnStreamEnd;
	pCB->OnVoiceError				=OnVoiceError;
	pCB->OnVoiceProcessingPassEnd	=OnVoiceProcessingPassEnd;
	pCB->OnVoiceProcessingPassStart	=OnVoiceProcessingPassStart;
	
	uint32_t	res	=FAudio_CreateSourceVoice(
		pFA, &pFASV, &wfx, 0, 1.0f, pCB, NULL, NULL);
	if(res)
	{
		printf("Error creating source voice for %s\n", szPath);
		return	false;
	}
	

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


void	StripExtension(const char *szFileName, char *extLess, size_t retBufSize)
{
	//copy stream
	strncpy(extLess, szFileName, retBufSize);

	//find last . I guess assumes only one .
	char	*pExt	=strrchr(extLess, '.');

	if(pExt == NULL)
	{
		return;	//no extension
	}

	//lazy null at the .
	*pExt	=0;
}


//return how many sounds loaded
int	SoundEffectLoadAllInPath(const char *szDir, FAudio *pFA)
{
	DIR	*pDir	=opendir(szDir);
	if(pDir == NULL)
	{
		return	0;
	}

	//lazy
	char	nameBuf[256], pathBuf[256];
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
			PrintErr(res);
			continue;
		}

		//regular file?
		if(S_ISREG(fileStuff.st_mode))
		{
			StripExtension(pEnt->d_name, nameBuf, 255);

			if(SoundEffectCreate(nameBuf, pathBuf, pFA))
			{
				count++;
			}
		}
	}
	return	count;
}


//think about returning indexes to save on string compares?
bool	SoundEffectPlay(const char *szName)
{
	int	idx	=GetIndex(szName);
	if(idx == -1)
	{
		return	false;
	}

	uint32_t	res	=FAudioSourceVoice_Start(sSoundFX[idx].mpSrcVoice, 0, FAUDIO_COMMIT_NOW);

	return	(res == 0);
}