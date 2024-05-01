#pragma once
#include "kv_storage.h"
#include <grpcpp/grpcpp.h>
#include "proto/kv.pb.h"
#include "proto/kv.grpc.pb.h"

using grpc::Status;
using grpc::ClientContext;
using grpc::Channel;


class KeyValueClient{
public:
KeyValueClient(KeyValueStorage& storage, std::string_view grpc_adress)
        : storage_(storage)
    {
        grpc_adress_ = grpc_adress;
    }

void CopyEntry(std::string_view key, std::string_view value){
        auto adresses = storage_.getAdresses();
        ReplicateEntryRequest request;
        ReplicateEntryResponse response;
        for (const auto& i: adresses){
            ClientContext context;
            auto channel = grpc::CreateChannel(i.first, grpc::InsecureChannelCredentials());
            KeyValueGRPC::Stub stub_(channel);
            request.set_key(key);
            request.set_value(value);
            Status status = stub_.ReplicateEntry(&context, request, &response);
            if (!status.ok()){
                adresses[i.first] = false;
            }
        }

}

void InformCluster(){
    auto adresses = storage_.getAdresses();
    JoinClusterRequest request;
    JoinClusterResponse response;
    request.set_ip_adress(grpc_adress_+'N');
    for (const auto& i: adresses){
        if (i.second){
            ClientContext context;
            auto channel = grpc::CreateChannel(i.first, grpc::InsecureChannelCredentials());
            KeyValueGRPC::Stub stub_(channel);
            Status status = stub_.JoinCluster(&context, request, &response);
            if (!status.ok()){
                adresses[i.first] = false;
            }
        }
    }
}

void CopyClusterInfo(const std::string& joinReplicaAddr){
    if (joinReplicaAddr == ""){
        return;
    }
    auto channel = grpc::CreateChannel(joinReplicaAddr, grpc::InsecureChannelCredentials());
    KeyValueGRPC::Stub stub_(channel);
    ClientContext context;
    JoinClusterRequest request;
    JoinClusterResponse response;
    request.set_ip_adress(joinReplicaAddr);
    Status status = stub_.JoinCluster(&context, request, &response);
    if (!status.ok()){
       //Продумать что делать если не установлено соединение с репликой
    }
    for (const auto& i: response.keys()){
        for (const auto& j: response.values()){
            storage_.AddEntry(i,j);
        }
    }
    for (const auto& i: response.ip_adresses()){
        storage_.AddReplicaAdress(i);
    }
    InformCluster();
}

private:
    KeyValueStorage& storage_;
    std::string grpc_adress_;
};
