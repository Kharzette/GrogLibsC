#include	<stdint.h>
#include	<stdbool.h>
#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>
#include	<threads.h>
#include	<time.h>
#include	<math.h>
#include	<sys/param.h>


//This time helper works by measuring the time a "frame" took
//and dishing out slices of time for use by updates.
//
//Update is for things like physics that use discreet slices to
//approximate something continuous, like the parabola of a jump arc.
//
//RenderUpdate is always an entire frame's time with no subdivision.
//This can be used for things in constant motion like a moving UV set
//for flowing lava, or a blinking cursor.
//
//A typical game loop would look like:
//while(bRunning)
//{
//	UpdateTimer_Stamp(pUT);
//	while(UpdateTimer_GetUpdateDeltaSeconds(pUT) > 0.0f)
//	{
//		HandleInputs();
//		UpdateGame();	
//		UpdateTimer_UpdateDone(pUT);
//	}
//	float	dt	=UpdateTimer_GetRenderUpdateDeltaSeconds(pUT);
//	RenderUpdate(dt);
//	DrawStuff();
//	Present()
//}
typedef	struct	UpdateTimer_t
{
	//time stats
	struct timespec	mLastTimeStamp;	//previous time
	struct timespec	mTimeNow;		//time as of last Stamp() call
	int64_t			mMaxDelta;		//biggest allowed deltatime

	//time step related
	bool		mbFixedStep;		//use a fixed time stamp for game/physics updates?
	bool		mbSpendRemainder;	//spend or roll over the small remainder?
	int64_t		mStep;				//fixed step in tics
	int64_t		mFullUpdateTime;	//counts down over the updates for this frame
}	UpdateTimer;


//forward declarations of stuff at the bottom
static uint64_t Delta(const UpdateTimer *pUT);
static float TicsToSeconds(uint64_t tics);
static float TicsToMilliSeconds(uint64_t tics);
static uint64_t SecondsToTics(float seconds);
static long MilliSecondsToTics(float milliSeconds);


//Fixed doles out time in predictable slices, nonfixed the delta fluctuates.
//Spend remainder is only valid for fixed mode and consumes the remaining time
//after the fixed slices have been consumed.
UpdateTimer	*UpdateTimer_Create(bool bFixed, bool bSpendRemainder)
{
	UpdateTimer	*pRet	=malloc(sizeof(UpdateTimer));

	memset(pRet, 0, sizeof(UpdateTimer));

	pRet->mbFixedStep		=bFixed;
	pRet->mbSpendRemainder	=bSpendRemainder;

	pRet->mMaxDelta	=SecondsToTics(0.1f);			//default
	pRet->mStep		=MilliSecondsToTics(16.6666f);	//default 60hz

	//get a first time reading
	clock_gettime(CLOCK_REALTIME, &pRet->mTimeNow);

	return	pRet;
}


//Mark the top of an update loop by getting the time elapsed since
//the last time this was called.
void UpdateTimer_Stamp(UpdateTimer *pUT)
{
	pUT->mLastTimeStamp	=pUT->mTimeNow;

	clock_gettime(CLOCK_REALTIME, &pUT->mTimeNow);

	pUT->mFullUpdateTime	+=Delta(pUT);
}


//this returns a slice of time for updating
uint64_t UpdateTimer_GetUpdateDeltaTics(const UpdateTimer *pUT)
{
	if(pUT->mbFixedStep)
	{
		if(pUT->mFullUpdateTime >= pUT->mStep)
		{
			return	pUT->mStep;
		}

		//if spend remainder is on, return a smaller timeslice to
		//use up all of the time.  This is contrary to the thinking
		//behind fixed time stepping, so use with caution
		if(pUT->mbSpendRemainder && pUT->mFullUpdateTime > 0)
		{
			return	pUT->mFullUpdateTime;
		}

		//Here there will likely be a small piece of time left.
		//This time can be left to process the next frame, or
		//spent as a fixed timeslice that will cut into next
		//frame's time a bit.

		//I think maybe the best thing to do is see if the remainder
		//is nearer to a fixed time slice than zero, and if so, spend
		//a fixed time slice and go negative, otherwise spend it next frame
		int64_t	halfStep	=pUT->mStep / 2;
		if(pUT->mFullUpdateTime >= halfStep)
		{
			return	pUT->mStep;
		}
		return	0L;
	}
	else
	{
		return	pUT->mFullUpdateTime;
	}
}


