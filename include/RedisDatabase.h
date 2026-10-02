#ifndef REDIS_DATABASE_H
#define REDIS_DATABASE_H


#include <string>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <chrono>

class RedisDatabase
{
public:
    // GET the singleton instance
    static RedisDatabase& getInstance();

    // Common Comands
    bool flushAll();

    // Key/Value Operations
    void set(const std::string& Key, const std::string& value);
    bool get(const std::string& Key, std::string& value);
    std::vector<std::string> Keys();
    std::string type(const std::string& Key);
    bool del(const std::string& Key);
    //expire
    bool expire(const std::string& Key, int seconds);
    //rename
    bool rename(const std::string& oldKey, const std::string& newKey);
    // Persistance: Dump / load the database from a file
    bool dump(const std::string& filename);
    bool load(const std::string& filename);


private:
    RedisDatabase() = default;
    ~RedisDatabase() = default;
    RedisDatabase(const RedisDatabase&) = delete;
    RedisDatabase& operator = (const RedisDatabase&) = delete;

    std::mutex db_mutex;
    std::unordered_map<std::string, std::string> kv_store;
    std::unordered_map<std::string, std::vector<std::string>> list_store;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hash_store;

    std::unordered_map<std::string, std::chrono::steady_clock::time_point> expire_map;
    int seconds;

};


#endif 