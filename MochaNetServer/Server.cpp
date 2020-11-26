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
    void CreateFieldObjectRecord()
    {
        SAConnection con;
        try {
            con.Connect(_TSA("127.0.0.1,5432@sqlapi"), _TSA("postgres"), _TSA("password"), SA_PostgreSQL_Client);
            LOG("%s","Connected to the database.");

            // Create Table.
            SACommand cmd;
            cmd.setConnection(&con);
            cmd.setCommandText("create table field_objects(id SERIAL PRIMARY KEY, type CHAR(4) NOT NULL, loc_x FLOAT, loc_y FLOAT, loc_z FLOAT, rot_x FLOAT, rot_y FLOAT, rot_z FLOAT);");
            cmd.Execute();

            con.Disconnect();
            LOG("%s","Disconnected from the database.");
        }
        catch(SAException &x)
        {
            con.Rollback();
            printf("%s\n", x.ErrText().GetMultiByteChars());
        }
    }

    void PutRandomArchersToDatabse( int inNumber )
    {
        Vector3 minPos( -3000.f, -3000.f, 230.f );
        Vector3 maxPos( 3000.f, 3000.f, 230.f );
        
        SAConnection con;
               
        try {
            con.Connect(_TSA("127.0.0.1,5432@sqlapi"), _TSA("postgres"), _TSA("password"), SA_PostgreSQL_Client);
            LOG("%s","Connected to the database.");

            // Insert Data.
            SACommand insert(&con, _TSA("INSERT INTO field_objects (type, loc_x, loc_y, loc_z) VALUES (:1, :2, :3, :4)"));


            for( int i = 0; i < inNumber; ++i )
            {
              Vector3 archerLcation = MMOMath::GetRandomVector( minPos, maxPos );
               insert << _TSA("ARCH") << archerLcation.mX << archerLcation.mY << archerLcation.mZ;
               insert.Execute();
            }
            
            con.Disconnect();
            LOG("%s","Disconnected from the database.");
        }
        catch(SAException &x)
        {
            con.Rollback();
            printf("%s\n", x.ErrText().GetMultiByteChars());
        }
    }
    
    void CreateArchersFromDB()
    {
        SAConnection con;
               
        try {
            con.Connect(_TSA("127.0.0.1,5432@sqlapi"), _TSA("postgres"), _TSA("password"), SA_PostgreSQL_Client);
            LOG("%s","Connected to the database.");
            
            SACommand select(&con, _TSA("SELECT id, type, loc_x, loc_y, loc_z FROM field_objects WHERE type = :1"));
            select << _TSA("ARCH");
            select.Execute();

            while(select.FetchNext()) {
                long id = select[1].asLong();
                SAString type = select[2].asString();
                double loc_x = select[3].asDouble();
                double loc_y = select[4].asDouble();
                double loc_z = select[5].asDouble();
                
                 printf("[%ld, %s] Location(%f, %f, %f) \n", id, type.GetMultiByteChars(), loc_x, loc_y, loc_z);
                
                Vector3 archerLocation;
                archerLocation.Set(loc_x, loc_y, loc_z);
                MochaObjectPtr go;
                
                go = GameObjectRegistry::sInstance->CreateGameObject( 'ARCH' );
                go->SetLocation( archerLocation );
            }
            
            con.Disconnect();
            LOG("%s","Disconnected from the database.");
        }
        catch(SAException &x)
        {
            con.Rollback();
            printf("%s\n", x.ErrText().GetMultiByteChars());
        }
    }

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
//    CreateFieldObjectRecord();
//    PutRandomArchersToDatabse(50);
    
    // There exists an error where number of object exceeds certain number,
    // which occurs at client side.
    
//    CreateArchersFromDB();
    
//    CreateRandomArcher( 10 );

//    CreateRandomArcher( 10 );
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

    cat->SetLocation( Vector3( 1.f - static_cast< float >( inPlayerId ), 0.f, 230.f ) );
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
