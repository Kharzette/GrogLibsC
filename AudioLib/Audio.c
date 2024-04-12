#include	<stdint.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<assert.h>
#include	<string.h>
#include	<FAudio.h>
#include	<F3DAudio.h>
#include	"tinywav/tinywav.h"
#include	"SoundEffect.h"


//track the vitals of each audio device
typedef struct DeviceStuff_t
{
	uint32_t	mIndex, mNumChannels, mSampleRate, mChanMask;
}	DeviceStuff;

typedef struct	Audio_t
{
	FAudio	*mpFA;

	//audio config stuff
	uint32_t	mDeviceCount;
	DeviceStuff	*mpDevices;

	//3d audio
	FAudioMasteringVoice	*mpFAMV;
	F3DAUDIO_LISTENER		mListener;

	//sound effects
	int	mNumSFXLoaded;
}	Audio;


//wierd 16 bit strings
//I fought with this for hours trying to go through proper channels
//in the end I gave up and just wrote a byte skip
__attribute_maybe_unused__
static void convert_goofy(const uint16_t *pGoofy, char *pConverted, int len)
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

static void convert_goofySigned(const int16_t *pGoofy, char *pConverted, int len)
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


Audio	*Audio_Create(int deviceIndex)
{
	Audio	*pRet	=malloc(sizeof(Audio));

	uint32_t	res	=FAudioCreate(&pRet->mpFA, 0, FAUDIO_DEFAULT_PROCESSOR);
	if(res != 0)
	{
		printf("Error creating FAudio!\n");
		free(pRet);
		return	NULL;
	}

	res	=FAudio_GetDeviceCount(pRet->mpFA, &pRet->mDeviceCount);
	if(res != 0)
	{
		printf("Error getting audio device count!\n");
		FAudio_Release(pRet->mpFA);
		free(pRet);
		return	NULL;
	}

	pRet->mpDevices	=malloc(sizeof(DeviceStuff) * pRet->mDeviceCount);

	for(int i=0;i < pRet->mDeviceCount;i++)
	{
		FAudioDeviceDetails	fadd;

		res	=FAudio_GetDeviceDetails(pRet->mpFA, i, &fadd);
		if(res != 0)
		{
			printf("Error getting device details for %d\n", i);
			continue;
		}

		printf("Device: %d with Role: ", i);

		switch(fadd.Role)
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

		convert_goofySigned(fadd.DeviceID, buf, 255);
		printf(" and DeviceID: %s", buf);

		convert_goofySigned(fadd.DisplayName, buf, 255);
		printf(" and DisplayName: %s\n", buf);

		PrintSpeakerStuff(fadd.OutputFormat.dwChannelMask);

		printf("Output format-> NumChannels: %d, BitsPerSample %d\n",
			fadd.OutputFormat.Format.nChannels,
			fadd.OutputFormat.Format.wBitsPerSample);

		pRet->mpDevices[i].mIndex		=i;
		pRet->mpDevices[i].mNumChannels	=fadd.OutputFormat.Format.nChannels;
		pRet->mpDevices[i].mSampleRate	=fadd.OutputFormat.Format.nSamplesPerSec;
		pRet->mpDevices[i].mChanMask	=fadd.OutputFormat.dwChannelMask;
	}

	res	=FAudio_CreateMasteringVoice(pRet->mpFA, &pRet->mpFAMV,
		pRet->mpDevices[deviceIndex].mNumChannels,
		pRet->mpDevices[deviceIndex].mSampleRate, 0, deviceIndex, NULL);

	if(res != 0)
	{
		printf("Error creating mastering voice!\n");
		FAudio_Release(pRet->mpFA);
		free(pRet->mpDevices);
		free(pRet);
		return	NULL;
	}

	pRet->mListener.OrientFront.x	=0.0f;
	pRet->mListener.OrientFront.y	=0.0f;
	pRet->mListener.OrientFront.z	=1.0f;

	pRet->mListener.OrientTop.x	=0.0f;
	pRet->mListener.OrientTop.y	=1.0f;
	pRet->mListener.OrientTop.z	=0.0f;

	pRet->mListener.pCone			=NULL;
	pRet->mListener.Position.x		=0.0f;
	pRet->mListener.Position.y		=0.0f;
	pRet->mListener.Position.z		=0.0f;

	pRet->mListener.Velocity	=pRet->mListener.Position;
	
	pRet->mNumSFXLoaded	=SoundEffectLoadAllInPath("Audio", pRet->mpFA);

	if(pRet->mNumSFXLoaded <= 0)
	{
		printf("Failed to load sounds...\n");
		return	0;
	}

	printf("Loaded up %d sound effects.\n", pRet->mNumSFXLoaded);

	res	=FAudio_StartEngine(pRet->mpFA);
	if(res != 0)
	{
		printf("Error creating mastering voice!\n");
		FAudio_Release(pRet->mpFA);
		free(pRet->mpDevices);
		free(pRet);
		return	NULL;
	}

	return	pRet;
}

