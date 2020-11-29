#include "MochaServerPCH.h"

NetworkManagerServer*    NetworkManagerServer::sInstance;

NetworkManagerServer::NetworkManagerServer() :
    mNewPlayerId( 1 ),
    mNewNetworkId( 1 ),
    mTimeBetweenStatePackets( 0.033f ),
    mClientDisconnectTimeout( 3.f )
{
}

bool NetworkManagerServer::StaticInit( uint16_t inPort, bool useMultiThreading )
{
    sInstance = new NetworkManagerServer();
    return sInstance->Init( inPort, useMultiThreading );
}

bool NetworkManagerServer::StaticInit( uint16_t inPort, bool useMultiThreading, int numThreads )
{
    sInstance = new NetworkManagerServer();
    return sInstance->Init( inPort, useMultiThreading, numThreads );
}

void NetworkManagerServer::HandleConnectionReset( const SocketAddress& inFromAddress )
{
    AddressToClientMap::const_iterator it;
    {
        read_only_lock lock(mtx);
        it = mAddressToClientMap.find( inFromAddress );
    }
    
    if( it != mAddressToClientMap.end() )
    {
        HandleClientDisconnected( it->second );
    }
}

void NetworkManagerServer::ProcessPacket( char* packetMem, InputMemoryBitStream& inInputStream, const SocketAddress& inFromAddress )
{
    AuthPendingClientSet::const_iterator authIt;
    {
        read_only_lock lock(mtx);
        authIt = mAuthPendingClientSet.find( inFromAddress );
    }
    
    // Socket Address를 인증대기목록에서 찾지 못한 경우
    // Heartbeat를 쏴주면서, 인증패킷을 보내도록 해야한다.
    if( authIt == mAuthPendingClientSet.end() )
    {
        // We do not create client proxy yet.
        ProcessClientHeartbeat( inInputStream, inFromAddress );
    }
    
    // Socket Address가 인증대기목록에 있는경우, 클라이언트는 로그인이나 사인업 패킷을 보냈을꺼임.
    else
    {
        // 이젠 Address로 Client를 찾아보자
        AddressToClientMap::const_iterator clientIt;
        {
            read_only_lock lock(mtx);
            clientIt = mAddressToClientMap.find ( inFromAddress );
        }
        
        // AddressToClient 맵에 없다는 거는 클라이언트 프록시가 아직 안만들어졌다는 것.
        // 즉 아직 인증이 안되어있다는 것임.
        if ( clientIt == mAddressToClientMap.end() )
        {
            // We create Client proxy in here.
            HandlePacketFromAuthPendingClient ( inInputStream, inFromAddress );
        }
        
        // 이 경우는 Client가 인증이 되었을 가능성이 높음.
        else
        {
            ProcessPacket( ( *clientIt ).second, inInputStream );
        }
    }
    
    // mPacketVector.erase(mPacketVector.begin());

    // LOG("Erase: %p", packetMem);
     delete[] packetMem;
}

void NetworkManagerServer::ProcessClientHeartbeat(InputMemoryBitStream &inInputStream, const SocketAddress &inFromAddress)
{
    //read the beginning- is it a hello?
    uint32_t    packetType;
    inInputStream.Read( packetType );
    if(  packetType == kHelloCC )
    {
        // code to update map
        {
            printf("Update Lock: ProcessClientHeartbeat\n");
            updatable_lock lock(mtx);
            mAuthPendingClientSet.insert(inFromAddress);
        }

        SendServerHeartbeat(inFromAddress);
    }
    else
    {
        //bad incoming packet from unknown client- we're under attack!!
        LOG( "Bad incoming packet: %d", packetType);
    }
}

void NetworkManagerServer::SendServerHeartbeat(const SocketAddress &inFromAddress)
{
    // Send Client a welcomePacket.
    // But the client has not authenticated yet.
    OutputMemoryBitStream heartbeatPacket;

    heartbeatPacket.Write( kWelcomeCC );

    printf( "Server sent heartbeat.\n" );

    SendPacket( heartbeatPacket, inFromAddress );
}

