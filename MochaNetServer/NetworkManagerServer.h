#pragma once

#include <fstream>

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
    void            RegisterRPC( MochaObjectPtr inGameObject );

    inline    MochaObjectPtr    RegisterAndReturn( IMochaObject* inGameObject );
    inline    MochaObjectPtr    RegisterRPCAndReturn( IMochaObject* inGameObject );

    
            void            UnregisterGameObject( IMochaObject* inGameObject );
            void            UnregisterRPC( IMochaObject* inGameObject );

            void            SetStateDirty( int inNetworkId, uint32_t inDirtyState );

            void            RespawnPlayers();

            ClientProxyPtr    GetClientProxy( int inPlayerId );
    
    inline    MochaObjectPtr    GetGameObject( int inNetworkId );

private:
            NetworkManagerServer();

            void    ProcessClientHeartbeat( InputMemoryBitStream& inInputStream, const SocketAddress& inFromAddress );
    
            void    SendServerHeartbeat( const SocketAddress& inFromAddress );
    
            void    HandlePacketFromAuthPendingClient( InputMemoryBitStream& inInputStream, const SocketAddress& inFromAddress );
    
            void    ProcessClientLogInPacket( InputMemoryBitStream& inInputStream, const SocketAddress& inFromAddress );
    
            void    ProcessClientSignUpPacket( InputMemoryBitStream& inInputStream, const SocketAddress& inFromAddress );
    
            void    ProcessPacket( ClientProxyPtr inClientProxy, InputMemoryBitStream& inInputStream );
                
            void    SendLoggedInPacket( ClientProxyPtr inClientProxy );

            void    SendSignedUpPacket( ClientProxyPtr inClientProxy );
    
            void    SendAuthFailurePacket( const SocketAddress& inFromAddress, const DBAuthResult authResult );
            
            void    AddWorldStateToPacket( OutputMemoryBitStream& inOutputStream );

            void    SendStatePacketToClient( ClientProxyPtr inClientProxy );
            void    WriteLastMoveTimestampIfDirty( OutputMemoryBitStream& inOutputStream, ClientProxyPtr inClientProxy );

            void    HandleInputPacket( ClientProxyPtr inClientProxy, InputMemoryBitStream& inInputStream );

            void    HandleClientDisconnected( ClientProxyPtr inClientProxy );

            int        GetNewNetworkId();
    
    typedef unordered_set< SocketAddress > AuthPendingClientSet;
    typedef unordered_map< int, ClientProxyPtr >    IntToClientMap;
    typedef unordered_map< SocketAddress, ClientProxyPtr >    AddressToClientMap;

    AuthPendingClientSet      mAuthPendingClientSet;
    AddressToClientMap        mAddressToClientMap;
    IntToClientMap            mPlayerIdToClientMap;

    int                mNewPlayerId;
    int                mNewNetworkId;

    float            mTimeOfLastSatePacket;
    float            mTimeBetweenStatePackets;
    float            mClientDisconnectTimeout;
    
    unsigned int     mTotalProcessedPackets;
    float            mTimeOfLastPPSLog;
    
    float            mFirstPacketProcessTime;
    
    mutex_type mtx;
    
    std::ofstream   fout;
    
};


inline MochaObjectPtr NetworkManagerServer::RegisterAndReturn( IMochaObject* inGameObject )
{
    MochaObjectPtr toRet( inGameObject );
    RegisterGameObject( toRet );
    return toRet;
}

inline MochaObjectPtr NetworkManagerServer::RegisterRPCAndReturn( IMochaObject* inGameObject )
{
    MochaObjectPtr toRet( inGameObject );
    RegisterRPC( toRet );
    return toRet;
}

inline MochaObjectPtr NetworkManagerServer::GetGameObject( int inNetworkId )
{
    IntToGameObjectMap::const_iterator gameObjectIt;
    
    // We Do not LOCK in here.
    // As we already have locked @SendStatePacketToClient
    
//    {
//        read_only_lock lock(mtx);
        gameObjectIt = mNetworkIdToGameObjectMap.find( inNetworkId );
//    }

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
