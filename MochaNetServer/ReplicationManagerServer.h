#pragma once

class ReplicationManagerServer
{
    

public:
    void ReplicateCreate( int inNetworkId, uint32_t inInitialDirtyState );
    void ReplicateDestroy( int inNetworkId );
    void SetStateDirty( int inNetworkId, uint32_t inDirtyState );
    void HandleCreateAckd( int inNetworkId );
    void RemoveFromReplication( int inNetworkId );

    void ReplicateRPC( int inNetworkId, uint32_t inInitialDirtyState);

    bool Write( OutputMemoryBitStream& inOutputStream, ReplicationManagerTransmissionData* ioTransmissionData, OutputMemoryBitStream& fragPacket );

private:

    uint32_t WriteCreateAction( OutputMemoryBitStream& inOutputStream, int inNetworkId, uint32_t inDirtyState );
    uint32_t WriteUpdateAction( OutputMemoryBitStream& inOutputStream, int inNetworkId, uint32_t inDirtyState );
    uint32_t WriteDestroyAction( OutputMemoryBitStream& inOutputStream, int inNetworkId, uint32_t inDirtyState );
    uint32_t WriteRPCAction( OutputMemoryBitStream& inOutputStream, int inNetworkId, uint32_t inDirtyState );

    unordered_map< int, ReplicationCommand >    mNetworkIdToReplicationCommand;
    vector< int >                                mNetworkIdsToRemove;

    uint32_t                    mMaximumSerlizableBytes = 500;
};
