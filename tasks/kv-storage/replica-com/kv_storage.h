#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <unordered_map>

class KeyValueStorage {
private:
    std::unordered_map<std::string, std::string> storage;
    std::unordered_map<std::string, bool> adresses;
    using storage_iterator = typename std::unordered_map<std::string, std::string>::iterator;
public:
    class Iterator{
        private:
            storage_iterator it_;
        public:
            Iterator(storage_iterator it_of_storage){
                it_ = it_of_storage;
            }

            Iterator& operator++(){
                it_++;
                return *this;
            }

            auto& operator*(){
                return *it_;
            }
            
            bool operator!=(const Iterator& rhs){
                return it_ != rhs.it_;
            }
    };
public:
    bool AddEntry(std::string_view key, std::string_view entry) {
        auto status = storage.insert({(std::string)key, (std::string)entry});
        return status.second;
    }

    std::string GetValue(std::string_view key) {
        auto i = storage.find(std::string(key));
        if (i == storage.end()){
            throw std::invalid_argument("Can't find this key");
        }
        std::string value (i->second);
        return value;
    }

    std::unordered_map<std::string, bool>& getAdresses(){
        return adresses;
    }

    void AddReplicaAdress(std::string_view adress){
        adresses.insert({(std::string)adress, true});
    }

    Iterator begin(){
        return Iterator(storage.begin());
    }

    Iterator end(){
        return Iterator(storage.end());
    }
};
