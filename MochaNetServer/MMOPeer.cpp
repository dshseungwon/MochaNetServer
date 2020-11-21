#include "MMOPCH.h"
#include <time.h>

std::unique_ptr< AMMOPeer >     AMMOPeer::sInstance;

// Sets default values
AMMOPeer::AMMOPeer() :
mShouldKeepRunning( true )
{
    SocketUtil::StaticInit();

    srand( static_cast< uint32_t >( time( nullptr ) ) );
    
    GameObjectRegistry::StaticInit();

    MMOWorld::StaticInit();
}

AMMOPeer::~AMMOPeer()
{
    SocketUtil::CleanUp();
}

void AMMOPeer::Run()
{
    DoRunLoop();
}

// For Server
void AMMOPeer::DoRunLoop()
{
    BeginPlay();
    
    while( mShouldKeepRunning )
    {
        Timing::sInstance.Update();

        Tick();
    }
}

void AMMOPeer::BeginPlay()
{

}

void AMMOPeer::Tick()
{
    MMOWorld::sInstance->Update();
}


