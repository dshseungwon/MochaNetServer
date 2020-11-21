#pragma once

class MMOMove
{
public:

    MMOMove() {}

    MMOMove( const MMOInputState& inInputState, float inTimestamp, float inDeltaTime ) :
        mInputState( inInputState ),
        mTimestamp( inTimestamp ),
        mDeltaTime( inDeltaTime )
    {}


    const MMOInputState&    GetInputState()    const        { return mInputState; }
    float                GetTimestamp()    const        { return mTimestamp; }
    float                GetDeltaTime()    const        { return mDeltaTime; }

    bool Write( OutputMemoryBitStream& inOutputStream ) const;
    bool Read( InputMemoryBitStream& inInputStream );

private:
    MMOInputState    mInputState;
    float        mTimestamp;
    float        mDeltaTime;

};


