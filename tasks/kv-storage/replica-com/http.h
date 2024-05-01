#pragma once

#include <crow.h>
#include <cstdint>

#include "kv_storage.h"
#include "grpc_client.h"

class KeyValueHTTPServer {
private:
    uint16_t server_port;
public:
    KeyValueHTTPServer(uint16_t port, const std::string& joinReplicaAddr, KeyValueStorage& storage, const std::string& grpc_adress)
            : storage_(storage), grpc_client(storage, grpc_adress)
    {
        server_port = port;
        grpc_client.CopyClusterInfo(joinReplicaAddr);
        InitHandlers();
    }

    void Start(){
        app_.port(server_port).run();
    }

private:
    void InitHandlers() {
        CROW_ROUTE(app_, "/entry").methods("POST"_method)([&](const crow::request& req){
            try{
                auto json_req = crow::json::load(req.body);

                if (!json_req){
                    throw std::invalid_argument("Can't parse json");
                }

                std::string key = json_req["key"].s();
                std::string value = json_req["value"].s();
                storage_.AddEntry(key, value);
                grpc_client.CopyEntry(key, value);
                return crow::response(200);

            }
            catch(std::invalid_argument){
                return crow::response(404);
            }
            catch(std::runtime_error){
                return crow::response(400);
            }
        });
        CROW_ROUTE(app_, "/entry").methods("GET"_method)([&](const crow::request& req){
            try{
                auto key_query = req.url_params.get("key");

                if (key_query == nullptr){
                    throw std::invalid_argument("Can't parse string query!");
                }

                std::string key = key_query;
                std::string value = storage_.GetValue(key);
                crow::json::wvalue json_response({"value", value});

                return crow::response(json_response);
            }
            catch(std::invalid_argument) {
                return crow::response(404);
            }
        });
        CROW_ROUTE(app_, "/entries").methods("GET"_method)([&](const crow::request& req){
            crow::json::wvalue x;
            size_t i = 0;
            for (auto it: storage_){
                x[i]["key"] = it.first;
                x[i]["value"] = it.second;
                i++;
            }
            return crow::response(std::move(x));
        });
        /* for tests */
        CROW_ROUTE(app_, "/readiness").methods("GET"_method)([](){ return "OK"; });
    }

private:
    crow::SimpleApp app_;
    KeyValueStorage& storage_;
    KeyValueClient grpc_client;
};
