//
//  ExplosionRPCServer.cpp
//  MochaNetServer
//
//  Created by Seungwon Ju on 2020/12/01.
//

#include "MochaServerPCH.h"

ExplosionRPCServer::ExplosionRPCServer() :
    mPlayerId( 0 ),
    kClassId('EXPR')
{
}

void ExplosionRPCServer::Update()
{
    
}

void ExplosionRPCServer::HandleDying()
{
    NetworkManagerServer::sInstance->UnregisterGameObject(this);
}

uint32_t ExplosionRPCServer::Write( OutputMemoryBitStream& inOutputStream, uint32_t inDirtyState ) const
{
    uint32_t written_state = 0;

    if( inDirtyState & State_Pose )
    {
        inOutputStream.Write( (bool)true );
        Vector3 location = GetLocation();
        inOutputStream.Write( location.mX );
        inOutputStream.Write( location.mY );
        inOutputStream.Write( location.mZ );
        
        inOutputStream.Write(GetCreationTime());
        
        written_state |= State_Pose;
    }
    else
    {
        inOutputStream.Write( (bool)false );
    }
    return written_state;

}
