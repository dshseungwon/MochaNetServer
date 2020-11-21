#include "MMOPCH.h"

// Add default functionality here for any IMochaObject functions that are not pure virtual.
IMochaObject::IMochaObject() :
    mColor( Colors::White ),
    mCollisionRadius( 0.5f ),
    mRotation( 0.f ),
    mScale( 1.0f ),
    mIndexInWorld( -1 ),
    mDoesWantToDie( false ),
    mNetworkId( 0 ),
    mPlayerId( 0 ),
    kClassId('GOBJ')
{
}

void IMochaObject::Update()
{
    //object don't do anything by default...
}


Vector3 IMochaObject::GetForwardVector()    const
{
    //should we cache this when you turn?
//    return Vector3( sinf( mRotation ), -cosf( mRotation ), 0.f );
    return Vector3( 0.f, -1.f, 0.f );
}

Vector3 IMochaObject::GetRightVector()    const
{
    //should we cache this when you turn?
    return Vector3( 1.f, 0.f, 0.f );
}

void IMochaObject::SetNetworkId( int inNetworkId )
{
    //this doesn't put you in the map or remove you from it
    mNetworkId = inNetworkId;

}

void IMochaObject::SetRotation( float inRotation )
{
    //should we normalize using fmodf?
    mRotation = inRotation;
}
