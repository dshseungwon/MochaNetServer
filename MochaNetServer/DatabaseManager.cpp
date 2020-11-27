//
//  DatabaseManager.cpp
//  MochaNetServer
//
//  Created by Seungwon Ju on 27/11/2020.
//

#include "DatabaseManager.h"

std::unique_ptr<DatabaseManager>    DatabaseManager::sInstance;

void DatabaseManager::StaticInit()
{
    sInstance.reset ( new DatabaseManager() );
}

void DatabaseManager::ConnectToDB(std::string address, std::string id, std::string passwd)
{
    try
    {
        mConnection.Connect(_TSA(address.c_str()), _TSA(id.c_str()), _TSA(passwd.c_str()), SA_PostgreSQL_Client);
        printf("%s","Connected to the database.");
    }
    catch(SAException &x)
    {
        mConnection.Rollback();
        printf("%s\n", x.ErrText().GetMultiByteChars());
    }
}

void DatabaseManager::ExecuteCommand(std::string command)
{
    try
    {
        SACommand cmd;
        cmd.setConnection(&mConnection);
        cmd.setCommandText(command.c_str());
        cmd.Execute();
    }
    catch(SAException &x)
    {
        mConnection.Rollback();
        printf("%s\n", x.ErrText().GetMultiByteChars());
    }
}

void DatabaseManager::DisconnectFromDB()
{
    try
    {
        if (mConnection.isConnected())
        {
            mConnection.Disconnect();
            printf("Disconnected from the database.\n");
        }
        else
        {
            printf("Not connected to database.\n");
        }
    }
    catch(SAException &x)
    {
        mConnection.Rollback();
        printf("%s\n", x.ErrText().GetMultiByteChars());
    }
}


DBAuthResult DatabaseManager::GetClientNameByLogInDB(std::string id, std::string pw)
{
    struct DBAuthResult ret;
    return ret;
}
DBAuthResult DatabaseManager::GetClientNameBySignUpDB(std::string id, std::string pw, std::string name)
{
    struct DBAuthResult ret;
    ret.resultCode = 0;
    ret.name = "Seungwon";
    return ret;
}
