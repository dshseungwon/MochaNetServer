#include "MMOPCH.h"

bool MMOMove::Write( OutputMemoryBitStream& inOutputStream ) const
{
    mInputState.Write( inOutputStream );
    inOutputStream.Write( mTimestamp );

    return true;
}

bool MMOMove::Read( InputMemoryBitStream& inInputStream )
{
    mInputState.Read( inInputStream );
    inInputStream.Read( mTimestamp );

    return true;
}
