//
//  DatabaseManager.cpp
//  MochaNetServer
//
//  Created by Seungwon Ju on 27/11/2020.
//

#include "MochaServerPCH.h"

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
        printf("%s","Connected to the database.\n");
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

void DatabaseManager::CreateUserRecord()
{
    try
    {
        if (mConnection.isConnected())
        {
            // Create Table.
            SACommand cmd;
            cmd.setConnection(&mConnection);
            cmd.setCommandText("create table users(id SERIAL PRIMARY KEY, name VARCHAR(20), identification VARCHAR(20), password VARCHAR(20));");
            cmd.Execute();
        }
        else
        {
            printf("Database is not connected.\n");
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
    
    try
    {
        if (mConnection.isConnected())
        {
            SACommand select(&mConnection, _TSA("SELECT name FROM users WHERE identification = :1 and password = :2"));
            select << _TSA(id.c_str()) << _TSA(pw.c_str());
            select.Execute();
            
            // No user exists. Return errorcode.
            if (select.FieldCount() == 0)
            {
                ret.resultCode = 2;
                ret.name = "No user exists.";
            }
            
            // User exists.
            else
            {
                select.FetchFirst();
                
                SAString name = select[1].asString();
                
                ret.resultCode = 0;
                ret.name = name.GetMultiByteChars();
            }
        }
        else
        {
            printf("Database is not connected.\n");
        }
    }
    catch(SAException &x)
    {
        mConnection.Rollback();
        printf("%s\n", x.ErrText().GetMultiByteChars());
        
        ret.resultCode = -1;
        ret.name = "Database Error.";
    }
    return ret;
}


DBAuthResult DatabaseManager::GetClientNameBySignUpDB(std::string id, std::string pw, std::string name)
{
    struct DBAuthResult ret;
    
    try
    {
        if (mConnection.isConnected())
        {
            SACommand select(&mConnection, _TSA("SELECT COUNT(*) FROM users WHERE identification = :1"));
            select << _TSA(id.c_str());
            select.Execute();
            
            select.FetchFirst();
            long count = select[1].asLong();
            
            // User already exists. Return errorcode.
            if (count > 0)
            {
                ret.resultCode = 1;
                ret.name = "User Already Exists.";
            }
            
            // It's Okay to do the signup.
            else
            {
                SACommand signup(&mConnection, _TSA("INSERT INTO users (name, identification, password) VALUES (:1, :2, :3)"));
                signup << _TSA(name.c_str()) << _TSA(id.c_str()) << _TSA(pw.c_str());
                signup.Execute();
                
                ret.resultCode = 0;
                ret.name = name;
            }
        }
        else
        {
            printf("Database is not connected.\n");

        }
    }
    catch(SAException &x)
    {
        mConnection.Rollback();
        printf("%s\n", x.ErrText().GetMultiByteChars());
        
        ret.resultCode = -1;
        ret.name = "Database Error.";
    }
    
    return ret;
}

void DatabaseManager::CreateFieldObjectRecord()
{
    SAConnection con;
    try
    {
        con.Connect(_TSA("127.0.0.1,5432@sqlapi"), _TSA("postgres"), _TSA("password"), SA_PostgreSQL_Client);
        LOG("%s","Connected to the database.");

        // Create Table.
        SACommand cmd;
        cmd.setConnection(&con);
        cmd.setCommandText("create table field_objects(id SERIAL PRIMARY KEY, type CHAR(4) NOT NULL, loc_x FLOAT, loc_y FLOAT, loc_z FLOAT, rot_x FLOAT, rot_y FLOAT, rot_z FLOAT);");
        cmd.Execute();

        con.Disconnect();
        LOG("%s","Disconnected from the database.");
    }
    catch(SAException &x)
    {
        con.Rollback();
        printf("%s\n", x.ErrText().GetMultiByteChars());
    }
}

void DatabaseManager::PutRandomArchersToDatabse( int inNumber )
{
    Vector3 minPos( -3000.f, -3000.f, 230.f );
    Vector3 maxPos( 3000.f, 3000.f, 230.f );
    
    SAConnection con;
           
    try
    {
        con.Connect(_TSA("127.0.0.1,5432@sqlapi"), _TSA("postgres"), _TSA("password"), SA_PostgreSQL_Client);
        LOG("%s","Connected to the database.");

        // Insert Data.
        SACommand insert(&con, _TSA("INSERT INTO field_objects (type, loc_x, loc_y, loc_z) VALUES (:1, :2, :3, :4)"));


        for( int i = 0; i < inNumber; ++i )
        {
          Vector3 archerLcation = MMOMath::GetRandomVector( minPos, maxPos );
           insert << _TSA("ARCH") << archerLcation.mX << archerLcation.mY << archerLcation.mZ;
           insert.Execute();
        }
        
        con.Disconnect();
        LOG("%s","Disconnected from the database.");
    }
    catch(SAException &x)
    {
        con.Rollback();
        printf("%s\n", x.ErrText().GetMultiByteChars());
    }
}

void DatabaseManager::CreateArchersFromDB()
{
    SAConnection con;
           
    try
    {
        con.Connect(_TSA("127.0.0.1,5432@sqlapi"), _TSA("postgres"), _TSA("password"), SA_PostgreSQL_Client);
        LOG("%s","Connected to the database.");
        
        SACommand select(&con, _TSA("SELECT id, type, loc_x, loc_y, loc_z FROM field_objects WHERE type = :1"));
        select << _TSA("ARCH");
        select.Execute();

        while(select.FetchNext()) {
            long id = select[1].asLong();
            SAString type = select[2].asString();
            double loc_x = select[3].asDouble();
            double loc_y = select[4].asDouble();
            double loc_z = select[5].asDouble();
            
             printf("[%ld, %s] Location(%f, %f, %f) \n", id, type.GetMultiByteChars(), loc_x, loc_y, loc_z);
            
            Vector3 archerLocation;
            archerLocation.Set(loc_x, loc_y, loc_z);
            MochaObjectPtr go;
            
            go = GameObjectRegistry::sInstance->CreateGameObject( 'ARCH' );
            go->SetLocation( archerLocation );
        }
        
        con.Disconnect();
        LOG("%s","Disconnected from the database.");
    }
    catch(SAException &x)
    {
        con.Rollback();
        printf("%s\n", x.ErrText().GetMultiByteChars());
    }
}
