#include "MochaServerPCH.h"

const float HALF_WORLD_HEIGHT = 100000.f;
const float HALF_WORLD_WIDTH = 100000.f;

FirstFantasyCharacterServer::FirstFantasyCharacterServer() :
    mCatControlType( ESCT_Human ),
    mTimeOfNextShot( 0.f ),
    mTimeBetweenShots( 0.2f ),
    mMaxRotationSpeed( 5.f ),
    mMaxLinearSpeed( 6000.f ),
    mVelocity( Vector3::Zero ),
    mWallRestitution( 0.1f ),
    mCatRestitution( 0.1f ),
    mThrustDirForward( 0.f ),
    mThrustDirRight( 0.f ),
    mPlayerId( 0 ),
    mIsShooting( false ),
    mHealth( 10 ),
    kClassId('PLYR')
{
    SetCollisionRadius( 0.5f );
}

void FirstFantasyCharacterServer::ProcessInput( float inDeltaTime, const MMOInputState& inInputState )
{
    //process our input....

    //turning...
//    float newRotation = GetRotation() + inInputState.GetDesiredHorizontalDelta() * mMaxRotationSpeed * inDeltaTime;
//    SetRotation( newRotation );

    //moving...
    float inputForwardDelta = inInputState.GetDesiredVerticalDelta();
    mThrustDirForward = inputForwardDelta;

    float inputRightDelta = inInputState.GetDesiredHorizontalDelta();
    mThrustDirRight = inputRightDelta;
    
    mIsShooting = inInputState.IsShooting();

}

void FirstFantasyCharacterServer::AdjustVelocityByThrust( float inDeltaTime, const MMOInputState& inInputState )
{
    //just set the velocity based on the thrust direction -- no thrust will lead to 0 velocity
    //simulating acceleration makes the client prediction a bit more complex
    Vector3 forwardVector = GetForwardVector();
    Vector3 rightVector = GetRightVector();
    
    Vector3 forward = forwardVector * ( mThrustDirForward * inDeltaTime * mMaxLinearSpeed );
    Vector3 right = rightVector * ( mThrustDirRight * inDeltaTime * mMaxLinearSpeed );
    
    mVelocity = forward + right;
    
}

void FirstFantasyCharacterServer::SimulateMovement( float inDeltaTime, const MMOInputState& inInputState )
{
    //simulate us...
    AdjustVelocityByThrust( inDeltaTime, inInputState );

    SetLocation( GetLocation() + mVelocity * inDeltaTime );

    //ProcessCollisions();
}

void FirstFantasyCharacterServer::ProcessCollisionsWithScreenWalls()
{
    Vector3 location = GetLocation();
    float x = location.mX;
    float y = location.mY;

    float vx = mVelocity.mX;
    float vy = mVelocity.mY;

    float radius = GetCollisionRadius();

    //if the cat collides against a wall, the quick solution is to push it off
    if( ( y + radius ) >= HALF_WORLD_HEIGHT && vy > 0 )
    {
        mVelocity.mY = -vy * mWallRestitution;
        location.mY = HALF_WORLD_HEIGHT - radius;
        SetLocation( location );
    }
    else if( y <= ( -HALF_WORLD_HEIGHT - radius ) && vy < 0 )
    {
        mVelocity.mY = -vy * mWallRestitution;
        location.mY = -HALF_WORLD_HEIGHT - radius;
        SetLocation( location );
    }

    if( ( x + radius ) >= HALF_WORLD_WIDTH && vx > 0 )
    {
        mVelocity.mX = -vx * mWallRestitution;
        location.mX = HALF_WORLD_WIDTH - radius;
        SetLocation( location );
    }
    else if(  x <= ( -HALF_WORLD_WIDTH - radius ) && vx < 0 )
    {
        mVelocity.mX = -vx * mWallRestitution;
        location.mX = -HALF_WORLD_WIDTH - radius;
        SetLocation( location );
    }
}

