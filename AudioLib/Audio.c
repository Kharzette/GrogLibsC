#include	<stdint.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<assert.h>
#include	<string.h>
#include	<FAudio.h>
#include	<F3DAudio.h>
#include	"tinywav/tinywav.h"


//track the vitals of each audio device
struct DeviceStuff
{
	uint32_t	mIndex, mNumChannels, mSampleRate, mChanMask;
};


//wierd 16 bit strings
//I fought with this for hours trying to go through proper channels
//in the end I gave up and just wrote a byte skip
void convert_goofy(const uint16_t *pGoofy, char *pConverted, int len)
{
	//NONE of the usual string conversions work on whatever this is
	for(int i=0;i < len;i++)
	{
		pConverted[i]	=pGoofy[i] & 0xFF;

		if(!pConverted[i])
		{
			break;
		}
	}
}


void	PrintSpeakerStuff(uint32_t channelMask)
{
	printf("Speakers:");

	if(channelMask & SPEAKER_FRONT_LEFT)
	{
		printf(" SPEAKER_FRONT_LEFT ");
	}
	if(channelMask & SPEAKER_FRONT_RIGHT)
	{
		printf(" SPEAKER_FRONT_RIGHT ");
	}
	if(channelMask & SPEAKER_FRONT_CENTER)
	{
		printf(" SPEAKER_FRONT_CENTER ");
	}
	if(channelMask & SPEAKER_LOW_FREQUENCY)
	{
		printf(" SPEAKER_LOW_FREQUENCY ");
	}
	if(channelMask & SPEAKER_BACK_LEFT)
	{
		printf(" SPEAKER_BACK_LEFT ");
	}
	if(channelMask & SPEAKER_BACK_RIGHT)
	{
		printf(" SPEAKER_BACK_RIGHT ");
	}
	if(channelMask & SPEAKER_FRONT_LEFT_OF_CENTER)
	{
		printf(" SPEAKER_FRONT_LEFT_OF_CENTER ");
	}
	if(channelMask & SPEAKER_FRONT_RIGHT_OF_CENTER)
	{
		printf(" SPEAKER_FRONT_RIGHT_OF_CENTER ");
	}
	if(channelMask & SPEAKER_BACK_CENTER)
	{
		printf(" SPEAKER_BACK_CENTER ");
	}
	if(channelMask & SPEAKER_SIDE_LEFT)
	{
		printf(" SPEAKER_SIDE_LEFT ");
	}
	if(channelMask & SPEAKER_SIDE_RIGHT)
	{
		printf(" SPEAKER_SIDE_RIGHT ");
	}
	if(channelMask & SPEAKER_TOP_CENTER)
	{
		printf(" SPEAKER_TOP_CENTER ");
	}
	if(channelMask & SPEAKER_TOP_FRONT_LEFT)
	{
		printf(" SPEAKER_TOP_FRONT_LEFT ");
	}
	if(channelMask & SPEAKER_TOP_FRONT_CENTER)
	{
		printf(" SPEAKER_TOP_FRONT_CENTER ");
	}
	if(channelMask & SPEAKER_TOP_FRONT_RIGHT)
	{
		printf(" SPEAKER_TOP_FRONT_RIGHT ");
	}
	if(channelMask & SPEAKER_TOP_BACK_LEFT)
	{
		printf(" SPEAKER_TOP_BACK_LEFT ");
	}
	if(channelMask & SPEAKER_TOP_BACK_CENTER)
	{
		printf(" SPEAKER_TOP_BACK_CENTER ");
	}
	if(channelMask & SPEAKER_TOP_BACK_RIGHT)
	{
		printf(" SPEAKER_TOP_BACK_RIGHT ");
	}
	printf("\n");
}


static void FAUDIOCALL OnBufferEnd(FAudioVoiceCallback *pCB, void *pContext)
{
}

static void FAUDIOCALL OnBufferStart(FAudioVoiceCallback *This, void *pContext)
{
}

static void FAUDIOCALL OnLoopEnd(FAudioVoiceCallback *This, void *pContext)
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


