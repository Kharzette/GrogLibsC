//This started out as jolt's helloworld.cpp
//
// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#include	<Jolt/Jolt.h>

// Jolt includes
#include	<Jolt/RegisterTypes.h>
#include	<Jolt/Core/Factory.h>
#include	<Jolt/Core/TempAllocator.h>
#include	<Jolt/Core/JobSystemThreadPool.h>
#include	<Jolt/Physics/PhysicsSettings.h>
#include	<Jolt/Physics/PhysicsSystem.h>
#include	<Jolt/Physics/Collision/Shape/BoxShape.h>
#include	<Jolt/Physics/Collision/Shape/SphereShape.h>
#include	<Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include	<Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include	<Jolt/Physics/Body/BodyCreationSettings.h>
#include	<Jolt/Physics/Body/BodyActivationListener.h>
#include	<Jolt/Physics/Character/Character.h>
#include	<Jolt/Physics/Character/CharacterVirtual.h>
#include	<Jolt/Physics/Collision/CollisionCollector.h>
#include	<Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include	<Jolt/Physics/Collision/RayCast.h>
#include	<Jolt/Physics/Collision/CastResult.h>

// STL includes
#include <iostream>
#include <cstdarg>
//#include <thread>

//include the h file to make sure things are extern c'd
#include	"PhysicsStuff.h"


// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

// We're also using STL classes in this example
using namespace std;

// Callback for traces, connect this to your own trace function if you have one
static void TraceImpl(const char *inFMT, ...)
{
	// Format the message
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);
	va_end(list);

	// Print to the TTY
	cout << buffer << endl;
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint inLine)
{
	// Print to the TTY
	cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr? inMessage : "") << endl;

	// Breakpoint
	return true;
};

#endif // JPH_ENABLE_ASSERTS

// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
namespace Layers
{
	static constexpr ObjectLayer	NON_MOVING					=0;
	static constexpr ObjectLayer	MOVING_FRIENDLY				=1;
	static constexpr ObjectLayer	MOVING_ENEMY				=2;
	static constexpr ObjectLayer	MOVING_FRIENDLY_PROJECTILE	=3;
	static constexpr ObjectLayer	MOVING_ENEMY_PROJECTILE		=4;
	static constexpr ObjectLayer	NUM_LAYERS					=5;
};

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
{
public:
	virtual bool	ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
	{
		switch (inObject1)
		{
		case	Layers::NON_MOVING:
		{
			//collide with all movers
			return	(inObject2 >= Layers::MOVING_FRIENDLY && inObject2 <= Layers::MOVING_ENEMY_PROJECTILE);
		}
		case	Layers::MOVING_FRIENDLY:
		{
			//collide with all but friendly projectiles
			return	(inObject2 != Layers::MOVING_FRIENDLY_PROJECTILE);
		}
		case	Layers::MOVING_ENEMY:
		{			
			//collide with all but enemy projectiles
			return	(inObject2 != Layers::MOVING_ENEMY_PROJECTILE);
		}
		case	Layers::MOVING_FRIENDLY_PROJECTILE:
		{
			//collide with non moving and enemy
			return	(inObject2 == Layers::NON_MOVING || inObject2 == Layers::MOVING_ENEMY);
		}
		case	Layers::MOVING_ENEMY_PROJECTILE:
		{
			//collide with non moving and friendly
			return	(inObject2 == Layers::NON_MOVING || inObject2 == Layers::MOVING_FRIENDLY);
		}
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace BroadPhaseLayers
{
	static constexpr BroadPhaseLayer	NON_MOVING(0);
	static constexpr BroadPhaseLayer	MOVING(1);
	static constexpr uint				NUM_LAYERS(2);
};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
{
public:
	BPLayerInterfaceImpl()
	{
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[Layers::NON_MOVING]					=BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::MOVING_FRIENDLY]			=BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::MOVING_ENEMY]				=BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::MOVING_FRIENDLY_PROJECTILE]	=BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::MOVING_ENEMY_PROJECTILE]	=BroadPhaseLayers::MOVING;
	}

	virtual uint	GetNumBroadPhaseLayers() const override
	{
		return	BroadPhaseLayers::NUM_LAYERS;
	}

	virtual BroadPhaseLayer	GetBroadPhaseLayer(ObjectLayer inLayer) const override
	{
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return	mObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char *			GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
	{
		switch ((BroadPhaseLayer::Type)inLayer)
		{
		case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
		case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
		default:													JPH_ASSERT(false); return "INVALID";
		}
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
	BroadPhaseLayer	mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool				ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
	{
		if(inLayer1 == Layers::NON_MOVING)
		{
			return	inLayer2 == BroadPhaseLayers::MOVING;
		}
		else
		{
			return	true;
		}
	}
};

// An example contact listener
class MyContactListener : public ContactListener
{
public:
	// See: ContactListener
	virtual ValidateResult	OnContactValidate(const Body &inBody1, const Body &inBody2, RVec3Arg inBaseOffset, const CollideShapeResult &inCollisionResult) override
	{
//		cout << "Contact validate callback" << endl;

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void			OnContactAdded(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings) override
	{
//		cout << "A contact was added" << endl;
	}

	virtual void			OnContactPersisted(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings) override
	{
//		cout << "A contact was persisted" << endl;
	}

	virtual void			OnContactRemoved(const SubShapeIDPair &inSubShapePair) override
	{
//		cout << "A contact was removed" << endl;
	}
};

// An example activation listener
class MyBodyActivationListener : public BodyActivationListener
{
public:
	virtual void		OnBodyActivated(const BodyID &inBodyID, uint64 inBodyUserData) override
	{
		cout << "A body got activated" << endl;
	}

	virtual void		OnBodyDeactivated(const BodyID &inBodyID, uint64 inBodyUserData) override
	{
		cout << "A body went to sleep" << endl;
	}
};

static void	sCopyVec(const Vec3 *pSrc, vec3 dst)
{
	dst[0]	=pSrc->GetX();
	dst[1]	=pSrc->GetY();
	dst[2]	=pSrc->GetZ();
}

__attribute_maybe_unused__
static void	sCopyvec(const vec3 src, Vec3 *pDst)
{
	pDst->SetX(src[0]);
	pDst->SetY(src[1]);
	pDst->SetZ(src[2]);
}

//the C object for the rest of GrogLibs
typedef struct	PhysicsStuff_t
{
	//this is some kind of wierd static thing
	JPH::Factory	*smpFInstance;

	PhysicsSystem			*mpPhys;
	JobSystemThreadPool		*mpJobs;
	TempAllocatorImpl		*mpTAlloc;

	//layer collision stuff
	//Create mapping table from object layer to broadphase layer
	//Note: As this is an interface, PhysicsSystem will take a
	//reference to this so this instance needs to stay alive!
	//Also have a look at BroadPhaseLayerInterfaceTable
	//or BroadPhaseLayerInterfaceMask for a simpler interface.
	BPLayerInterfaceImpl				mBroadI;

	//Create class that filters object vs broadphase layers
	//Note: As this is an interface, PhysicsSystem will take
	//a reference to this so this instance needs to stay alive!
	//Also have a look at ObjectVsBroadPhaseLayerFilterTable or
	//ObjectVsBroadPhaseLayerFilterMask for a simpler interface.
	ObjectVsBroadPhaseLayerFilterImpl	mObjVsBroadFilter;

	//Create class that filters object vs object layers
	//Note: As this is an interface, PhysicsSystem will take
	//a reference to this so this instance needs to stay alive!
	//Also have a look at ObjectLayerPairFilterTable or
	//ObjectLayerPairFilterMask for a simpler interface.
	//pRet->mpObjI	=new ObjectLayerPairFilterImpl();
	ObjectLayerPairFilterImpl			mObjI;

	//listener stuff
	MyBodyActivationListener	mBodyActivate;
	MyContactListener			mContact;
}	PhysicsStuff;

typedef struct	PhysVCharacter_t
{
	CharacterVirtual	*mpChar;

	Shape		*mpStandingShape;
	Shape		*mpCrouchingShape;

}	PhysVCharacter;

typedef struct	PhysCharacter_t
{
	Character	*mpChar;

	Shape		*mpStandingShape;
	Shape		*mpCrouchingShape;

	uint32_t	mMoveMode;

	float	mRunSpeed;
	float	mWalkSpeed;
	float	mSwimSpeed;
	float	mAirSpeed;	//for midair movement
	float	mFlySpeed;	//for flying mode
	float	mJumpSpeed;

}	PhysCharacter;

PhysicsStuff	*Phys_Create(void)
{
	//should use new I think here?
	PhysicsStuff	*pRet	=new PhysicsStuff();

	//Register allocation hook. In this example we'll just let Jolt
	//use malloc / free but you can override these if you want (see Memory.h).
	//This needs to be done before any other Jolt function is called.
	RegisterDefaultAllocator();

	//Install trace and assert callbacks
	Trace	=TraceImpl;
	JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

	//Create a factory, this class is responsible for creating instances
	//of classes based on their name or hash and is mainly used for
	//deserialization of saved data.
	//It is not directly used in this example but still required.
	pRet->smpFInstance	=Factory::sInstance	=new Factory();

	//Register all physics types with the factory and install their
	//collision handlers with the CollisionDispatch class.
	//If you have your own custom shape types you probably need to register
	//their handlers with the CollisionDispatch before calling this function.
	//If you implement your own default material (PhysicsMaterial::sDefault)
	//make sure to initialize it before this function or else this function
	//will create one for you.
	RegisterTypes();

	//We need a temp allocator for temporary allocations during the physics update. We're
	//pre-allocating 10 MB to avoid having to do allocations during the physics update.
	//B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
	//If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
	//malloc / free.
	pRet->mpTAlloc	=new TempAllocatorImpl(10 * 1024 * 1024);

	//We need a job system that will execute physics jobs on multiple threads. Typically
	//you would implement the JobSystem interface yourself and let Jolt Physics run on top
	//of your own job scheduler. JobSystemThreadPool is an example implementation.
	pRet->mpJobs	=new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers,
						thread::hardware_concurrency() - 1);

	//This is the max amount of rigid bodies that you can add to the
	//physics system. If you try to add more you'll get an error.
	//Note: This value is low because this is a simple test. For a
	//real project use something in the order of 65536.
	const uint	cMaxBodies	=8192;

	//This determines how many mutexes to allocate to protect rigid
	//bodies from concurrent access. Set it to 0 for the default settings.
	const uint	cNumBodyMutexes	=0;

	//This is the max amount of body pairs that can be queued
	//at any time (the broad phase will detect overlapping
	//body pairs based on their bounding boxes and will insert them
	//into a queue for the narrowphase). If you make this buffer
	//too small the queue will fill up and the broad phase jobs will
	//start to do narrow phase work. This is slightly less efficient.
	//Note: This value is low because this is a simple test. For a
	//real project use something in the order of 65536.
	const uint	cMaxBodyPairs	=8192;

	//This is the maximum size of the contact constraint buffer.
	//If more contacts (collisions between bodies) are detected than this
	//number then these contacts will be ignored and bodies will
	//start interpenetrating / fall through the world.
	//Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
	const uint	cMaxContactConstraints	=4096;

	//Now we can create the actual physics system.
	pRet->mpPhys	=new PhysicsSystem();
	pRet->mpPhys->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs,
		cMaxContactConstraints,
		pRet->mBroadI, pRet->mObjVsBroadFilter, pRet->mObjI);

	// A body activation listener gets notified when bodies activate and go to sleep
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	pRet->mpPhys->SetBodyActivationListener(&pRet->mBodyActivate);

	// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	pRet->mpPhys->SetContactListener(&pRet->mContact);

	return	pRet;
}

