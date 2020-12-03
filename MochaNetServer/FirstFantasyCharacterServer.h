#pragma once

enum ECatControlType
{
    ESCT_Human,
    ESCT_AI
};

class FirstFantasyCharacterServer : public IMochaObject
{
public:
    enum ECatReplicationState
    {
        ECRS_Pose = 1 << 0,
        ECRS_Color = 1 << 1,
        ECRS_PlayerId = 1 << 2,
        ECRS_Health = 1 << 3,

        ECRS_AllState = ECRS_Pose | ECRS_Color | ECRS_PlayerId | ECRS_Health
    };
    virtual    FirstFantasyCharacterServer*    GetAsCharacter() { return this; }
    virtual uint32_t GetAllStateMask()    const override    { return ECRS_AllState; }
    void ProcessInput( float inDeltaTime, const MMOInputState& inInputState );
    void SimulateMovement( float inDeltaTime, const MMOInputState& inInputState );

    void        SetPlayerId( uint32_t inPlayerId )            { mPlayerId = inPlayerId; }
    uint32_t    GetPlayerId()                        const     { return mPlayerId; }

    void            SetVelocity( const Vector3& inVelocity )    { mVelocity = inVelocity; }
    const Vector3&    GetVelocity()                        const    { return mVelocity; }

    virtual uint32_t    Write( OutputMemoryBitStream& inOutputStream, uint32_t inDirtyState ) const override;
    
    
    static MochaObjectPtr    StaticCreate() { return NetworkManagerServer::sInstance->RegisterAndReturn( new FirstFantasyCharacterServer() ); }
    virtual void HandleDying() override;

    virtual void Update() override;

    void SetCatControlType( ECatControlType inCatControlType ) { mCatControlType = inCatControlType; }

    void TakeDamage( int inDamagingPlayerId );
    
    virtual uint32_t GetClassId() const override { return kClassId; }

protected:
    FirstFantasyCharacterServer();

private:

    void HandleShooting();

    ECatControlType    mCatControlType;


    float        mTimeOfNextShot;
    float        mTimeBetweenShots;
    
    void    AdjustVelocityByThrust( float inDeltaTime, const MMOInputState& inInputState );

    Vector3                mVelocity;


    float                mMaxLinearSpeed;
    float                mMaxRotationSpeed;

    //bounce fraction when hitting various things
    float                mWallRestitution;
    float                mCatRestitution;


    uint32_t            mPlayerId;

protected:

    ///move down here for padding reasons...
    
    float                mLastMoveTimestamp;

    float                mThrustDirForward;
    float               mThrustDirRight;
    int                    mHealth;

    bool                mIsShooting;
    
    uint32_t kClassId;

};

