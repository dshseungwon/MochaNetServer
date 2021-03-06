#pragma once

class Server : public AMMOPeer
{
public:

    static bool StaticInit();

    virtual void Tick() override;

    virtual void Run() override;

    void HandleNewClient( ClientProxyPtr inClientProxy );
    void HandleLostClient( ClientProxyPtr inClientProxy );

    shared_ptr< FirstFantasyCharacterServer >    GetCatForPlayer( int inPlayerId );
    void    SpawnPlayer( int inPlayerId );


private:
    Server();

    bool    InitNetworkManager(bool useMultiThreading);
    bool    InitNetworkManager(bool useMultiThreading, int numThreads);
    
    void    SetupWorld();

};
