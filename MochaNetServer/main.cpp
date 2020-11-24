//
//  main.cpp
//  MochaNetServer
//
//  Created by Seungwon Ju on 2020/11/21.
//

#include "MochaServerPCH.h"


const char** __argv;
int __argc;

void work(int t, int id) {
  printf("%d start \n", id);
  std::this_thread::sleep_for(std::chrono::seconds(t));
  printf("%d end after %ds\n", id, t);
}


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
