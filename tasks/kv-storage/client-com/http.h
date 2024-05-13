#pragma once

#include <crow.h>
#include <cstdint>

#include "kv_storage.h"

class KeyValueHTTPServer {
private:
    uint16_t Server_Port_;
    crow::SimpleApp App_;
    KeyValueStorage& Storage_;
public:
    KeyValueHTTPServer(uint16_t Port, KeyValueStorage& Storage)
        : Storage_(Storage)
    {
        Server_Port_ = Port;
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


//for easy checking
//curl -X POST -H "Content-Type: application/json" -d '{"key": "52_key", "value":"31_value"}' http://0.0.0.0:8000/entry
//curl -X GET http://0.0.0.0:8000/entry?key=52_key

//curl -X GET  http://0.0.0.0:8000/entries