void NetworkManagerServer::HandlePacketFromAuthPendingClient( InputMemoryBitStream& inInputStream, const SocketAddress& inFromAddress )
{
    uint32_t    packetType;
    inInputStream.Read( packetType );
    
    // Server's heartbeat has been dropped.
    if( packetType == kHelloCC )
    {
        SendServerHeartbeat(inFromAddress);
    }
    // Client Request Login
    else if(  packetType == kLogCC )
    {
        ProcessClientLogInPacket(inInputStream, inFromAddress);
    }
    
    // Client Request SignUP
    else if ( packetType == kSignCC )
    {
        ProcessClientSignUpPacket(inInputStream, inFromAddress);
    }
    else
    {
        LOG( "Bad incoming packet: %d", packetType);
    }
}

void NetworkManagerServer::ProcessClientLogInPacket(InputMemoryBitStream &inInputStream, const SocketAddress &inFromAddress)
{
    //read the name
    string id;
    inInputStream.Read( id );
    string pw;
    inInputStream.Read ( pw );
    
    struct DBAuthResult authResult = DatabaseManager::sInstance->GetClientNameByLogInDB(id, pw);
    
    // Successfully found the user.
    if (authResult.resultCode == 0)
    {
        ClientProxyPtr newClientProxy = std::make_shared< ClientProxy >( inFromAddress, authResult.name, mNewPlayerId++ );
        
        // code to update map
        {
            printf("Update Lock: ProcessClientLogInPacket\n");
            updatable_lock lock(mtx);
            // printf("Update Locked. ");
            mAddressToClientMap[ inFromAddress ] = newClientProxy;
            mPlayerIdToClientMap[ newClientProxy->GetPlayerId() ] = newClientProxy;
            
            // Here creates a client character in server.
            static_cast< Server* > ( AMMOPeer::sInstance.get() )->HandleNewClient( newClientProxy );
            
            for( const auto& pair: mNetworkIdToGameObjectMap )
            {
                newClientProxy->GetReplicationManagerServer().ReplicateCreate( pair.first, pair.second->GetAllStateMask() );
            }
            // printf("Update Unlocked.\n");
        }

        //and welcome the client...
        SendLoggedInPacket( newClientProxy );
    }
    
    // Failed to found the user.
    else
    {
        SendAuthFailurePacket( inFromAddress, authResult );
    }
}

void NetworkManagerServer::SendLoggedInPacket( ClientProxyPtr inClientProxy )
{
    OutputMemoryBitStream loggedInPacket;

    loggedInPacket.Write( kLoggedCC );
    loggedInPacket.Write( inClientProxy->GetPlayerId() );
    loggedInPacket.Write( inClientProxy->GetName() );

    LOG( "[Log In] Server Welcoming, new client '%s' as player %d", inClientProxy->GetName().c_str(), inClientProxy->GetPlayerId() );

    SendPacket( loggedInPacket, inClientProxy->GetSocketAddress() );
}

void NetworkManagerServer::SendSignedUpPacket( ClientProxyPtr inClientProxy )
{
    OutputMemoryBitStream signedUpPacket;

    signedUpPacket.Write( kSignedCC );
    signedUpPacket.Write( inClientProxy->GetPlayerId() );
    signedUpPacket.Write( inClientProxy->GetName() );

    LOG( "[Sign Up] Server Welcoming, new client '%s' as player %d", inClientProxy->GetName().c_str(), inClientProxy->GetPlayerId() );

    SendPacket( signedUpPacket, inClientProxy->GetSocketAddress() );
}

