#include "MochaServerPCH.h"

const float HALF_WORLD_HEIGHT = 100000.f;
const float HALF_WORLD_WIDTH = 100000.f;

FirstFantasyCharacterServer::FirstFantasyCharacterServer() :
    mCatControlType( ESCT_Human ),
    mTimeOfNextShot( 0.f ),
    mTimeBetweenShots( 3.f ),
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
    float inputForwardDelta = inInputState.GetDesiredVerticalDelta();
    mThrustDirForward = inputForwardDelta;

    float inputRightDelta = inInputState.GetDesiredHorizontalDelta();
    mThrustDirRight = inputRightDelta;
    
    mIsShooting = inInputState.IsShooting();

}

void FirstFantasyCharacterServer::AdjustVelocityByThrust( float inDeltaTime, const MMOInputState& inInputState )
{
    Vector3 forwardVector = GetForwardVector();
    Vector3 rightVector = GetRightVector();
    
    Vector3 forward = forwardVector * ( mThrustDirForward * inDeltaTime * mMaxLinearSpeed );
    Vector3 right = rightVector * ( mThrustDirRight * inDeltaTime * mMaxLinearSpeed );
    
    mVelocity = forward + right;
    
}

void FirstFantasyCharacterServer::SimulateMovement( float inDeltaTime, const MMOInputState& inInputState )
{
    AdjustVelocityByThrust( inDeltaTime, inInputState );

    SetLocation( GetLocation() + mVelocity * inDeltaTime );

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
    
    if( mThrustDirRight != 0.f )
    {
        inOutputStream.Write( true );
        inOutputStream.Write( mThrustDirRight > 0.f );
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
        
        // SEND RPC PACKET
        MochaObjectPtr go;
        go = GameObjectRegistry::sInstance->CreateGameObject( 'EXPR' );
        
        Vector3 InvokeLocation = GetLocation() + (GetForwardVector() * 1000);
        go->SetLocation( InvokeLocation );
        go->SetCreationTime(Timing::sInstance.GetTimeSinceEpoch());
        
        
        for( auto goIt = MMOWorld::sInstance->GetGameObjects().begin(), end = MMOWorld::sInstance->GetGameObjects().end(); goIt != end; ++goIt )
        {
            IMochaObject* target = goIt->get();
            if( target->GetClassId() == 'ARCH' && !target->DoesWantToDie() )
            {
                //simple collision test for spheres- are the radii summed less than the distance?
                Vector3 targetLocation = target->GetLocation();

                Vector3 delta = targetLocation - InvokeLocation;
                float distSq = delta.LengthSq2D();
                
//                printf("distSq: %f\n", distSq);
                if (distSq < 20000)
                {
                    target->SetDoesWantToDie( true );
                    printf("Destroy %d", target->GetClassId());
                }
            }
        }
        // go->SetDoesWantToDie(true);
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
