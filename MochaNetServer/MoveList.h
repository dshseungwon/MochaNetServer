#pragma once

class MoveList
{
public:

    typedef deque< MMOMove >::const_iterator            const_iterator;
    typedef deque< MMOMove >::const_reverse_iterator    const_reverse_iterator;
    
    MoveList():
        mLastMoveTimestamp( -1.f )
    {}
    
    const    MMOMove&    AddMove( const MMOInputState& inInputState, float inTimestamp );
            bool    AddMove( const MMOMove& inMove );

            void    RemovedProcessedMoves( float inLastMoveProcessedOnServerTimestamp );

    float            GetLastMoveTimestamp()    const    { return mLastMoveTimestamp; }

    const MMOMove&        GetLatestMove()            const    { return mMoves.back(); }

    void            Clear()                            { mMoves.clear(); }
    bool            HasMoves()                const    { return !mMoves.empty(); }
    int                GetMoveCount()            const    { return mMoves.size(); }

    //for for each, we have to match stl calling convention
    const_iterator    begin()                    const    { return mMoves.begin(); }
    const_iterator    end()                    const    { return mMoves.end(); }

    const MMOMove&        operator[]( size_t i )    const    { return mMoves[ i ]; }
private:

    float            mLastMoveTimestamp;
    deque< MMOMove >    mMoves;




};