void NetworkManagerServer::SendAuthFailurePacket(const SocketAddress &inFromAddress, const DBAuthResult authResult)
{
    OutputMemoryBitStream authFailPacket;

    authFailPacket.Write( kFailedCC );
    
    authFailPacket.Write( authResult.resultCode );

    printf( "Server sent auth failure.\n" );

    SendPacket( authFailPacket, inFromAddress );
}

void NetworkManagerServer::ProcessClientSignUpPacket(InputMemoryBitStream &inInputStream, const SocketAddress &inFromAddress)
{
    //read the name
    string id;
    inInputStream.Read( id );
    string pw;
    inInputStream.Read ( pw );
    string name;
    inInputStream.Read ( name );
    
    struct DBAuthResult authResult = DatabaseManager::sInstance->GetClientNameBySignUpDB(id, pw, name);
    
    // Successfully found the user.
    if (authResult.resultCode == 0)
    {
        ClientProxyPtr newClientProxy = std::make_shared< ClientProxy >( inFromAddress, authResult.name, mNewPlayerId++ );
        
        // code to update map
        {
            printf("Update Lock: ProcessClientSignUpPacket\n");
            updatable_lock lock(mtx);
            // printf("Update Locked. ");
            mAddressToClientMap[ inFromAddress ] = newClientProxy;
            mPlayerIdToClientMap[ newClientProxy->GetPlayerId() ] = newClientProxy;
            
            // Here creates a client character in server.
            static_cast< Server* > ( AMMOPeer::sInstance.get() )->HandleNewClient( newClientProxy );
            
            for( const auto& pair: mNetworkIdToGameObjectMap )
            {
                newClientProxy->GetReplicationManagerServer().ReplicateCreate( pair.first, pair.second->GetAllStateMask() );
            }
            // printf("Update Unlocked.\n");
        }

        //and welcome the client...
        SendSignedUpPacket( newClientProxy );
    }
    
    // Failed to found the user.
    else
    {
        SendAuthFailurePacket( inFromAddress, authResult );
    }
}


void NetworkManagerServer::ProcessPacket( ClientProxyPtr inClientProxy, InputMemoryBitStream& inInputStream )
{
    //remember we got a packet so we know not to disconnect for a bit
    inClientProxy->UpdateLastPacketTime();

    uint32_t    packetType;
    inInputStream.Read( packetType );
    switch( packetType )
    {
    case kHelloCC:
        printf( "Authenticated client has sent a hello Packet.\n");
        break;
    case kSignCC:
        SendSignedUpPacket( inClientProxy );
        break;
    case kLogCC:
        SendLoggedInPacket ( inClientProxy );
        break;
    case kInputCC:
        HandleInputPacket( inClientProxy, inInputStream );
        break;
    default:
        LOG( "Unknown packet type: %d", packetType);
        break;
    }
}

void NetworkManagerServer::RespawnPlayers()
{
    read_only_lock lock(mtx);
    for( auto it = mAddressToClientMap.begin(), end = mAddressToClientMap.end(); it != end; ++it )
    {
        ClientProxyPtr clientProxy = it->second;
    
        clientProxy->RespawnPlayerIfNecessary();
    }
}

void NetworkManagerServer::SendOutgoingPackets()
{
    //let's send a client a state packet whenever their move has come in...
    for( auto it = mAddressToClientMap.begin(), end = mAddressToClientMap.end(); it != end; ++it )
    {
        ClientProxyPtr clientProxy = it->second;
        if( clientProxy->IsLastMoveTimestampDirty() )
        {
            if (bMultiThreading)
            {
                mPool->EnqueueJob([&, clientProxy]() mutable {
                    SendStatePacketToClient(clientProxy);
                });
            }
            else
            {
                SendStatePacketToClient( clientProxy );
            }
        }
    }
}