void	Phys_Destroy(PhysicsStuff **ppPS)
{
	PhysicsStuff	*pPS	=*ppPS;

	//delete temp allocator
	delete	pPS->mpTAlloc;

	//nuke job system
	delete	pPS->mpJobs;

	//nuke phys
	delete	pPS->mpPhys;

	//Unregisters all types with the factory and cleans up the default material
	UnregisterTypes();

	//Destroy the factory
	delete Factory::sInstance;

	Factory::sInstance	=nullptr;

	*ppPS	=NULL;
}


void	Phys_Update(PhysicsStuff *pPS, float secDelta)
{
	pPS->mpPhys->Update(secDelta, 1, pPS->mpTAlloc, pPS->mpJobs);
}


//returns the ID
uint32_t	Phys_CreateAndAddHeightField(PhysicsStuff *pPS,
				const float *pHeights,
				const vec3 org, uint32_t squareSize)
{
	//The main way to interact with the bodies in the physics system
	//is through the body interface. There is a locking and a non-locking
	//variant of this. We're going to use the locking version (even
	//though we're not planning to access bodies from multiple threads)
	BodyInterface	&body_interface	=pPS->mpPhys->GetBodyInterface();

	Vec3Arg	VOrg(org[0], org[1], org[2]);
	Vec3Arg	VScale(1,1,1);

	//I truly despise C++, why do I have to cast all this crap!?
	HeightFieldShapeSettings	hfss((const float *)pHeights,
			(Vec3Arg)VOrg, (Vec3Arg)VScale, squareSize);

	//A ref counted object on the stack (base class RefTarget) should
	//be marked as such to prevent it from being freed when its reference
	//count goes to 0.
	hfss.SetEmbedded();

	//Create the shape
	ShapeSettings::ShapeResult	hfSR	=hfss.Create();

	//We don't expect an error here, but you can check result for
	//HasError() / GetError()
	ShapeRefC	hfS	=hfSR.Get();

	//Create the settings for the body itself. Note that here you can
	//also set other properties like the restitution / friction.
	BodyCreationSettings	hfb_settings(hfS, VOrg,
			Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);

	//Create the actual rigid body
	//Note that if we run out of bodies this can return nullptr
	Body	*pHFB	=body_interface.CreateBody(hfb_settings);

	assert(pHFB);

	//Add it to the world
	body_interface.AddBody(pHFB->GetID(), EActivation::DontActivate);

	return	pHFB->GetID().GetIndexAndSequenceNumber();
}


