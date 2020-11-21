#include "MochaServerPCH.h"



//uncomment this when you begin working on the server

bool Server::StaticInit()
{
    sInstance.reset( new Server() );

    return true;
}

Server::Server()
{

    GameObjectRegistry::sInstance->RegisterCreationFunction( 'RCAT', FirstFantasyCharacterServer::StaticCreate );


    InitNetworkManager();
    
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

bool Server::InitNetworkManager()
{
    string portString = StringUtils::GetCommandLineArg( 1 );
    uint16_t port = stoi( portString );

    return NetworkManagerServer::StaticInit( port );
}



void Server::SetupWorld()
{
    //spawn some random mice
//    CreateRandomMice( 10 );
    
    //spawn more random mice!
//    CreateRandomMice( 10 );
}

void Server::Tick()
{
    NetworkManagerServer::sInstance->ProcessIncomingPackets();

    NetworkManagerServer::sInstance->CheckForDisconnects();

    NetworkManagerServer::sInstance->RespawnCats();

    AMMOPeer::Tick();

    NetworkManagerServer::sInstance->SendOutgoingPackets();

}

void Server::HandleNewClient( ClientProxyPtr inClientProxy )
{
    
    int playerId = inClientProxy->GetPlayerId();
    
    // ScoreBoardManager::sInstance->AddEntry( playerId, inClientProxy->GetName() );
    SpawnCatForPlayer( playerId );
}

void Server::SpawnCatForPlayer( int inPlayerId )
{
    shared_ptr<FirstFantasyCharacterServer> cat = std::static_pointer_cast< FirstFantasyCharacterServer >( GameObjectRegistry::sInstance->CreateGameObject( 'RCAT' ) );
    // cat->SetColor( ScoreBoardManager::sInstance->GetEntry( inPlayerId )->GetColor() );
    cat->SetPlayerId( inPlayerId );
    //gotta pick a better spawn location than this...
    cat->SetLocation( Vector3( 1.f - static_cast< float >( inPlayerId ), 0.f, 230.f ) );

}

void Server::HandleLostClient( ClientProxyPtr inClientProxy )
{
    //kill client's cat
    //remove client from scoreboard
    int playerId = inClientProxy->GetPlayerId();

    // ScoreBoardManager::sInstance->RemoveEntry( playerId );
    shared_ptr<FirstFantasyCharacterServer> cat = GetCatForPlayer( playerId );
    if( cat )
    {
        cat->SetDoesWantToDie( true );
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
