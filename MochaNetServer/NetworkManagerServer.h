#pragma once

class NetworkManagerServer : public NetworkManager
{
public:
    static NetworkManagerServer*    sInstance;

    static bool                StaticInit( uint16_t inPort );
        
    virtual void            ProcessPacket( char* packetMem, InputMemoryBitStream inInputStream, const SocketAddress inFromAddress ) override;
    virtual void            HandleConnectionReset( const SocketAddress& inFromAddress ) override;
        
            void            SendOutgoingPackets();
            void            CheckForDisconnects();

    void            RegisterGameObject( MochaObjectPtr inGameObject );
    inline    MochaObjectPtr    RegisterAndReturn( IMochaObject* inGameObject );
            void            UnregisterGameObject( IMochaObject* inGameObject );
            void            SetStateDirty( int inNetworkId, uint32_t inDirtyState );

            void            RespawnPlayers();

            ClientProxyPtr    GetClientProxy( int inPlayerId ) const;

private:
            NetworkManagerServer();

            void    HandlePacketFromNewClient( InputMemoryBitStream& inInputStream, const SocketAddress& inFromAddress );
            void    ProcessPacket( ClientProxyPtr inClientProxy, InputMemoryBitStream& inInputStream );
            
            void    SendWelcomePacket( ClientProxyPtr inClientProxy );
            void    UpdateAllClients();
            
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
    
};


inline MochaObjectPtr NetworkManagerServer::RegisterAndReturn( IMochaObject* inGameObject )
{
    MochaObjectPtr toRet( inGameObject );
    RegisterGameObject( toRet );
    return toRet;
}
