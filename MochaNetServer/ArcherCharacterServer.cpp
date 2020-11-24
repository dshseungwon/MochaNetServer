#include "MochaServerPCH.h"

ArcherCharacterServer::ArcherCharacterServer() :
    mPlayerId( 0 ),
    kClassId('ARCH')
{
}

void ArcherCharacterServer::Update()
{
    
}

uint32_t ArcherCharacterServer::Write( OutputMemoryBitStream& inOutputStream, uint32_t inDirtyState ) const
{
    uint32_t written_state = 0;
    
    Vector3 location = GetLocation();
    inOutputStream.Write( location.mX );
    inOutputStream.Write( location.mY );
    inOutputStream.Write( location.mZ );
    
    return written_state;

}
