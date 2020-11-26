#pragma once

using mutex_type = std::shared_timed_mutex;
using read_only_lock  = std::shared_lock<mutex_type>;
using updatable_lock = std::unique_lock<mutex_type>;

class NetworkManagerServer : public NetworkManager
{
public:
    static NetworkManagerServer*    sInstance;

    static bool                StaticInit( uint16_t inPort, bool useMultiThreading );
    static bool                StaticInit( uint16_t inPort, bool useMultiThreading, int numThreads);
        
    virtual void            ProcessPacket( char* packetMem, InputMemoryBitStream& inInputStream, const SocketAddress& inFromAddress ) override;
    virtual void            HandleConnectionReset( const SocketAddress& inFromAddress ) override;
        
            void            SendOutgoingPackets();
            void            CheckForDisconnects();

    void            RegisterGameObject( MochaObjectPtr inGameObject );
    inline    MochaObjectPtr    RegisterAndReturn( IMochaObject* inGameObject );
            void            UnregisterGameObject( IMochaObject* inGameObject );
            void            SetStateDirty( int inNetworkId, uint32_t inDirtyState );

            void            RespawnPlayers();

            ClientProxyPtr    GetClientProxy( int inPlayerId );
    
    inline    MochaObjectPtr    GetGameObject( int inNetworkId );

private:
            NetworkManagerServer();

            void    HandlePacketFromNewClient( InputMemoryBitStream& inInputStream, const SocketAddress& inFromAddress );
            void    ProcessPacket( ClientProxyPtr inClientProxy, InputMemoryBitStream& inInputStream );
            
            void    SendWelcomePacket( ClientProxyPtr inClientProxy );
            // void    UpdateAllClients();
            
            void    AddWorldStateToPacket( OutputMemoryBitStream& inOutputStream );

            void    SendStatePacketToClient( ClientProxyPtr inClientProxy );
            void    WriteLastMoveTimestampIfDirty( OutputMemoryBitStream& inOutputStream, ClientProxyPtr inClientProxy );

            void    HandleInputPacket( ClientProxyPtr inClientProxy, InputMemoryBitStream& inInputStream );

            void    HandleClientDisconnected( ClientProxyPtr inClientProxy );

            int        GetNewNetworkId();

    typedef unordered_map< int, ClientProxyPtr >    IntToClientMap;
    typedef unordered_map< SocketAddress, ClientProxyPtr >    AddressToClientMap;

    AddressToClientMap        mAddressToClientMap;
    IntToClientMap            mPlayerIdToClientMap;

    int                mNewPlayerId;
    int                mNewNetworkId;

    float            mTimeOfLastSatePacket;
    float            mTimeBetweenStatePackets;
    float            mClientDisconnectTimeout;
    
    mutex_type mtx;
    
};


inline MochaObjectPtr NetworkManagerServer::RegisterAndReturn( IMochaObject* inGameObject )
{
    MochaObjectPtr toRet( inGameObject );
    RegisterGameObject( toRet );
    return toRet;
}

inline MochaObjectPtr NetworkManagerServer::GetGameObject( int inNetworkId )
{
    IntToGameObjectMap::const_iterator gameObjectIt;
    
    {
        read_only_lock lock(mtx);
        gameObjectIt = mNetworkIdToGameObjectMap.find( inNetworkId );
    }

    if( gameObjectIt != mNetworkIdToGameObjectMap.end() )
    {
        // printf("Found GameObject and Return.\n");
        return gameObjectIt->second;
    }
    else
    {
        printf("GetGameObject has failed.\nThe gameobject does not exist at mNetworkIdToGameObjectMap.\n");
        return MochaObjectPtr();
    }
}
