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
private:
    KeyValueStorage& Storage_;
    KeyValueClient& gRPC_Client_;
    std::string Ip_Address_;
public:
    KeyValueServiceImpl(KeyValueStorage& Storage, const std::string& Ip_Address, KeyValueClient& gRPC_Client)
        : Storage_(Storage), gRPC_Client_(gRPC_Client)
    {
        Ip_Address_ = Ip_Address;
    }

    //Returns storage and adresses of gRPC servers or nothing depending on the context of the request
    Status JoinCluster(ServerContext* Context, const JoinClusterRequest* Request, JoinClusterResponse* Response) {
        //Takes ip_adress from request
        std::string Address_Request = Request->ip_adress();

        //Forms copy of storage and array of ip's to response
        for (const auto& i : Storage_) {
            Response->add_keys(i.first);
            Response->add_values(i.second);
        }
        for (const auto& i : Storage_.GetAddresses()) {
            std::cout << "IP: " << i << std::endl;
            //To evade connection of new replica to itself
            if (Address_Request == i){
                continue;
            }
            Response->add_ip_adresses(i);
        }
        
        //Estabilish connection and save ip of replica
        //if address already in set it means stub with this address is created and no need to make it again
        if (Storage_.FindAddress(Address_Request) == false){
            gRPC_Client_.EstabilishConnection(Address_Request);
            Storage_.AddReplicaAddress(Address_Request);
        }
        return Status::OK;
    }

    //Returns result of adding new element in storage
    Status ReplicateEntry(ServerContext* Context, const ReplicateEntryRequest* Request, ReplicateEntryResponse* Response) {
        bool Insertion_Result = Storage_.AddEntry(Request->key(), Request->value());
        Response->set_result(Insertion_Result);
        return Status::OK;
    }

    Status InformCluster(ServerContext* Context, const InformClusterRequest* Request, InformClusterResponse* Response){
        //Takes ip_adress from request
        std::string Address_Request = Request->ip_adress();

        //Debug info to signalize new server that need connection
        std::cout << "Wow new ip - haven't seen it before - " << Address_Request << std::endl;

        //Estabilish connection and save ip of replica
        gRPC_Client_.EstabilishConnection(Address_Request);
        Storage_.AddReplicaAddress(Address_Request);
        Response->set_result(true);
        return Status::OK;
    }

    //Creates instance of gRPC server and running it (blocks thread)
    void RunService() {
        ServerBuilder Builder;
        Builder.AddListeningPort(Ip_Address_, grpc::InsecureServerCredentials());
        Builder.RegisterService(this);
        std::unique_ptr<Server> server(Builder.BuildAndStart());
        server->Wait();
    }
};