//render updates just return the entire deltatime in one slice
uint64_t UpdateTimer_GetRenderUpdateDeltaTics(const UpdateTimer *pUT)
{
	return	Delta(pUT);
}


//This subtracts the timeslice from FullUpdateTime, so should be
//done at the bottom of an update loop
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
			else
			{
				//see if a fixed step was spent, sending FullUpdate negative
				int64_t	halfStep	=pUT->mStep / 2;
				if(pUT->mFullUpdateTime >= halfStep)
				{
					//go negative
					pUT->mFullUpdateTime	-=pUT->mStep;
				}
			}
		}
	}
	else
	{
		pUT->mFullUpdateTime	=0;
	}

	assert(pUT->mFullUpdateTime <= pUT->mMaxDelta);
}


//allows a user preference on the maximum
//deltatime allowed (for game stability)
void UpdateTimer_SetMaxDeltaTics(UpdateTimer *pUT, uint64_t tics)
{
	pUT->mMaxDelta	=tics;
}


//user setting for the fixed timeslice amount used
void UpdateTimer_SetFixedTimeStepTics(UpdateTimer *pUT, uint64_t tics)
{
	pUT->mStep	=tics;
}


//returns the time amount between this frame and last
static uint64_t Delta(const UpdateTimer *pUT)
{
	uint64_t	delta	=pUT->mTimeNow.tv_nsec - pUT->mLastTimeStamp.tv_nsec;

	//will this ever overflow?
	//the nsecs are longs, so makes me nervous
	assert(delta < UINT64_MAX);

	return	MIN(delta, pUT->mMaxDelta);
}


//helper functions to return delta times in convenient units
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

float UpdateTimer_GetRenderUpdateDeltaSeconds(UpdateTimer *pUT)
{
	return	TicsToSeconds(UpdateTimer_GetRenderUpdateDeltaTics(pUT));
}

float UpdateTimer_GetRenderUpdateDeltaMilliSeconds(UpdateTimer *pUT)
{
	return	TicsToMilliSeconds(UpdateTimer_GetRenderUpdateDeltaTics(pUT));
} 


//helper for setting time amounts in convenient units
void UpdateTimer_SetMaxDeltaMilliSeconds(UpdateTimer *pUT, float milliSeconds)
{
	pUT->mMaxDelta	=MilliSecondsToTics(milliSeconds);
}

void UpdateTimer_SetMaxDeltaSeconds(UpdateTimer *pUT, float seconds)
{
	pUT->mMaxDelta	=SecondsToTics(seconds);
}

void UpdateTimer_SetFixedTimeStepSeconds(UpdateTimer *pUT, float seconds)
{
	pUT->mStep	=SecondsToTics(seconds);
}

void UpdateTimer_SetFixedTimeStepMilliSeconds(UpdateTimer *pUT, float milliSeconds)
{
	pUT->mStep	=MilliSecondsToTics(milliSeconds);
}


//static stuff to help with unit conversion
//from here down
static uint64_t UpdateTimer_DeltaTics(UpdateTimer *pUT)
{
	return	Delta(pUT);
}

//deltas are reasonably safe in floats
static float UpdateTimer_DeltaSeconds(UpdateTimer *pUT)
{
	uint64_t	tics	=Delta(pUT);

	return	TicsToSeconds(tics);
}

//deltas are reasonably safe in floats
static float UpdateTimer_DeltaMilliSeconds(UpdateTimer *pUT)
{
	uint64_t	tics	=Delta(pUT);

	return	TicsToMilliSeconds(tics);
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
