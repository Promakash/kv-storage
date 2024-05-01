#pragma once
#include "kv_storage.h"
#include <grpcpp/grpcpp.h>
#include "proto/kv.pb.h"
#include "proto/kv.grpc.pb.h"

using grpc::Status;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;

class KeyValueServiceImpl : public KeyValueGRPC::Service{
public:
KeyValueServiceImpl(KeyValueStorage& storage, std::string_view ip_adress)
        : storage_(storage)
    {
        ip_adress_ = ip_adress;
    }

Status JoinCluster(ServerContext* context, const JoinClusterRequest* request, JoinClusterResponse* response){
    std::string adress_request = request->ip_adress();
    if (adress_request.back() != 'N'){
        for (const auto& i: storage_){
            response->add_keys(i.first);
            response->add_values(i.second);
        }
        auto adresses = storage_.getAdresses();
        for (const auto& i: adresses){
            if (i.second){
                response->add_ip_adresses(i.first);
            }
        }
        response->add_ip_adresses(ip_adress_);
    }
    else{
        adress_request.pop_back();
    }
    storage_.AddReplicaAdress(adress_request);
    return Status::OK;
}

::grpc::Status ReplicateEntry(ServerContext* context, const ReplicateEntryRequest* request, ReplicateEntryResponse* response){
    bool insertion_result = storage_.AddEntry(request->key(), request->value());
    response->set_result(insertion_result);
    return Status::OK;
}

void RunService(){
    ServerBuilder builder;
    builder.AddListeningPort(ip_adress_,grpc::InsecureServerCredentials());
    builder.RegisterService(this);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    server->Wait();
}

private:
    KeyValueStorage& storage_;
    std::string ip_adress_;
};
