#pragma once
#include "kv_storage.h"
#include "grpc_client.h"
#include <grpcpp/grpcpp.h>
#include "proto/kv.pb.h"
#include "proto/kv.grpc.pb.h"

using grpc::Status;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;

class KeyValueServiceImpl : public KeyValueGRPC::Service {
public:
    KeyValueServiceImpl(KeyValueStorage& storage, std::string_view ip_adress, KeyValueClient& grpc_client)
        : storage_(storage), grpc_client_(grpc_client)
    {
        ip_adress_ = ip_adress;
    }

    //Returns storage and adresses of gRPC servers or nothing depending on the context of the request
    Status JoinCluster(ServerContext* context, const JoinClusterRequest* request, JoinClusterResponse* response) {
        //Takes ip_adress from request
        std::string adress_request = request->ip_adress();

        //if ip_adress hasn't special symbol at back ('N'), copy entire storage and adresses of gRPC servers
        if (adress_request.back() != 'N') {
            for (const auto& i : storage_) {
                response->add_keys(i.first);
                response->add_values(i.second);
            }
            auto adresses = storage_.getAdresses();
            for (const auto& i : adresses) {
                std::cout << "IP: " << i << std::endl;
                response->add_ip_adresses(i);
            }
        }
        //if ip_adress has special symbol at back ('N'), it means server need to estabilish connection with client and save its adress;
        else {
            adress_request.pop_back();
            //Debug info to signalize new server that need connection
            std::cout << "Wow new ip - haven't seen it before - " << adress_request << std::endl;
        }
        grpc_client_.EstabilishConnection(adress_request);
        storage_.AddReplicaAdress(adress_request);
        return Status::OK;
    }

    //Returns result of adding new element in storage
    Status ReplicateEntry(ServerContext* context, const ReplicateEntryRequest* request, ReplicateEntryResponse* response) {
        bool insertion_result = storage_.AddEntry(request->key(), request->value());
        response->set_result(insertion_result);
        return Status::OK;
    }

    //Creates instance of gRPC server and running it (blocks thread)
    void RunService() {
        ServerBuilder builder;
        builder.AddListeningPort(ip_adress_, grpc::InsecureServerCredentials());
        builder.RegisterService(this);
        std::unique_ptr<Server> server(builder.BuildAndStart());
        server->Wait();
    }

private:
    KeyValueStorage& storage_;
    KeyValueClient& grpc_client_;
    std::string ip_adress_;
};