PhysCharacter	*Phys_CreateCharacter(PhysicsStuff *pPS,
	float radius, float height,
	const vec3 org, uint16_t layer)
{
	PhysCharacter	*pRet	=(PhysCharacter *)malloc(sizeof(PhysCharacter));

	memset(pRet, 0, sizeof(PhysCharacter));

	//set defaults TODO: let user set these
	pRet->mRunSpeed		=10.0f;
	pRet->mWalkSpeed	=4.0f;
	pRet->mAirSpeed		=4.0f;
	pRet->mFlySpeed		=30.0f;
	pRet->mSwimSpeed	=6.0f;
	pRet->mJumpSpeed	=5.0f;

	pRet->mMoveMode	=MOVE_RUN;

	pRet->mpStandingShape	=new CapsuleShape(height * 0.5f, radius);
	pRet->mpCrouchingShape	=new CapsuleShape(height * 0.25f, radius);

	Vec3Arg	VOrg(org[0], org[1], org[2]);

	Ref<CharacterSettings>	cs	=new CharacterSettings();

	cs->mLayer				=layer;
	cs->mMaxSlopeAngle		=DegreesToRadians(50.0f);
	cs->mShape				=pRet->mpStandingShape;
	cs->mSupportingVolume	=Plane(Vec3::sAxisY(), -(height - radius));

	pRet->mpChar	=new Character(cs, VOrg, Quat::sIdentity(), 0, pPS->mpPhys);

	pRet->mpChar->AddToPhysicsSystem();

	return	pRet;
}

