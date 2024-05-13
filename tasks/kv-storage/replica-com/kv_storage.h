#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <unordered_set>

class KeyValueStorage {
private:
    std::unordered_map<std::string, std::string> Storage_;
    std::unordered_set<std::string> Addresses_;
    using Storage_Iterator = typename std::unordered_map<std::string, std::string>::iterator;
public:
    //Iterator == storage iterator 
    class Iterator{
        private:
            Storage_Iterator it_;
        public:
            Iterator(Storage_Iterator it_of_storage){
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
    //Returns result of insertion: false - key was already in storage, true - successful insertion
    bool AddEntry(const std::string& Key, const std::string& Entry) {
        auto Status = Storage_.insert({Key, Entry});
        return Status.second;
    }

    //Returns pair <bool, value> by key. bool = false if nothing is found.
    std::pair<bool, std::string> GetValue(const std::string& Key) {
        //it == storage.end() if found nothing
        auto it = Storage_.find(Key);
        if (it == Storage_.end()){
            return {false, ""};
        }
        std::string Result (it->second);
        return {true, Result};
    }

    //Returns set of addresses of gRPC servers
    std::unordered_set<std::string> GetAddresses(){
        return Addresses_;
    }

    //Adds new address to set of gRPC servers
    void AddReplicaAddress(const std::string& Address){
        Addresses_.insert(Address);
    }

    //Removes address from set of gRPC servers
    void DeleteReplicaAddress(const std::string& Address){
        Addresses_.erase(Address);
    }

    //Searches for address in set
    bool FindAddress(const std::string& Address){
        if (Addresses_.find(Address) == Addresses_.end()){
            return false;
        }
        return true;
    }

    Iterator begin(){
        return Iterator(Storage_.begin());
    }

    Iterator end(){
        return Iterator(Storage_.end());
    }
};