Audio	*Audio_CreateInteractive(void)
{
	Audio	*pRet	=malloc(sizeof(Audio));

	uint32_t	res	=FAudioCreate(&pRet->mpFA, 0, FAUDIO_DEFAULT_PROCESSOR);
	if(res != 0)
	{
		printf("Error creating FAudio!\n");
		free(pRet);
		return	NULL;
	}

	res	=FAudio_GetDeviceCount(pRet->mpFA, &pRet->mDeviceCount);
	if(res != 0)
	{
		printf("Error getting audio device count!\n");
		FAudio_Release(pRet->mpFA);
		free(pRet);
		return	NULL;
	}

	pRet->mpDevices	=malloc(sizeof(DeviceStuff) * pRet->mDeviceCount);

	for(int i=0;i < pRet->mDeviceCount;i++)
	{
		FAudioDeviceDetails	fadd;

		res	=FAudio_GetDeviceDetails(pRet->mpFA, i, &fadd);
		if(res != 0)
		{
			printf("Error getting device details for %d\n", i);
			continue;
		}

		printf("Device: %d with Role: ", i);

		switch(fadd.Role)
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

		convert_goofySigned(fadd.DeviceID, buf, 255);
		printf(" and DeviceID: %s", buf);

		convert_goofySigned(fadd.DisplayName, buf, 255);
		printf(" and DisplayName: %s\n", buf);

		PrintSpeakerStuff(fadd.OutputFormat.dwChannelMask);

		printf("Output format-> NumChannels: %d, BitsPerSample %d\n",
			fadd.OutputFormat.Format.nChannels,
			fadd.OutputFormat.Format.wBitsPerSample);

		pRet->mpDevices[i].mIndex		=i;
		pRet->mpDevices[i].mNumChannels	=fadd.OutputFormat.Format.nChannels;
		pRet->mpDevices[i].mSampleRate	=fadd.OutputFormat.Format.nSamplesPerSec;
		pRet->mpDevices[i].mChanMask	=fadd.OutputFormat.dwChannelMask;
	}

	printf("\nWhich device would you like to use?\n");

	char	inBuf[32];
	fgets(inBuf, 31, stdin);

	int	chosenIndex	=atoi(inBuf);
	if(chosenIndex < 0 || chosenIndex > pRet->mDeviceCount)
	{
		chosenIndex	=0;	//default
	}

	res	=FAudio_CreateMasteringVoice(pRet->mpFA, &pRet->mpFAMV,
		pRet->mpDevices[chosenIndex].mNumChannels,
		pRet->mpDevices[chosenIndex].mSampleRate, 0, chosenIndex, NULL);

	if(res != 0)
	{
		printf("Error creating mastering voice!\n");
		FAudio_Release(pRet->mpFA);
		free(pRet->mpDevices);
		free(pRet);
		return	NULL;
	}

	pRet->mListener.OrientFront.x	=0.0f;
	pRet->mListener.OrientFront.y	=0.0f;
	pRet->mListener.OrientFront.z	=1.0f;

	pRet->mListener.OrientTop.x	=0.0f;
	pRet->mListener.OrientTop.y	=1.0f;
	pRet->mListener.OrientTop.z	=0.0f;

	pRet->mListener.pCone			=NULL;
	pRet->mListener.Position.x		=0.0f;
	pRet->mListener.Position.y		=0.0f;
	pRet->mListener.Position.z		=0.0f;

	pRet->mListener.Velocity	=pRet->mListener.Position;
	
	pRet->mNumSFXLoaded	=SoundEffectLoadAllInPath("Audio", pRet->mpFA);

	if(pRet->mNumSFXLoaded <= 0)
	{
		printf("Failed to load sounds...\n");
		return	0;
	}

	printf("Loaded up %d sound effects.\n", pRet->mNumSFXLoaded);

	res	=FAudio_StartEngine(pRet->mpFA);
	if(res != 0)
	{
		printf("Error creating mastering voice!\n");
		FAudio_Release(pRet->mpFA);
		free(pRet->mpDevices);
		free(pRet);
		return	NULL;
	}

	return	pRet;
}


int	main(void)
{
	printf("Blort!  Testing audio stuffs...\n");

	FAudio	*pFA;

	uint32_t	res	=FAudioCreate(&pFA, 0, FAUDIO_DEFAULT_PROCESSOR);

	uint32_t	deviceCount;

	res	=FAudio_GetDeviceCount(pFA, &deviceCount);

	DeviceStuff	devices[deviceCount];

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

		convert_goofySigned(pFADD.DeviceID, buf, 255);
		printf(" and DeviceID: %s", buf);

		convert_goofySigned(pFADD.DisplayName, buf, 255);
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

	char	pathBuf[256];

	getcwd(pathBuf, 255);

	printf("Current Dir: %s\n", pathBuf);

	int	numLoaded	=SoundEffectLoadAllInPath("Audio", pFA);

	if(numLoaded <= 0)
	{
		printf("Failed to load sounds...\n");
		return	0;
	}

	printf("Loaded up %d sound effects.\n", numLoaded);

	res	=FAudio_StartEngine(pFA);
	assert(!res);

	if(!SoundEffectPlay("JumpIdle"))
	{
		printf("Error playing sound!.\n");
	}

	//lazy wait for sound effect to finish
	sleep(7);

	SoundEffectDestroyAll();

	FAudioVoice_DestroyVoice(pFAMV);

	FAudio_Release(pFA);

	return	1;
}

void	Audio_Destroy(Audio **ppAud)
{
	Audio	*pAud	=*ppAud;

	SoundEffectDestroyAll();

	FAudioVoice_DestroyVoice(pAud->mpFAMV);

	FAudio_Release(pAud->mpFA);

	*ppAud	=NULL;
}