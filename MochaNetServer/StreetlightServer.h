#pragma once


class StreetlightServer : public IMochaObject
{
public:
    enum EStreetlightReplicationState
    {
        ECRS_Pose = 1 << 0,
        ECRS_AllState = ECRS_Pose
    };
    virtual uint32_t GetAllStateMask()    const override    { return ECRS_AllState; }

    void        SetPlayerId( uint32_t inPlayerId )            { mPlayerId = inPlayerId; }
    
    uint32_t    GetPlayerId()                        const     { return mPlayerId; }

    virtual uint32_t    Write( OutputMemoryBitStream& inOutputStream, uint32_t inDirtyState ) const override;
    
    static MochaObjectPtr    StaticCreate() { return NetworkManagerServer::sInstance->RegisterAndReturn( new StreetlightServer() ); }
    
    virtual void Update() override;
    
    virtual uint32_t GetClassId() const override { return kClassId; }

protected:
    StreetlightServer();

    uint32_t            mPlayerId;

    uint32_t kClassId;

};

