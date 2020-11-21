#pragma once

#include "MMOMath.h"
#include "MemoryBitStream.h"

class IMochaObject
{

public:
    
    virtual uint32_t GetClassId() const { return kClassId; }
    
    static IMochaObject* CreateInstance() { return static_cast< IMochaObject* >( new IMochaObject() ); }
    
    IMochaObject();

    virtual uint32_t GetAllStateMask()    const { return 0; }

    virtual void    Update();

    virtual void    HandleDying() {}

            void    SetIndexInWorld( int inIndex )                        { mIndexInWorld = inIndex; }
            int        GetIndexInWorld()                const                { return mIndexInWorld; }

            void    SetRotation( float inRotation );
            float    GetRotation()                    const                { return mRotation; }

            void    SetScale( float inScale )                            { mScale = inScale; }
            float    GetScale()                        const                { return mScale; }

    void            SetVelocity( const Vector3& inVelocity )    { mVelocity = inVelocity; }
    const Vector3&    GetVelocity()                        const    { return mVelocity; }

    
    const Vector3&        GetLocation()                const                { return mLocation; }
            void        SetLocation( const Vector3& inLocation )        { mLocation = inLocation; }

            float        GetCollisionRadius()        const                { return mCollisionRadius; }
            void        SetCollisionRadius( float inRadius )            { mCollisionRadius = inRadius; }

            Vector3        GetForwardVector()            const;
            Vector3     GetRightVector()            const;

            void        SetColor( const Vector3& inColor )                    { mColor = inColor; }
    const Vector3&        GetColor()                    const                { return mColor; }

            bool        DoesWantToDie()                const                { return mDoesWantToDie; }
            void        SetDoesWantToDie( bool inWants )                { mDoesWantToDie = inWants; }

            int            GetNetworkId()                const                { return mNetworkId; }
            void        SetNetworkId( int inNetworkId );

    void        SetPlayerId( uint32_t inPlayerId )            { mPlayerId = inPlayerId; }
    uint32_t    GetPlayerId()                        const     { return mPlayerId; }
    
    
    virtual uint32_t    Write( OutputMemoryBitStream& inOutputStream, uint32_t inDirtyState ) const    {  ( void ) inOutputStream; ( void ) inDirtyState; return 0; }
    virtual void        Read( InputMemoryBitStream& inInputStream )                                    { ( void ) inInputStream; }

private:

    Vector3                                            mVelocity;
    Vector3                                            mLocation;
    Vector3                                            mColor;
    
    float                                            mCollisionRadius;


    float                                            mRotation;
    float                                            mScale;
    int                                                mIndexInWorld;

    bool                                            mDoesWantToDie;

    int                                                mNetworkId;
    
    uint32_t            mPlayerId;
    
    uint32_t                                        kClassId;
    
protected:
    float                mTimeLocationBecameOutOfSync;
    float                mTimeVelocityBecameOutOfSync;
    int                  mHealth;
    float                mThrustDir;
};


typedef shared_ptr<IMochaObject>   MochaObjectPtr;
