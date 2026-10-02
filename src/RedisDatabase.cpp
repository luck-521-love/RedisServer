#include "../include/RedisDatabase.h"


#include <fstream>
#include <sstream> 

RedisDatabase& RedisDatabase::getInstance()
{
    static RedisDatabase instance;
    return instance;
}

// Common Comands
bool RedisDatabase::flushAll()
{
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store.clear();
    list_store.clear();
    hash_store.clear();
    return true;
}

// Key/Value Operations
void RedisDatabase::set(const std::string& Key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store[Key] = value;
}
bool RedisDatabase::get(const std::string& Key, std::string& value)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = kv_store.find(Key);
    if (it != kv_store.end())
    {
        value = it->second;
        return true;
    }
    return false;
}
std::vector<std::string> RedisDatabase::Keys()
{
    std::lock_guard<std::mutex> lock(db_mutex);
    std::vector<std::string> result;
    for (const auto& pair : kv_store)
    {
        result.push_back(pair.first);
    }
    for (const auto& pair : list_store)
    {
        result.push_back(pair.first);
    }
    for (const auto& pair : hash_store)
    {
        result.push_back(pair.first);
    }
    return result;
}
std::string RedisDatabase::type(const std::string& Key)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    if (kv_store.find(Key) != kv_store.end())
        return "string";
    if (list_store.find(Key) != list_store.end())
        return "list";
    if (hash_store.find(Key) != hash_store.end())
        return "hash";
    else
        return "none";
}
bool RedisDatabase::del(const std::string& Key)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    bool erased = false;
    erased |= kv_store.erase(Key) > 0;
    erased |= list_store.erase(Key) > 0;
    erased |= hash_store.erase(Key) > 0;
    return false;

}
//expire
bool RedisDatabase::expire(const std::string& Key, int  seconds)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    bool exists = (kv_store.find(Key) != kv_store.end()) ||
        (list_store.find(Key) != list_store.end()) ||
        (hash_store.find(Key) != hash_store.end());
    if (!exists)
        return false;

    expire_map[Key] = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    return true;
}
//rename
bool RedisDatabase::rename(const std::string& oldKey, const std::string& newKey)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    bool found = false;

    auto itKv = kv_store.find(oldKey);
    if (itKv != kv_store.end())
    {
        kv_store[newKey] = itKv->second;
        kv_store.erase(itKv);
        found = true;
    }

    auto itList = list_store.find(oldKey);
    if (itList != list_store.end())
    {
        list_store[newKey] = itList->second;
        list_store.erase(itList);
        found = true;
    }

    auto itHash = hash_store.find(oldKey);
    if (itHash != hash_store.end())
    {
        hash_store[newKey] = itHash->second;
        hash_store.erase(itHash);
        found = true;
    }

    auto itExpire = expire_map.find(oldKey);
    if (itExpire != expire_map.end())
    {
        expire_map[newKey] = itExpire->second;
        expire_map.erase(itExpire);

    }
    return found;
}
// key/Value Operations
// List Operations
// Hash Operations

/*
Very simple text=based persistance: each line encodes a record


Memory -> File - dump()
File -> Memory - load()

K = Key Value
L = List
H = Hash

*/


bool RedisDatabase::dump(const std::string& filename)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) return false;

    for (const auto& kv : kv_store) {
        ofs << "K" << kv.first << " " << kv.second << "\n";
    }
    for (const auto& kv : list_store)
    {
        ofs << "L" << kv.first;
        for (const auto& item : kv.second)
            ofs << " " << item;
        ofs << "\n";

    }
    for (const auto& kv : hash_store)
    {
        ofs << "H" << kv.first;
        for (const auto& field_val : kv.second)
            ofs << " " << field_val.first << ":" << field_val.second;
        ofs << "\n";
    }

    return true;
}
bool RedisDatabase::load(const std::string& filename)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) return false;

    kv_store.clear();
    list_store.clear();
    hash_store.clear();

    std::string line;
    while (std::getline(ifs, line))
    {
        std::istringstream iss(line);
        char type;
        iss >> type;
        if (type == 'K')
        {
            std::string Key, value;
            iss >> Key >> value;
            kv_store[Key] = value;
        }
        else if (type == 'L')
        {
            std::string Key;
            iss >> Key;
            std::string item;
            std::vector<std::string> list;
            while (iss >> item)
            {
                list.push_back(item);
                list_store[Key] = list;
            }
        }
        else if (type == 'H')
        {
            std::string Key;
            iss >> Key;
            std::unordered_map<std::string, std::string> hash;
            std::string pair;
            while (iss >> pair)
            {
                auto pos = pair.find(':');
                if (pos != std::string::npos)
                {
                    std::string field = pair.substr(0, pos);
                    std::string value = pair.substr(pos + 1);
                    hash[field] = value;
                }
            }
            hash_store[Key] = hash;
        }

    }

    return true;
}