#include "MMOPCH.h"

float kDesiredFrameTime = 0.0166f;

using namespace std::chrono;

Timing    Timing::sInstance;

namespace
{
    high_resolution_clock::time_point sStartTime;
}

Timing::Timing()
{

    sStartTime = high_resolution_clock::now();

}

void Timing::Update()
{

    double currentTime = GetTime();

    mDeltaTime = ( float ) ( currentTime - mLastFrameStartTime );

    //frame lock at 60fps
    while( mDeltaTime < kDesiredFrameTime )
    {
        currentTime = GetTime();

        mDeltaTime = (float)( currentTime - mLastFrameStartTime );
    }
    
    mLastFrameStartTime = currentTime;
    mFrameStartTimef = static_cast< float > ( mLastFrameStartTime );

}

double Timing::GetTime() const
{
    auto now = high_resolution_clock::now();
    auto ms = duration_cast< milliseconds >( now - sStartTime ).count();
    //a little uncool to then convert into a double just to go back, but oh well.
    return static_cast< double >( ms ) / 1000;
}

unsigned long Timing::GetTimeSinceEpoch() const
{
//    auto time_since_epoch = system_clock::now().time_since_epoch();
//    auto ms = duration_cast< milliseconds >(time_since_epoch).count();
//    return static_cast< float >(ms);
    
    unsigned long milliseconds_since_epoch =
        std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::system_clock::now().time_since_epoch()).count();
    
    return milliseconds_since_epoch;
}
