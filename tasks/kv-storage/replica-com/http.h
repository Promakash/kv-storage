#pragma once

#include <crow.h>
#include <cstdint>

#include "kv_storage.h"
#include "grpc_client.h"

class KeyValueHTTPServer {
private:
    uint16_t Server_Port_;
    crow::SimpleApp App_;
    KeyValueStorage& Storage_;
    KeyValueClient& gRPC_Client_;
public:
    KeyValueHTTPServer(uint16_t Port, const std::string& JoinReplicaAddr, KeyValueStorage& Storage, KeyValueClient& gRPC_Client)
            : Storage_(Storage), gRPC_Client_(gRPC_Client)
    {
        Server_Port_ = Port;
        //Copy info by gRPC from Replica if given
        gRPC_Client_.CopyClusterInfo(JoinReplicaAddr);
        InitHandlers();
    }

    void Start(){
        App_.port(Server_Port_).run();
    }

private:
    void InitHandlers() {
        CROW_ROUTE(App_, "/entry").methods("POST"_method)([&](const crow::request& Request){
            std::string Key;
            std::string Value;
            try{
                //Take json request
                auto Json_Request = crow::json::load(Request.body);

                //if can't parse json body returns 400(Bad Request) and specifying error
                if (!Json_Request){
                    return crow::response(400, "Unable to parse json correctly!");
                }

                //parse json request
                Key = Json_Request["key"].s();
                Value = Json_Request["value"].s();
            }
            //If parsing of json values goes wrong or json load fails return 400(Bad Request)
            catch(...){
                return crow::response(400, "Error while parsing json!");
            }

            //if key already exists in storage returns 400(Bad Request) and message about it
            if (Storage_.AddEntry(Key, Value) == false){
                return crow::response(400, "Key already in storage!");
            }
                
            //if everything is good start replication calls to every instance of gRPC server and return 200(OK)
            gRPC_Client_.CopyEntry(Key, Value);
            return crow::response(200);
        });
        CROW_ROUTE(App_, "/entry").methods("GET"_method)([&](const crow::request& Request){
            //parse key from url query
            auto Key_Query = Request.url_params.get("key");

            //If there is no key in the query - returns 400 with specifying message
            if (Key_Query == nullptr){
                return crow::response(400, "The key was not found in the query");
            }

            std::string Key = Key_Query;
            std::pair<bool, std::string> Search_Result = Storage_.GetValue(Key);
            if (Search_Result.first == false){
                return crow::response(404, "Nothing is found by this key!");
            }
            //Forms answer if everything is ok
            crow::json::wvalue Json_Response({"value", Search_Result.second});

            //return json and 200(OK)
            return crow::response(std::move(Json_Response));
        });
        CROW_ROUTE(App_, "/entries").methods("GET"_method)([&](const crow::request& Request){
            crow::json::wvalue Json_Response;
            //Initializes i to put values in order
            size_t i = 0;
            //Forms array of jsons
            for (auto it: Storage_){
                Json_Response[i]["key"] = it.first;
                Json_Response[i]["value"] = it.second;
                i++;
            }
            //Returns json of jsons and 200(OK)
            return crow::response(std::move(Json_Response));
        });
        /* for tests */
        CROW_ROUTE(App_, "/readiness").methods("GET"_method)([](){ return "OK"; });
    }
};