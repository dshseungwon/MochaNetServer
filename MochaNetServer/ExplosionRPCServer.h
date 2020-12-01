#pragma once


class ExplosionRPCServer : public IMochaObject
{
public:
    enum EExplosionRPCReplicationState
    {
        State_Pose = 1 << 0,
        State_AllState = State_Pose
    };
    virtual uint32_t GetAllStateMask()    const override    { return State_AllState; }

    void        SetPlayerId( uint32_t inPlayerId )            { mPlayerId = inPlayerId; }
    
    uint32_t    GetPlayerId()                        const     { return mPlayerId; }

    virtual uint32_t    Write( OutputMemoryBitStream& inOutputStream, uint32_t inDirtyState ) const override;
    
    static MochaObjectPtr    StaticCreate() { return NetworkManagerServer::sInstance->RegisterAndReturn( new ExplosionRPCServer() ); }
    
    virtual void HandleDying() override;

    virtual void Update() override;
    
    virtual uint32_t GetClassId() const override { return kClassId; }

protected:
    ExplosionRPCServer();

    uint32_t            mPlayerId;

    uint32_t kClassId;

};

