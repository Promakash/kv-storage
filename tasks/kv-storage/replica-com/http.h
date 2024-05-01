#pragma once

#include <crow.h>
#include <cstdint>

#include "kv_storage.h"
#include "grpc_client.h"

class KeyValueHTTPServer {
private:
    uint16_t server_port;
public:
    KeyValueHTTPServer(uint16_t port, const std::string& joinReplicaAddr, KeyValueStorage& storage, KeyValueClient& grpc_client)
            : storage_(storage), grpc_client_(grpc_client)
    {
        server_port = port;
        //Copy info by gRPC from Replica if given
        grpc_client_.CopyClusterInfo(joinReplicaAddr);
        InitHandlers();
    }

    void Start(){
        app_.port(server_port).run();
    }

private:
    void InitHandlers() {
        CROW_ROUTE(app_, "/entry").methods("POST"_method)([&](const crow::request& req){
            try{
                //Take json request
                auto json_req = crow::json::load(req.body);

                //if can't parse throw exception
                if (!json_req){
                    throw std::invalid_argument("Can't parse json");
                }

                //parse json request
                std::string key = json_req["key"].s();
                std::string value = json_req["value"].s();

                //if key already exists in storage throw exception
                if (storage_.AddEntry(key, value) == false){
                    throw std::invalid_argument("Already exists!");
                }
                
                //if everything is good start replication calls to every instance of gRPC server and return 200(OK)
                grpc_client_.CopyEntry(key, value);
                return crow::response(200);

            }
            //If parsing goes wrong or json load fails or anything else return 400(Bad Request)
            catch(...){
                return crow::response(400);
            }
        });
        CROW_ROUTE(app_, "/entry").methods("GET"_method)([&](const crow::request& req){
            try{
                //parse key from url query
                auto key_query = req.url_params.get("key");

                //If can't parse throw invalid_argument
                if (key_query == nullptr){
                    throw std::invalid_argument("Can't parse string query!");
                }

                std::string key = key_query;
                //GetValue throws invalid_argument exception if couldn't find by key
                std::string value = storage_.GetValue(key);
                //Forms answer if everything is ok
                crow::json::wvalue json_response({"value", value});

                //return json and 200(OK)
                return crow::response(std::move(json_response));
            }
            //if can't find key or bad parsing return 404(Not found)
            catch(std::invalid_argument) {
                return crow::response(404);
            }
        });
        CROW_ROUTE(app_, "/entries").methods("GET"_method)([&](const crow::request& req){
            crow::json::wvalue x;
            //Initializes i to put values in order
            size_t i = 0;
            //Forms array of jsons
            for (auto it: storage_){
                x[i]["key"] = it.first;
                x[i]["value"] = it.second;
                i++;
            }
            //Returns json of jsons and 200(OK)
            return crow::response(std::move(x));
        });
        /* for tests */
        CROW_ROUTE(app_, "/readiness").methods("GET"_method)([](){ return "OK"; });
    }

private:
    crow::SimpleApp app_;
    KeyValueStorage& storage_;
    KeyValueClient& grpc_client_;
};
