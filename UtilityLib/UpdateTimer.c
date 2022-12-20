#include	<stdint.h>
#include	<stdbool.h>
#include	<stdlib.h>
#include	<assert.h>
#include	<threads.h>
#include	<time.h>
#include	<math.h>
#include	<sys/param.h>


typedef	struct	UpdateTimer_t
{
	//time stats
	struct timespec	mLastTimeStamp;	//previous time
	struct timespec	mTimeNow;		//time as of last Stamp() call
	uint64_t		mMaxDelta;		//biggest allowed deltatime

	//time step related
	bool		mbFixedStep;		//use a fixed time stamp for game/physics updates?
	bool		mbSpendRemainder;	//spend or roll over the small remainder?
	uint64_t	mStep;				//fixed step in tics
	uint64_t	mFullUpdateTime;	//counts down over the updates for this frame
}	UpdateTimer;


static uint64_t Delta(UpdateTimer *pUT)
{
	uint64_t	delta	=pUT->mTimeNow.tv_nsec - pUT->mLastTimeStamp.tv_nsec;

	//will this ever overflow?
	assert(delta < UINT64_MAX);

	return	MIN(delta, pUT->mMaxDelta);
}


static float TicsToSeconds(uint64_t tics)
{
	double	secs	=tics / 1000000000.0;

	return	(float)secs;
}


static float TicsToMilliSeconds(uint64_t tics)
{
	double	msecs	=tics / 1000000.0;

	return	(float)msecs;
}


static uint64_t SecondsToTics(float seconds)
{
	uint64_t	tics	=(double)seconds * 1000000000.0;

	return	tics;
}


static long MilliSecondsToTics(float milliSeconds)
{
	uint64_t	msFreq	=(double)milliSeconds * 1000000.0;

	return	msFreq;
}


UpdateTimer	*UpdateTimer_Create(bool bFixed, bool bSpendRemainder)
{
	UpdateTimer	*pRet	=malloc(sizeof(UpdateTimer));

	pRet->mbFixedStep		=bFixed;
	pRet->mbSpendRemainder	=bSpendRemainder;

	pRet->mMaxDelta	=SecondsToTics(0.1f);	//default

	return	pRet;
}


uint64_t UpdateTimer_GetRenderUpdateDeltaTics(UpdateTimer *pUT)
{
	uint64_t	tics	=Delta(pUT);
	if(pUT->mbFixedStep && !pUT->mbSpendRemainder)
	{
		//subtract remainder
		//return	tics - mFullUpdateTime;
		return	tics;
	}
	else
	{
		return	tics;
	}
}


float UpdateTimer_GetRenderUpdateDeltaSeconds(UpdateTimer *pUT)
{
	return	TicsToSeconds(UpdateTimer_GetRenderUpdateDeltaTics(pUT));
} 


float UpdateTimer_GetRenderUpdateDeltaMilliSeconds(UpdateTimer *pUT)
{
	return	TicsToMilliSeconds(UpdateTimer_GetRenderUpdateDeltaTics(pUT));
} 


uint64_t UpdateTimer_GetUpdateDeltaTics(UpdateTimer *pUT)
{
	if(pUT->mbFixedStep)
	{
		if(pUT->mFullUpdateTime >= pUT->mStep)
		{
			return	pUT->mStep;
		}

		if(pUT->mbSpendRemainder && pUT->mFullUpdateTime > 0)
		{
			return	pUT->mFullUpdateTime;
		}
		return	0L;
	}
	else
	{
		return	pUT->mFullUpdateTime;
	}
}


float UpdateTimer_GetUpdateDeltaSeconds(UpdateTimer *pUT)
{
	uint64_t	tics	=UpdateTimer_GetUpdateDeltaTics(pUT);

	return	TicsToSeconds(tics);
}


float UpdateTimer_GetUpdateDeltaMilliSeconds(UpdateTimer *pUT)
{
	uint64_t	tics	=UpdateTimer_GetUpdateDeltaTics(pUT);

	return	TicsToMilliSeconds(tics);
}


void UpdateTimer_UpdateDone(UpdateTimer *pUT)
{
	if(pUT->mbFixedStep)
	{
		if(pUT->mFullUpdateTime >= pUT->mStep)
		{
			pUT->mFullUpdateTime	-=pUT->mStep;
		}
		else
		{
			if(pUT->mbSpendRemainder)
			{
				pUT->mFullUpdateTime	=0;
			}
		}
	}
	else
	{
		pUT->mFullUpdateTime	=0;
	}
}


//allows a user preference on the maximum
//deltatime allowed (for game stability)
void UpdateTimer_SetMaxDeltaTics(UpdateTimer *pUT, uint64_t tics)
{
	pUT->mMaxDelta	=tics;
}


void UpdateTimer_SetMaxDeltaMilliSeconds(UpdateTimer *pUT, float milliSeconds)
{
	pUT->mMaxDelta	=MilliSecondsToTics(milliSeconds);
}


void UpdateTimer_SetMaxDeltaSeconds(UpdateTimer *pUT, float seconds)
{
	pUT->mMaxDelta	=SecondsToTics(seconds);
}


void UpdateTimer_SetFixedTimeStepTics(UpdateTimer *pUT, long tics)
{
	pUT->mStep	=tics;
}


void UpdateTimer_SetFixedTimeStepSeconds(UpdateTimer *pUT, float seconds)
{
	pUT->mStep	=SecondsToTics(seconds);
}


void UpdateTimer_SetFixedTimeStepMilliSeconds(UpdateTimer *pUT, float milliSeconds)
{
	pUT->mStep	=MilliSecondsToTics(milliSeconds);
}


void UpdateTimer_Stamp(UpdateTimer *pUT)
{
	pUT->mLastTimeStamp	=pUT->mTimeNow;

	clock_gettime(CLOCK_REALTIME, &pUT->mTimeNow);

	pUT->mFullUpdateTime	+=Delta(pUT);
}


uint64_t UpdateTimer_DeltaTics(UpdateTimer *pUT)
{
	return	Delta(pUT);
}


//deltas are reasonably safe in floats
float UpdateTimer_DeltaSeconds(UpdateTimer *pUT)
{
	uint64_t	tics	=Delta(pUT);

	return	TicsToSeconds(tics);
}


//deltas are reasonably safe in floats
float UpdateTimer_DeltaMilliSeconds(UpdateTimer *pUT)
{
	uint64_t	tics	=Delta(pUT);

	return	TicsToMilliSeconds(tics);
}

int main(int argc, char const *argv[])
{
	/* code */
	return 0;
}
