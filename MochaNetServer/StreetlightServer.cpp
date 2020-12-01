#include "MochaServerPCH.h"

StreetlightServer::StreetlightServer() :
    mPlayerId( 0 ),
    kClassId('SLIT')
{
}

void StreetlightServer::Update()
{
    
}

void StreetlightServer::HandleDying()
{
    NetworkManagerServer::sInstance->UnregisterGameObject( this );
}

uint32_t StreetlightServer::Write( OutputMemoryBitStream& inOutputStream, uint32_t inDirtyState ) const
{
    uint32_t written_state = 0;
    
    if( inDirtyState & ECRS_Pose )
    {
        inOutputStream.Write( (bool)true );
        Vector3 location = GetLocation();
        inOutputStream.Write( location.mX );
        inOutputStream.Write( location.mY );
        inOutputStream.Write( location.mZ );
        
        written_state |= ECRS_Pose;
    }
    else
    {
        inOutputStream.Write( (bool)false );
    }
    return written_state;

}