uint32_t FirstFantasyCharacterServer::Write( OutputMemoryBitStream& inOutputStream, uint32_t inDirtyState ) const
{
    uint32_t writtenState = 0;

    if( inDirtyState & ECRS_PlayerId )
    {
        inOutputStream.Write( (bool)true );
        inOutputStream.Write( GetPlayerId() );

        writtenState |= ECRS_PlayerId;
    }
    else
    {
        inOutputStream.Write( (bool)false );
    }


    if( inDirtyState & ECRS_Pose )
    {
        inOutputStream.Write( (bool)true );

        Vector3 velocity = mVelocity;
        inOutputStream.Write( velocity.mX );
        inOutputStream.Write( velocity.mY );
        inOutputStream.Write( velocity.mZ );

        Vector3 location = GetLocation();
        inOutputStream.Write( location.mX );
        inOutputStream.Write( location.mY );
        inOutputStream.Write( location.mZ );

        inOutputStream.Write( GetRotation() );

        writtenState |= ECRS_Pose;
    }
    else
    {
        inOutputStream.Write( (bool)false );
    }

    //always write mThrustDir- it's just two bits
    if( mThrustDirForward != 0.f )
    {
        inOutputStream.Write( true );
        inOutputStream.Write( mThrustDirForward > 0.f );
    }
    else
    {
        inOutputStream.Write( false );
    }

    if( inDirtyState & ECRS_Color )
    {
        inOutputStream.Write( (bool)true );
        inOutputStream.Write( GetColor() );

        writtenState |= ECRS_Color;
    }
    else
    {
        inOutputStream.Write( (bool)false );
    }

    if( inDirtyState & ECRS_Health )
    {
        inOutputStream.Write( (bool)true );
        inOutputStream.Write( mHealth, 4 );

        writtenState |= ECRS_Health;
    }
    else
    {
        inOutputStream.Write( (bool)false );
    }

    return writtenState;
    

}





void FirstFantasyCharacterServer::HandleDying()
{
    NetworkManagerServer::sInstance->UnregisterGameObject( this );
}

void FirstFantasyCharacterServer::Update()
{
    IMochaObject::Update();
    
    Vector3 oldLocation = GetLocation();
    Vector3 oldVelocity = GetVelocity();
    float oldRotation = GetRotation();

    ClientProxyPtr client = NetworkManagerServer::sInstance->GetClientProxy( GetPlayerId() );
    if( client )
    {
        MoveList& moveList = client->GetUnprocessedMoveList();
        for( const MMOMove& unprocessedMove : moveList )
        {
            const MMOInputState& currentState = unprocessedMove.GetInputState();
            float deltaTime = unprocessedMove.GetDeltaTime();
            ProcessInput( deltaTime, currentState );
            SimulateMovement( deltaTime, currentState );
        }

        moveList.Clear();
    }

    HandleShooting();

    if( !MMOMath::Is2DVectorEqual( oldLocation, GetLocation() ) ||
        !MMOMath::Is2DVectorEqual( oldVelocity, GetVelocity() ) ||
        oldRotation != GetRotation() )
    {
        NetworkManagerServer::sInstance->SetStateDirty( GetNetworkId(), ECRS_Pose );
    }
}

void FirstFantasyCharacterServer::HandleShooting()
{
    float time = Timing::sInstance.GetFrameStartTime();
    if( mIsShooting && Timing::sInstance.GetFrameStartTime() > mTimeOfNextShot )
    {
        //not exact, but okay
        mTimeOfNextShot = time + mTimeBetweenShots;

        //fire!
        //YarnPtr yarn = std::static_pointer_cast< Yarn >( GameObjectRegistry::sInstance->CreateGameObject( 'YARN' ) );
        //yarn->InitFromShooter( this );
    }
}

void FirstFantasyCharacterServer::TakeDamage( int inDamagingPlayerId )
{
    mHealth--;
    if( mHealth <= 0.f )
    {
        //score one for damaging player...
        //ScoreBoardManager::sInstance->IncScore( inDamagingPlayerId, 1 );

        //and you want to die
        SetDoesWantToDie( true );

        //tell the client proxy to make you a new cat
        ClientProxyPtr clientProxy = NetworkManagerServer::sInstance->GetClientProxy( GetPlayerId() );
        if( clientProxy )
        {
            clientProxy->HandleCatDied();
        }
    }

    //tell the world our health dropped
    NetworkManagerServer::sInstance->SetStateDirty( GetNetworkId(), ECRS_Health );
}
