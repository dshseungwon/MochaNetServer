#include "MMOPCH.h"

NetworkManager::NetworkManager() :
    mBytesSentThisFrame( 0 ),
    mDropPacketChance( 0.f ),
    mSimulatedLatency( 0.f ),
    mCurrentPacketIndex(0)
{
    mPool = std::make_shared<ThreadPool>(100);
    mPacketVector.reserve(10000);
}

NetworkManager::~NetworkManager()
{
}

bool NetworkManager::Init( uint16_t inPort )
{
    mSocket = SocketUtil::CreateUDPSocket( INET );
    SocketAddress ownAddress( INADDR_ANY, inPort );
    mSocket->Bind( ownAddress );

    LOG( "Initializing NetworkManager at port %d", inPort );

    mBytesReceivedPerSecond = WeightedTimedMovingAverage( 1.f );
    mBytesSentPerSecond = WeightedTimedMovingAverage( 1.f );

    //did we bind okay?
    if( mSocket == nullptr )
    {
        LOG( "Failed to bind the client socket.", inPort );
        return false;
    }

    if( mSocket->SetNonBlockingMode( true ) != NO_ERROR )
    {
        LOG( "Failed to set the socket as nonblocking mode.", inPort );
        return false;
    }

    return true;
}

void NetworkManager::ProcessIncomingPackets()
{
    ReadIncomingPacketsIntoQueue();

//    ProcessQueuedPackets();
    ProcessQueuedPacketsVector();

    UpdateBytesSentLastFrame();

}

void NetworkManager::ReadIncomingPacketsIntoQueue()
{
//    char packetMem[ 1500 ];
//    int packetSize = sizeof( packetMem );
//    Here exists a memory leak. Should be handled.

    //keep reading until we don't have anything to read ( or we hit a max number that we'll process per frame )
    int receivedPackedCount = 0;
    int totalReadByteCount = 0;

    while( receivedPackedCount < kMaxPacketsPerFrameCount )
    {
        char * packetMem = new char[1500]();
        int packetSize = 1500;
        
        InputMemoryBitStream inputStream( packetMem, packetSize * 8 );
        SocketAddress fromAddress;
        
        int readByteCount = mSocket->ReceiveFrom( packetMem, packetSize, fromAddress );
        if( readByteCount == 0 )
        {
            //nothing to read
            delete[] packetMem;
            break;
        }
        else if( readByteCount == -WSAECONNRESET )
        {
            //port closed on other end, so DC this person immediately
            delete[] packetMem;
            HandleConnectionReset( fromAddress );
        }
        else if( readByteCount > 0 )
        {
            LOG("packetMem: %p", packetMem);
            inputStream.ResetToCapacity( readByteCount );
            ++receivedPackedCount;
            totalReadByteCount += readByteCount;

            
            float simulatedReceivedTime = Timing::sInstance.GetTimef() + mSimulatedLatency;
//            mPacketQueue.emplace( simulatedReceivedTime, inputStream, fromAddress );
            mPacketVector.emplace_back( packetMem, simulatedReceivedTime, inputStream, fromAddress );
            
        }
        else
        {
            delete[] packetMem;
            LOG ("%s", "Error @ ReadIncomingPacketsIntoQueue.");
        }
    }

    if( totalReadByteCount > 0 )
    {
        mBytesReceivedPerSecond.UpdatePerSecond( static_cast< float >( totalReadByteCount ) );
    }
}

void NetworkManager::ProcessQueuedPacketsVector()
{
    while( mCurrentPacketIndex < mPacketVector.size() )
    {
        ReceivedPacket& nextPacket = mPacketVector[mCurrentPacketIndex];
        
        if( Timing::sInstance.GetTimef() >= nextPacket.GetReceivedTime() )
        {
            LOG("%s", "Packet Received!");

            mPool->EnqueueJob([&](){
                ProcessPacket( nextPacket.GetPacketMem(), nextPacket.GetPacketBuffer(), nextPacket.GetFromAddress() );
            });
            
//            ProcessPacket( nextPacket.GetPacketBuffer(), nextPacket.GetFromAddress() );

            mCurrentPacketIndex++;
//            mPacketVector.erase(mPacketVector.begin());
        }
        else
        {
            LOG("Current Server Time: %f", Timing::sInstance.GetTimef());
            LOG("Packet Received Time: %f", nextPacket.GetReceivedTime());
            break;
        }
    }
}

void NetworkManager::ProcessQueuedPackets()
{
    while( !mPacketQueue.empty() )
    {
        ReceivedPacket& nextPacket = mPacketQueue.front();
        
        if( Timing::sInstance.GetTimef() >= nextPacket.GetReceivedTime() )
        {
            LOG("%s", "Packet Received!");

            mPool->EnqueueJob([&](){
                ProcessPacket( nextPacket.GetPacketMem(), nextPacket.GetPacketBuffer(), nextPacket.GetFromAddress() );
            });

//            ProcessPacket( nextPacket.GetPacketBuffer(), nextPacket.GetFromAddress() );
            
            mPacketQueue.pop();
        }
        else
        {
            LOG("Current Server Time: %f", Timing::sInstance.GetTimef());
            LOG("Packet Received Time: %f", nextPacket.GetReceivedTime());
            break;
        }
    
    }
//    mPool->Join();
}

void NetworkManager::SendPacket( const OutputMemoryBitStream& inOutputStream, const SocketAddress& inFromAddress )
{
//    LOG ("%s", "SEND PACKET!");
    int sentByteCount = mSocket->SendTo( inOutputStream.GetBufferPtr(), inOutputStream.GetByteLength(), inFromAddress );
    if( sentByteCount > 0 )
    {
        mBytesSentThisFrame += sentByteCount;
    }
}

void NetworkManager::UpdateBytesSentLastFrame()
{
    if( mBytesSentThisFrame > 0 )
    {
        mBytesSentPerSecond.UpdatePerSecond( static_cast< float >( mBytesSentThisFrame ) );

        mBytesSentThisFrame = 0;
    }

}


NetworkManager::ReceivedPacket::ReceivedPacket( char* packetMem, float inReceivedTime, InputMemoryBitStream& ioInputMemoryBitStream, const SocketAddress& inFromAddress ) :
    mPacketMem (packetMem),
    mReceivedTime( inReceivedTime ),
    mPacketBuffer( ioInputMemoryBitStream ),
    mFromAddress( inFromAddress )
{
}


void NetworkManager::AddToNetworkIdToGameObjectMap( MochaObjectPtr inGameObject )
{
    mNetworkIdToGameObjectMap[ inGameObject->GetNetworkId() ] = inGameObject;
}

void NetworkManager::RemoveFromNetworkIdToGameObjectMap( MochaObjectPtr inGameObject )
{
    mNetworkIdToGameObjectMap.erase( inGameObject->GetNetworkId() );
}
