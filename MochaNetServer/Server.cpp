#include "MochaServerPCH.h"

bool Server::StaticInit()
{
    sInstance.reset( new Server() );

    return true;
}

Server::Server()
{
    GameObjectRegistry::sInstance->RegisterCreationFunction( 'PLYR', FirstFantasyCharacterServer::StaticCreate );
    GameObjectRegistry::sInstance->RegisterCreationFunction( 'ARCH', ArcherCharacterServer::StaticCreate );
    GameObjectRegistry::sInstance->RegisterCreationFunction( 'SLIT', StreetlightServer::StaticCreate );
    GameObjectRegistry::sInstance->RegisterCreationFunction( 'EXPR', ExplosionRPCServer::StaticCreate );

    
    DatabaseManager::StaticInit();
    DatabaseManager::sInstance->ConnectToDB("127.0.0.1,5432@firstfantasy", "postgres", "LAUmac0117!");
    
    // Pass 'true' to enable multi-threading
    InitNetworkManager(true);
    
    // Setup latency
    float latency = 0.0f;
    string latencyString = StringUtils::GetCommandLineArg( 2 );
    if( !latencyString.empty() )
    {
        latency = stof( latencyString );
    }
    NetworkManagerServer::sInstance->SetSimulatedLatency( latency );
    
}


void Server::Run()
{
    SetupWorld();

    AMMOPeer::Run();
}

bool Server::InitNetworkManager(bool useMultiThreading)
{
    string portString = StringUtils::GetCommandLineArg( 1 );
    uint16_t port = stoi( portString );

    return NetworkManagerServer::StaticInit( port, useMultiThreading );
}

bool Server::InitNetworkManager(bool useMultiThreading, int numThreads)
{
    string portString = StringUtils::GetCommandLineArg( 1 );
    uint16_t port = stoi( portString );

    return NetworkManagerServer::StaticInit( port, useMultiThreading, numThreads );
}

namespace
{
//    void CreateRandomArcher( int inNumber )
//    {
//        Vector3 minPos( -1000.f, -1000.f, 230.f );
//        Vector3 maxPos( 1000.f, 1000.f, 230.f );
//        MochaObjectPtr go;
//
//        //make a mouse somewhere- where will these come from?
//        for( int i = 0; i < inNumber; ++i )
//        {
//            go = GameObjectRegistry::sInstance->CreateGameObject( 'ARCH' );
//            Vector3 archerLcation = MMOMath::GetRandomVector( minPos, maxPos );
//            go->SetLocation( archerLcation );
//        }
//    }

} // End of namespace


void Server::SetupWorld()
{
//    CreateRandomArcher( 10 );
    
//    DatabaseManager::sInstance->CreateFieldObjectRecord();
//    DatabaseManager::sInstance->CreateUserRecord();
//    DatabaseManager::sInstance->PutRandomArchersToDatabse(50);
    
    DatabaseManager::sInstance->CreateArchersFromDB();


}

void Server::Tick()
{
    NetworkManagerServer::sInstance->ProcessIncomingPackets();

    NetworkManagerServer::sInstance->CheckForDisconnects();

    NetworkManagerServer::sInstance->RespawnPlayers();

    AMMOPeer::Tick();

    NetworkManagerServer::sInstance->SendOutgoingPackets();

}

void Server::HandleNewClient( ClientProxyPtr inClientProxy )
{
    int playerId = inClientProxy->GetPlayerId();
    
    SpawnPlayer( playerId );
}

void Server::SpawnPlayer( int inPlayerId )
{
    shared_ptr<FirstFantasyCharacterServer> cat = std::static_pointer_cast< FirstFantasyCharacterServer >( GameObjectRegistry::sInstance->CreateGameObject( 'PLYR' ) );

    cat->SetPlayerId( inPlayerId );

    cat->SetLocation( Vector3( 1.f - static_cast< float >( inPlayerId ), 0.f, 0.f ) );
}

void Server::HandleLostClient( ClientProxyPtr inClientProxy )
{
    //Remove client's player
    int playerId = inClientProxy->GetPlayerId();


    shared_ptr<FirstFantasyCharacterServer> plyr = GetCatForPlayer( playerId );
    if( plyr )
    {
        plyr->SetDoesWantToDie( true );
    }
}

shared_ptr<FirstFantasyCharacterServer> Server::GetCatForPlayer( int inPlayerId )
{
    //run through the objects till we find the cat...
    //it would be nice if we kept a pointer to the cat on the clientproxy
    //but then we'd have to clean it up when the cat died, etc.
    //this will work for now until it's a perf issue
    const auto& gameObjects = MMOWorld::sInstance->GetGameObjects();
    for( int i = 0, c = gameObjects.size(); i < c; ++i )
    {
        MochaObjectPtr go = gameObjects[ i ];
        FirstFantasyCharacterServer* cat = dynamic_cast<FirstFantasyCharacterServer*>(go.get());
        if( cat && cat->GetPlayerId() == inPlayerId )
        {
            return std::static_pointer_cast< FirstFantasyCharacterServer >( go );
        }
    }

    return nullptr;

}