void NetworkManagerServer::SendStatePacketToClient( ClientProxyPtr inClientProxy )
{
    //build state packet
    OutputMemoryBitStream    statePacket;

    //it's state!
    statePacket.Write( kStateCC );

    WriteLastMoveTimestampIfDirty( statePacket, inClientProxy );

    {
        updatable_lock lock(mtx);
        // printf("Update Lock: SendStatePacketToClient\n");
        inClientProxy->GetReplicationManagerServer().Write( statePacket );
    }
    
    SendPacket( statePacket, inClientProxy->GetSocketAddress() );
    
}

void NetworkManagerServer::WriteLastMoveTimestampIfDirty( OutputMemoryBitStream& inOutputStream, ClientProxyPtr inClientProxy )
{
    //first, dirty?
    bool isTimestampDirty = inClientProxy->IsLastMoveTimestampDirty();
    inOutputStream.Write( isTimestampDirty );
    if( isTimestampDirty )
    {
        inOutputStream.Write( inClientProxy->GetUnprocessedMoveList().GetLastMoveTimestamp() );
        inClientProxy->SetIsLastMoveTimestampDirty( false );
    }
}

//should we ask the server for this? or run through the world ourselves?
void NetworkManagerServer::AddWorldStateToPacket( OutputMemoryBitStream& inOutputStream )
{
    const auto& gameObjects = MMOWorld::sInstance->GetGameObjects();

    //now start writing objects- do we need to remember how many there are? we can check first...
    inOutputStream.Write( gameObjects.size() );

    for( MochaObjectPtr gameObject : gameObjects )
    {
        inOutputStream.Write( gameObject->GetNetworkId() );
        inOutputStream.Write( gameObject->GetClassId() );
        gameObject->Write( inOutputStream, 0xffffffff );
    }
}

int NetworkManagerServer::GetNewNetworkId()
{
    int toRet = mNewNetworkId++;
    if( mNewNetworkId < toRet )
    {
        LOG( "Network ID Wrap Around!!! You've been playing way too long...", 0 );
    }

    return toRet;

}

void NetworkManagerServer::HandleInputPacket( ClientProxyPtr inClientProxy, InputMemoryBitStream& inInputStream )
{
    uint32_t moveCount = 0;
    MMOMove move;
    inInputStream.Read( moveCount, 2 );
    for( ; moveCount > 0; --moveCount )
    {
        if( move.Read( inInputStream ) )
        {
            if( inClientProxy->GetUnprocessedMoveList().AddMove( move ) )
            {
                inClientProxy->SetIsLastMoveTimestampDirty( true );
            }
        }
    }
}

ClientProxyPtr NetworkManagerServer::GetClientProxy( int inPlayerId )
{
    IntToClientMap::const_iterator it;
    
    {
        read_only_lock lock(mtx);
        it = mPlayerIdToClientMap.find( inPlayerId );
    }
    
    if( it != mPlayerIdToClientMap.end() )
    {
        return it->second;
    }

    return nullptr;
}

void NetworkManagerServer::CheckForDisconnects()
{
    vector< ClientProxyPtr > clientsToDC;

    float minAllowedLastPacketFromClientTime = Timing::sInstance.GetTimef() - mClientDisconnectTimeout;
    
    {
        read_only_lock lock(mtx);
        for( const auto& pair: mAddressToClientMap )
        {
            if( pair.second->GetLastPacketFromClientTime() < minAllowedLastPacketFromClientTime )
            {
                //can't remove from map while in iterator, so just remember for later...
                clientsToDC.push_back( pair.second );
            }
        }
    }
    
    for( ClientProxyPtr client: clientsToDC )
    {
        HandleClientDisconnected( client );
    }
}

void NetworkManagerServer::HandleClientDisconnected( ClientProxyPtr inClientProxy )
{
    LOG( "Client %s has been disconnected [%d]", inClientProxy->GetName().c_str(), inClientProxy->GetPlayerId() );
    
    {
        printf("Update Lock: HandleClientDisconnected\n");
        updatable_lock lock(mtx);
//        printf("Update Locked. ");
        mPlayerIdToClientMap.erase( inClientProxy->GetPlayerId() );
        mAddressToClientMap.erase( inClientProxy->GetSocketAddress() );
        static_cast< Server* > ( AMMOPeer::sInstance.get() )->HandleLostClient( inClientProxy );
//        printf("Update Unlocked.\n");
    }
    
    //was that the last client? if so, bye!
    if( mAddressToClientMap.empty() )
    {
//        Engine::sInstance->SetShouldKeepRunning( false );
    }
}

