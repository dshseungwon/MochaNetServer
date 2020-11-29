#pragma once

struct DBAuthResult
{
    int resultCode;
    std::string name;
};

class DatabaseManager
{
public:
    static void StaticInit();
    DatabaseManager();
    ~DatabaseManager();

    static std::unique_ptr< DatabaseManager >        sInstance;
    
    void ConnectToDB(std::string address, std::string id, std::string passwd);
    
    void ExecuteCommand(std::string command);
    
    void DisconnectFromDB();
    
    DBAuthResult GetClientNameByLogInDB(std::string id, std::string pw);
    
    DBAuthResult GetClientNameBySignUpDB(std::string id, std::string pw, std::string name);
    
    void CreateUserRecord();

    void CreateFieldObjectRecord();
    void PutRandomArchersToDatabse( int inNumber );
    void CreateArchersFromDB();

private:
    SAConnection mConnection;
    
    std::string mDBAddress;
    std::string mDBId;
    std::string mDBPasswd;
};