int	main(void)
{
	printf("Blort!  Testing audio stuffs...\n");

	FAudio	*pFA;

	uint32_t	res	=FAudioCreate(&pFA, 0, FAUDIO_DEFAULT_PROCESSOR);

	uint32_t	deviceCount;

	res	=FAudio_GetDeviceCount(pFA, &deviceCount);

	struct	DeviceStuff	devices[deviceCount];

	for(int i=0;i < deviceCount;i++)
	{
		FAudioDeviceDetails	pFADD;

		FAudio_GetDeviceDetails(pFA, i, &pFADD);

		printf("Device: %d with Role: ", i);

		switch(pFADD.Role)
		{
			case	FAudioNotDefaultDevice:
				printf("FAudioNotDefaultDevice");
				break;
			case	FAudioDefaultConsoleDevice:
				printf("FAudioDefaultConsoleDevice");
				break;
			case	FAudioDefaultMultimediaDevice:
				printf("FAudioDefaultMultimediaDevice");
				break;
			case	FAudioDefaultCommunicationsDevice:
				printf("FAudioDefaultCommunicationsDevice");
				break;
			case	FAudioDefaultGameDevice:
				printf("FAudioDefaultGameDevice");
				break;
			case	FAudioGlobalDefaultDevice:
				printf("FAudioGlobalDefaultDevice");
				break;
			case	FAudioInvalidDeviceRole:
				printf("FAudioInvalidDeviceRole");
				break;
		}

		char	buf[256];

		convert_goofy(pFADD.DeviceID, buf, 255);
		printf(" and DeviceID: %s", buf);

		convert_goofy(pFADD.DisplayName, buf, 255);
		printf(" and DisplayName: %s\n", buf);

		PrintSpeakerStuff(pFADD.OutputFormat.dwChannelMask);

		printf("Output format-> NumChannels: %d, BitsPerSample %d\n",
			pFADD.OutputFormat.Format.nChannels, pFADD.OutputFormat.Format.wBitsPerSample);

		devices[i].mIndex		=i;
		devices[i].mNumChannels	=pFADD.OutputFormat.Format.nChannels;
		devices[i].mSampleRate	=pFADD.OutputFormat.Format.nSamplesPerSec;
		devices[i].mChanMask	=pFADD.OutputFormat.dwChannelMask;
	}

	printf("\nWhich device would you like to use?\n");

	char	inBuf[32];
	fgets(inBuf, 31, stdin);

	int	chosenIndex	=atoi(inBuf);
	if(chosenIndex < 0 || chosenIndex > deviceCount)
	{
		chosenIndex	=2;	//my default
	}

	FAudioMasteringVoice	*pFAMV;
	res	=FAudio_CreateMasteringVoice(pFA, &pFAMV,
		devices[chosenIndex].mNumChannels,
		devices[chosenIndex].mSampleRate, 0, chosenIndex, NULL);

//	F3DAUDIO_HANDLE	handy3D;

//	F3DAudioInitialize(devices[chosenIndex].mChanMask, 343.0f, handy3D);

	F3DAUDIO_LISTENER	list;

	list.OrientFront.x	=0.0f;
	list.OrientFront.y	=0.0f;
	list.OrientFront.z	=1.0f;

	list.OrientTop.x	=0.0f;
	list.OrientTop.y	=1.0f;
	list.OrientTop.z	=0.0f;

	list.pCone			=NULL;
	list.Position.x		=0.0f;
	list.Position.y		=0.0f;
	list.Position.z		=0.0f;

	list.Velocity	=list.Position;

//	DIR	*pDir	=opendir(".");

	char	pathBuf[256];

	getcwd(pathBuf, 255);

	printf("Current Dir: %s\n", pathBuf);

//	closedir(pDir);


	TinyWav	tw;

	tinywav_open_read(&tw, "AudioLib/LevelX.wav", TW_SPLIT);

	FAudioSourceVoice	*pFASV;

	FAudioWaveFormatEx	wfx;
	wfx.cbSize			=0;
	wfx.nBlockAlign		=tw.numChannels * (tw.h.BitsPerSample / 8);
	wfx.nChannels		=tw.numChannels;
	wfx.nSamplesPerSec	=tw.h.SampleRate;
	wfx.wFormatTag		=tw.h.AudioFormat;
	wfx.wBitsPerSample	=tw.sampFmt * 8;	//the enum is also the size in bytes
	wfx.nAvgBytesPerSec	=tw.h.SampleRate * wfx.nBlockAlign;

	FAudioVoiceCallback	favc	=
	{
		OnBufferEnd,
		OnBufferStart,
		OnLoopEnd,
		OnStreamEnd,
		OnVoiceError,
		OnVoiceProcessingPassEnd,
		OnVoiceProcessingPassStart
	};

	res	=FAudio_CreateSourceVoice(pFA, &pFASV, &wfx, 0, 1.0f, &favc, NULL, NULL);
	assert(!res);

	FAudioBuffer	audBuf;
	memset(&audBuf, 0, sizeof(audBuf));

	if(tw.sampFmt == TW_INT16)
	{
		audBuf.AudioBytes	=tw.h.Subchunk2Size;
		audBuf.pAudioData	=malloc(audBuf.AudioBytes);
		tinywav_read_16(&tw, (void *)audBuf.pAudioData, tw.numFramesInHeader);
	}
	else
	{
		audBuf.AudioBytes	=tw.h.Subchunk2Size;
		audBuf.pAudioData	=malloc(tw.h.Subchunk2Size);
		tinywav_read_f(&tw, (void *)audBuf.pAudioData, tw.numFramesInHeader);
	}


	tinywav_close_read(&tw);

	res	=FAudioSourceVoice_SubmitSourceBuffer(pFASV, &audBuf, NULL);
	assert(!res);

	res	=FAudioSourceVoice_Start(pFASV, 0, FAUDIO_COMMIT_NOW);
	assert(!res);

	res	=FAudio_StartEngine(pFA);
	assert(!res);

	//lazy wait for sound effect to finish
	sleep(10);

	FAudioVoice_DestroyVoice(pFASV);
	FAudioVoice_DestroyVoice(pFAMV);

	free((void *)audBuf.pAudioData);

	FAudio_Release(pFA);
}