void NetworkManagerServer::RegisterGameObject( MochaObjectPtr inGameObject )
{
    //assign network id
    int newNetworkId = GetNewNetworkId();
    inGameObject->SetNetworkId( newNetworkId );


    // We Do not LOCK in here.
    // As we already have locked @handlePacketFromNewClient
    
        //add mapping from network id to game object
        mNetworkIdToGameObjectMap[ newNetworkId ] = inGameObject;

        //tell all client proxies this is new...
        for( const auto& pair: mAddressToClientMap )
        {
            pair.second->GetReplicationManagerServer().ReplicateCreate( newNetworkId, inGameObject->GetAllStateMask() );
        }
    
}


void NetworkManagerServer::UnregisterGameObject( IMochaObject* inGameObject )
{
    printf("Update Lock: UnregisterGameObject\n");
    updatable_lock lock(mtx);
//    printf("Update Locked: UNREGISTER GAMEOBJECT");
    
    int networkId = inGameObject->GetNetworkId();

    mNetworkIdToGameObjectMap.erase( networkId );

    //tell all client proxies to STOP replicating!
    //tell all client proxies this is new...
    for( const auto& pair: mAddressToClientMap )
    {
        pair.second->GetReplicationManagerServer().ReplicateDestroy( networkId );
    }
    
//    printf("Update Unlocked: UNREGISTER GAMEOBJECT");
}

void NetworkManagerServer::SetStateDirty( int inNetworkId, uint32_t inDirtyState )
{
    // printf("Update Lock: SetStateDirty\n");
    updatable_lock lock(mtx);
//    printf("Update Locked. ");
    //tell everybody this is dirty
    for( const auto& pair: mAddressToClientMap )
    {
        pair.second->GetReplicationManagerServer().SetStateDirty( inNetworkId, inDirtyState );
    }
//    printf("Update Unlocked.\n");
}


//void NetworkManagerServer::HandlePacketFromNewClient( InputMemoryBitStream& inInputStream, const SocketAddress& inFromAddress )
//{
//    //read the beginning- is it a hello?
//    uint32_t    packetType;
//    inInputStream.Read( packetType );
//    if(  packetType == kHelloCC )
//    {
//        //read the name
//        string name;
//        inInputStream.Read( name );
//        ClientProxyPtr newClientProxy = std::make_shared< ClientProxy >( inFromAddress, name, mNewPlayerId++ );
//
//        // code to update map
//        {
//            printf("Update Lock: HandlePacketFromNewClient\n");
//            updatable_lock lock(mtx);
//            // printf("Update Locked. ");
//            mAddressToClientMap[ inFromAddress ] = newClientProxy;
//            mPlayerIdToClientMap[ newClientProxy->GetPlayerId() ] = newClientProxy;
//
//            // Here creates a client character in server.
//            static_cast< Server* > ( AMMOPeer::sInstance.get() )->HandleNewClient( newClientProxy );
//
//            for( const auto& pair: mNetworkIdToGameObjectMap )
//            {
//                newClientProxy->GetReplicationManagerServer().ReplicateCreate( pair.first, pair.second->GetAllStateMask() );
//            }
//            // printf("Update Unlocked.\n");
//        }
//
//        //and welcome the client...
//        SendWelcomePacket( newClientProxy );
//
//    }
//    else
//    {
//        //bad incoming packet from unknown client- we're under attack!!
//        LOG( "Bad incoming packet: %d", packetType);
//    }
//}

