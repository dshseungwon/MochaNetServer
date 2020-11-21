#pragma once


class AMMOPeer
{
	
public:	
	// Sets default values for this actor's properties
	AMMOPeer();
    virtual ~AMMOPeer();
    
    static std::unique_ptr< AMMOPeer >   sInstance;

    virtual void        Run();
    void            SetShouldKeepRunning( bool inShouldKeepRunning ) { mShouldKeepRunning = inShouldKeepRunning; }

protected:
	// Called when the game starts or when spawned
    virtual void BeginPlay();
    
    virtual void Tick();
    
private:
    void        DoRunLoop();

    bool    mShouldKeepRunning;

};
