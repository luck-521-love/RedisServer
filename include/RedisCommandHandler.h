#ifndef REDIS_COMMAND_HANDLER_H
#define REDIS_COMMAND_HANDLER_H

#include <string>

class RedisCommandHandler
{

public:
    RedisCommandHandler();
    //Process a command form a client and return RESP-formatted reponse.
    std::string processCommand(const std::string& commandLine);
};

#endif