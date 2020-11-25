#pragma once

typedef unordered_map< int, MochaObjectPtr > IntToGameObjectMap;

class NetworkManager
{
public:
    static const uint32_t    kHelloCC = 'HELO';
    static const uint32_t    kWelcomeCC = 'WLCM';
    static const uint32_t    kStateCC = 'STAT';
    static const uint32_t    kInputCC = 'INPT';
    static const int        kMaxPacketsPerFrameCount = 10;

    NetworkManager();
    virtual ~NetworkManager();

    bool    Init( uint16_t inPort, bool useMultiThreading, int numThreads = 4);
    void    ProcessIncomingPackets();

    virtual void    ProcessPacket( char* packetMem, InputMemoryBitStream& inInputStream, const SocketAddress& inFromAddress ) = 0;
    virtual void    HandleConnectionReset( const SocketAddress& inFromAddress ) { ( void ) inFromAddress; }

            void    SendPacket( const OutputMemoryBitStream& inOutputStream, const SocketAddress& inFromAddress );

            const WeightedTimedMovingAverage& GetBytesReceivedPerSecond()    const    { return mBytesReceivedPerSecond; }
            const WeightedTimedMovingAverage& GetBytesSentPerSecond()        const    { return mBytesSentPerSecond; }

            void    SetDropPacketChance( float inChance )    { mDropPacketChance = inChance; }
            float    GetDropPacketChance() const                { return mDropPacketChance; }
            void    SetSimulatedLatency( float inLatency )    { mSimulatedLatency = inLatency; }
            float    GetSimulatedLatency() const                { return mSimulatedLatency; }

            inline    MochaObjectPtr    GetGameObject( int inNetworkId ) const;
            void    AddToNetworkIdToGameObjectMap( MochaObjectPtr inGameObject );
            void    RemoveFromNetworkIdToGameObjectMap( MochaObjectPtr inGameObject );

protected:

    IntToGameObjectMap        mNetworkIdToGameObjectMap;

private:

    class ReceivedPacket
    {
    public:
        ReceivedPacket( char* packetMem, float inReceivedTime, InputMemoryBitStream& inInputMemoryBitStream, const SocketAddress& inAddress );

        char*                      GetPacketMem() { return mPacketMem; }
        const    SocketAddress&            GetFromAddress()    const    { return mFromAddress; }
                float                    GetReceivedTime()    const    { return mReceivedTime; }
                InputMemoryBitStream&    GetPacketBuffer()            { return mPacketBuffer; }

    private:
        char*                    mPacketMem;
        float                    mReceivedTime;
        InputMemoryBitStream    mPacketBuffer;
        SocketAddress            mFromAddress;

    };

    void    UpdateBytesSentLastFrame();
    void    ReadIncomingPacketsIntoQueue();
    void    ProcessQueuedPackets();
    
    void    ProcessQueuedPacketsVector();

    queue< ReceivedPacket, list< ReceivedPacket > >    mPacketQueue;

    UDPSocketPtr    mSocket;

    WeightedTimedMovingAverage    mBytesReceivedPerSecond;
    WeightedTimedMovingAverage    mBytesSentPerSecond;

    int                            mBytesSentThisFrame;

    float                        mDropPacketChance;
    float                        mSimulatedLatency;
    
protected:
    bool                    bMultiThreading;
    int                     mNumThreads;
    
    std::shared_ptr<ThreadPool>         mPool;
    vector< ReceivedPacket > mPacketVector;
    int                      mCurrentPacketIndex;
};

    
    
inline MochaObjectPtr NetworkManager::GetGameObject( int inNetworkId ) const
{
    auto gameObjectIt = mNetworkIdToGameObjectMap.find( inNetworkId );
    if( gameObjectIt != mNetworkIdToGameObjectMap.end() )
    {
        return gameObjectIt->second;
    }
    else
    {
        return MochaObjectPtr();
    }
}
