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
    // LOG("inputStream 1: %p", &inInputStream);
    
    //try to get the client proxy for this address
    //pass this to the client proxy to process
    
    AddressToClientMap::const_iterator it;
    {
        read_only_lock lock(mtx);
        it = mAddressToClientMap.find( inFromAddress );
    }
    
    if( it == mAddressToClientMap.end() )
    {
        //didn't find one? it's a new cilent..is the a HELO? if so, create a client proxy...
        HandlePacketFromNewClient( inInputStream, inFromAddress );
    }
    else
    {
        ProcessPacket( ( *it ).second, inInputStream );
    }
    
    // mPacketVector.erase(mPacketVector.begin());

    // LOG("Erase: %p", packetMem);
     delete[] packetMem;
}


void NetworkManagerServer::ProcessPacket( ClientProxyPtr inClientProxy, InputMemoryBitStream& inInputStream )
{
    // LOG("inputStream 2: %p", &inInputStream);
    //remember we got a packet so we know not to disconnect for a bit
    inClientProxy->UpdateLastPacketTime();

    uint32_t    packetType;
    inInputStream.Read( packetType );
    switch( packetType )
    {
    case kHelloCC:
        //need to resend welcome. to be extra safe we should check the name is the one we expect from this address,
        //otherwise something weird is going on...
        SendWelcomePacket( inClientProxy );
        break;
    case kInputCC:
        HandleInputPacket( inClientProxy, inInputStream );
        break;
    default:
        LOG( "Unknown packet type: %d", packetType);
        break;
    }
}


void NetworkManagerServer::HandlePacketFromNewClient( InputMemoryBitStream& inInputStream, const SocketAddress& inFromAddress )
{
    //read the beginning- is it a hello?
    uint32_t    packetType;
    inInputStream.Read( packetType );
    if(  packetType == kHelloCC )
    {
        //read the name
        string name;
        inInputStream.Read( name );
        ClientProxyPtr newClientProxy = std::make_shared< ClientProxy >( inFromAddress, name, mNewPlayerId++ );
        
        // code to update map
        {
            printf("Update Lock: HandlePacketFromNewClient\n");
            updatable_lock lock(mtx);
            // printf("Update Locked. ");
            mAddressToClientMap[ inFromAddress ] = newClientProxy;
            mPlayerIdToClientMap[ newClientProxy->GetPlayerId() ] = newClientProxy;
            
            static_cast< Server* > ( AMMOPeer::sInstance.get() )->HandleNewClient( newClientProxy );
            
            for( const auto& pair: mNetworkIdToGameObjectMap )
            {
                newClientProxy->GetReplicationManagerServer().ReplicateCreate( pair.first, pair.second->GetAllStateMask() );
            }
            // printf("Update Unlocked.\n");
        }

        //and welcome the client...
        SendWelcomePacket( newClientProxy );
        
    }
    else
    {
        //bad incoming packet from unknown client- we're under attack!!
        LOG( "Bad incoming packet: %d", packetType);
    }
}

void NetworkManagerServer::SendWelcomePacket( ClientProxyPtr inClientProxy )
{
    OutputMemoryBitStream welcomePacket;

    welcomePacket.Write( kWelcomeCC );
    welcomePacket.Write( inClientProxy->GetPlayerId() );

    LOG( "Server Welcoming, new client '%s' as player %d", inClientProxy->GetName().c_str(), inClientProxy->GetPlayerId() );

    SendPacket( welcomePacket, inClientProxy->GetSocketAddress() );
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

    inClientProxy->GetReplicationManagerServer().Write( statePacket );
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
    printf("Update Lock: SetStateDirty\n");
    updatable_lock lock(mtx);
//    printf("Update Locked. ");
    //tell everybody this is dirty
    for( const auto& pair: mAddressToClientMap )
    {
        pair.second->GetReplicationManagerServer().SetStateDirty( inNetworkId, inDirtyState );
    }
//    printf("Update Unlocked.\n");
}