PhysVCharacter	*Phys_CreateVCharacter(PhysicsStuff *pPS,
	float radius, float height,
	const vec3 org)
{
	PhysVCharacter	*pRet	=(PhysVCharacter *)malloc(sizeof(PhysVCharacter));

	memset(pRet, 0, sizeof(PhysVCharacter));

	pRet->mpStandingShape	=new CapsuleShape(height * 0.5f, radius);
	pRet->mpCrouchingShape	=new CapsuleShape(height * 0.25f, radius);

	Vec3Arg	VOrg(org[0], org[1], org[2]);

	Ref<CharacterVirtualSettings>	cs	=new CharacterVirtualSettings();

	cs->mMaxSlopeAngle		=DegreesToRadians(50.0f);
	cs->mShape				=pRet->mpStandingShape;
	cs->mSupportingVolume	=Plane(Vec3::sAxisY(), -(height - radius));

	pRet->mpChar	=new CharacterVirtual(cs, VOrg, Quat::sIdentity(), pPS->mpPhys);

	return	pRet;
}

void	Phys_CharacterDestroy(PhysCharacter **ppChar)
{
	PhysCharacter	*pChar	=*ppChar;

	pChar->mpChar->RemoveFromPhysicsSystem();

	delete	pChar->mpChar;
	delete	pChar->mpCrouchingShape;
	delete	pChar->mpStandingShape;

	free(pChar);

	*ppChar	=NULL;
}

void	Phys_VCharacterDestroy(PhysVCharacter **ppChar)
{
	PhysVCharacter	*pChar	=*ppChar;

	delete	pChar->mpChar;
	delete	pChar->mpCrouchingShape;
	delete	pChar->mpStandingShape;

	free(pChar);

	*ppChar	=NULL;
}

//move should be a unit vector
void	Phys_CharacterMove(PhysicsStuff *pPS, PhysCharacter *pChar,
	const vec3 move, bool bJump, bool bStanceSwitch, float secDelta)
{
	//Cancel movement in opposite direction of normal when touching
	//something we can't walk up
	Vec3	movDir	={	move[0], move[1], move[2]	};
	
	Character::EGroundState	gstate	=pChar->mpChar->GetGroundState();
	if(gstate == Character::EGroundState::OnSteepGround
		|| gstate == Character::EGroundState::NotSupported)
	{
		Vec3	normal	=pChar->mpChar->GetGroundNormal();
		normal.SetY(0.0f);
		float	dot	=normal.Dot(movDir);
		if(dot < 0.0f)
		{
			movDir	-=(dot * normal) / normal.LengthSq();
		}
	}
	
	//Stance switch
	if(bStanceSwitch)
	{
		float	slop	=1.5f * pPS->mpPhys->GetPhysicsSettings().mPenetrationSlop;
		if(pChar->mpChar->GetShape() == pChar->mpStandingShape)
		{
			pChar->mpChar->SetShape(pChar->mpCrouchingShape, slop);
		}
		else
		{
			pChar->mpChar->SetShape(pChar->mpStandingShape, slop);
		}
	}
	
	if(pChar->mpChar->IsSupported())
	{
		float	curSpeed	=0.0f;
		
		switch(pChar->mMoveMode)
		{
			case	MOVE_WALK:
				curSpeed	=pChar->mWalkSpeed;
				break;
			case	MOVE_RUN:
				curSpeed	=pChar->mRunSpeed;
				break;
			case	MOVE_FLY:
				curSpeed	=pChar->mFlySpeed;
				break;
			case	MOVE_SWIM:
				curSpeed	=pChar->mSwimSpeed;
				break;
			default:
				//TODO: warn
				curSpeed	=pChar->mWalkSpeed;
		}
		
		//Update velocity
		Vec3	curVelocity		=pChar->mpChar->GetLinearVelocity();
		Vec3	desiredVelocity	=curSpeed * movDir;
		
		if(!desiredVelocity.IsNearZero() || curVelocity.GetY() < 0.0f)
		{
			desiredVelocity.SetY(curVelocity.GetY());
		}
		
		Vec3	newVelocity	=0.75f * curVelocity + 0.25f * desiredVelocity;
		
		//Jump
		if(bJump && gstate == Character::EGroundState::OnGround)
		{
			newVelocity	+=Vec3(0, pChar->mJumpSpeed, 0);
		}
		
		//Update the velocity
		pChar->mpChar->SetLinearVelocity(newVelocity);
	}
	else	//midair?
	{
		float	curSpeed	=pChar->mAirSpeed;
		
		//Update velocity
		Vec3	curVelocity		=pChar->mpChar->GetLinearVelocity();
		Vec3	desiredVelocity	=curSpeed * movDir;
		
		desiredVelocity.SetY(curVelocity.GetY());
		
		Vec3	newVelocity	=0.75f * curVelocity + 0.25f * desiredVelocity;
		
		//Jump
		if(bJump && gstate == Character::EGroundState::OnGround)
		{
			newVelocity	+=Vec3(0, pChar->mJumpSpeed, 0);
		}
		
		//Update the velocity
		pChar->mpChar->SetLinearVelocity(newVelocity);
	}
}

