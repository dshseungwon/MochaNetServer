//
//  main.cpp
//  MochaNetServer
//
//  Created by Seungwon Ju on 2020/11/21.
//

#include "MochaServerPCH.h"

const char** __argv;
int __argc;

int main(int argc, const char * argv[]) {
    
    __argc = argc;
    __argv = argv;
    
    if (Server::StaticInit())
    {
        Server::sInstance->Run();
    }
    else
    {
        return 1;
    }
    return 0;
}
