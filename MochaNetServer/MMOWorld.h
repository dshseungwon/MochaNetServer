#pragma once

class MMOWorld
{
public:

    static void StaticInit();

    static std::unique_ptr< MMOWorld >        sInstance;

    void AddGameObject( MochaObjectPtr inGameObject );
    void RemoveGameObject( MochaObjectPtr inGameObject );

    void Update();

    const std::vector< MochaObjectPtr >&    GetGameObjects()    const    { return mGameObjects; }

private:


    MMOWorld();

    int    GetIndexOfGameObject( MochaObjectPtr inGameObject );

    std::vector< MochaObjectPtr >    mGameObjects;
};