//Move should be a scaled vector in meters per second.
//BipedMover holds and controls the frame to frame velocity
//and this function does the move, bouncing off collidable
//stuff to end up at a position.  This decides the final
//position of the character, and influences the velocity for
//the next frame as well.
void	Phys_VCharacterMove(PhysicsStuff *pPS, PhysVCharacter *pChar,
	const vec3 move, float secDelta, vec3 resultVelocity)
{
	CharacterVirtual::ExtendedUpdateSettings	eus;

	Vec3	grav	=pPS->mpPhys->GetGravity();
	Vec3	movDir	={	move[0], move[1], move[2]	};

	Vec3	oldPos	=pChar->mpChar->GetPosition();

	pChar->mpChar-> SetLinearVelocity(movDir);

	pChar->mpChar->ExtendedUpdate(secDelta, grav, eus,
		pPS->mpPhys->GetDefaultBroadPhaseLayerFilter(Layers::MOVING_FRIENDLY),
		pPS->mpPhys->GetDefaultLayerFilter(Layers::MOVING_FRIENDLY),
		{}, {}, *pPS->mpTAlloc);
	
	//calc new effective velocity
	RVec3	newPos	=pChar->mpChar->GetPosition();

	//this is movement over a frame, not meters per second
	Vec3	res	=Vec3(newPos - oldPos);

	//pass back the result
	sCopyVec(&res, resultVelocity);
}

void	Phys_CharacterGetPos(const PhysCharacter *pChar, vec3 pos)
{
	RVec3	rpos	=pChar->mpChar->GetPosition();

	pos[0]	=rpos.GetX();
	pos[1]	=rpos.GetY();
	pos[2]	=rpos.GetZ();
}

void	Phys_VCharacterGetPos(const PhysVCharacter *pChar, vec3 pos)
{
	RVec3	rpos	=pChar->mpChar->GetPosition();

	pos[0]	=rpos.GetX();
	pos[1]	=rpos.GetY();
	pos[2]	=rpos.GetZ();
}

bool	Phys_CharacterIsSupported(const PhysCharacter *pChar)
{
	return	pChar->mpChar->IsSupported();
}

void	Phys_VCharacterGetGroundNormal(const PhysVCharacter *pChar, vec3 normal)
{
	Vec3	norm	=pChar->mpChar->GetGroundNormal();

	sCopyVec(&norm, normal);
}

bool	Phys_VCharacterIsSupported(const PhysVCharacter *pChar)
{
	return	pChar->mpChar->IsSupported();
}


//returns the ID
uint32_t	Phys_CreateAndAddSphere(PhysicsStuff *pPS, float radius,
	const vec3 org, uint16_t layer)
{
	BodyInterface	&body_interface	=pPS->mpPhys->GetBodyInterface();

	Vec3Arg	VOrg(org[0], org[1], org[2]);

	//Now create a dynamic body to bounce on the floor
	//Note that this uses the shorthand version of creating and adding
	//a body to the world
	BodyCreationSettings	ss(new SphereShape(radius), VOrg,
			Quat::sIdentity(), EMotionType::Dynamic, layer);

	BodyID	sphere_id	=body_interface.CreateAndAddBody(ss, EActivation::Activate);

	return	sphere_id.GetIndexAndSequenceNumber();
}


void	Phys_RemoveAndDestroyBody(PhysicsStuff *pPS, uint32_t bodyID)
{
	BodyID	bid(bodyID);

	BodyInterface	&body_interface	=pPS->mpPhys->GetBodyInterface();

	body_interface.RemoveBody(bid);
	body_interface.DestroyBody(bid);
}


void	Phys_GetBodyPos(const PhysicsStuff *pPS, uint32_t bodyID, vec3 pos)
{
	BodyID	bid(bodyID);

	const BodyInterface	&body_interface	=pPS->mpPhys->GetBodyInterface();

	RVec3	rpos	=body_interface.GetPosition(bid);

	pos[0]	=rpos.GetX();
	pos[1]	=rpos.GetY();
	pos[2]	=rpos.GetZ();
}

void	Phys_GetBodyLayer(const PhysicsStuff *pPS, uint32_t bodyID, uint16_t *pLay)
{
	BodyID	bid(bodyID);

	const BodyInterface	&body_interface	=pPS->mpPhys->GetBodyInterface();

	ObjectLayer	ol	=body_interface.GetObjectLayer(bid);

	*pLay	=(uint16_t)ol;
}


void	Phys_SetRestitution(PhysicsStuff *pPS, uint32_t bodyID, float resti)
{
	BodyID	bid(bodyID);

	BodyInterface	&body_interface	=pPS->mpPhys->GetBodyInterface();

	body_interface.SetRestitution(bid, resti);
}


bool	Phys_CastRayAtBodyBroad(const PhysicsStuff *pPS, vec3 org, uint32_t bodyID)
{
	const BroadPhaseQuery	&bpq	=pPS->mpPhys->GetBroadPhaseQuery();

	BodyID	bid(bodyID);

	const BodyInterface	&body_interface	=pPS->mpPhys->GetBodyInterface();

	RVec3	rpos	=body_interface.GetPosition(bid);

	RayCast	rc;

	sCopyvec(org, &rc.mOrigin);

	//make direction vector
	rc.mDirection	=rpos - rc.mOrigin;

//	AllHitCollisionCollector<RayCastBodyCollector>		ahcc;
	ClosestHitCollisionCollector<RayCastBodyCollector>	chcc;

	bpq.CastRay(rc, chcc,
		pPS->mpPhys->GetDefaultBroadPhaseLayerFilter(Layers::MOVING_FRIENDLY),
		pPS->mpPhys->GetDefaultLayerFilter(Layers::MOVING_FRIENDLY));

//	ahcc.Sort();

	//bool	bHits	=!ahcc.mHits.empty();
	if(!chcc.HadHit())
	{
		return	false;
	}
	return	(chcc.mHit.mBodyID == bid);
/*
	for(BroadPhaseCastResult hit : ahcc.mHits)
	{
		if(hit.mBodyID == bid)
		{
			return	true;
		}
	}
	return	false;*/
}

bool	Phys_CastRayAtBodyNarrow(const PhysicsStuff *pPS, vec3 org, uint32_t bodyID)
{
	const NarrowPhaseQuery	&npq	=pPS->mpPhys->GetNarrowPhaseQuery();

	BodyID	bid(bodyID);

	const BodyInterface	&body_interface	=pPS->mpPhys->GetBodyInterface();

	RVec3	rpos	=body_interface.GetPosition(bid);
	Vec3	rayOrg;

	sCopyvec(org, &rayOrg);

	//make direction vector
	Vec3	rayDir	=rpos - rayOrg;

	RRayCast	rc	={	rayOrg, rayDir	};

	RayCastResult	hit;

	bool	bHit	=npq.CastRay(rc, hit,
		pPS->mpPhys->GetDefaultBroadPhaseLayerFilter(Layers::MOVING_FRIENDLY),
		pPS->mpPhys->GetDefaultLayerFilter(Layers::MOVING_FRIENDLY));

	if(!bHit)
	{
		return	false;
	}

	return	(hit.mBodyID == bid);
}