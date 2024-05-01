#pragma once
#include <iostream>
#include "kv_storage.h"
#include <grpcpp/grpcpp.h>
#include "proto/kv.pb.h"
#include "proto/kv.grpc.pb.h"

using grpc::Status;
using grpc::ClientContext;
using grpc::Channel;


class KeyValueClient {
public:

    KeyValueClient(KeyValueStorage& storage, std::string_view grpc_adress)
        : storage_(storage)
    {
        grpc_adress_ = grpc_adress;
    }

private:
    
    //Asks every known gRPC server to open a channel with Client's instance
    void InformCluster() {
        JoinClusterRequest request;
        JoinClusterResponse response;

        //Set special symbol at end of adress to point that request is not asking for copying storage.
        request.set_ip_adress(grpc_adress_ + 'N');
        auto it = current_connections.begin();

        //Increase iterator because begin() points to a replica that has already been connected
        it++;

        while (it != current_connections.end()) {

            ClientContext context;
            //Sends request
            Status status = (*it).first.JoinCluster(&context, request, &response);

            if (!status.ok()) {
                //erase() method returns next iterator after deleted iterator. Delete adress from storage_.
                it = current_connections.erase(it);
                storage_.DeleteReplicaAdress(context.peer());

                //No need to increment iterator because of erase() method
                continue;
            }
            it++;
        }
    }

public:
    //Creates new gRPC's connection to given instance
    void EstabilishConnection(const std::string& ip_adress) {
        auto new_channel = grpc::CreateChannel(ip_adress, grpc::InsecureChannelCredentials());
        KeyValueGRPC::Stub new_stub(new_channel);
        current_connections.push_back({ new_stub, new_channel });
    }

    //Sends gRPC's request to copy new Entry from HTTP server to every instance of known gRPC servers
    void CopyEntry(std::string_view key, std::string_view value) {

        //Forms info for request
        ReplicateEntryRequest request;
        ReplicateEntryResponse response;

        //Points at first element of current_connections
        auto it = current_connections.begin();

        //Iterates through every connection and sends data
        while (it != current_connections.end()) {
            //Setting info to send
            ClientContext context;
            request.set_key(key);
            request.set_value(value);

            Status status = (*it).first.ReplicateEntry(&context, request, &response);
            
            //If connection is lost - deletes it. Deletion happens locally.
            if (!status.ok()) {

                //Debug info 
                std::cout << "List size: " << current_connections.size() << std::endl;
                std::cout << "One of replicas was shut down. " << context.peer() << std::endl;
                std::cout << "Starting deletion" << std::endl;

                //erase() method returns next iterator after deleted iterator. Delete adress from storage_.
                it = current_connections.erase(it);
                storage_.DeleteReplicaAdress(context.peer());

                std::cout << "Other connections left: ";
                for (auto j : storage_.getAdresses()) {
                    std::cout << j << std::endl;
                }
                std::cout << std::endl;

                //No need to increment iterator because of erase() method
                continue;
            }
            it++;
        }
    }

    //Create's connection to given gRPC server and replicate all data from it(if given). Calls InformCluster.
    void CopyClusterInfo(const std::string& joinReplicaAddr) {

        //Returns if no Replica's adress is given
        if (joinReplicaAddr == "") {
            return;
        }

        //Creating connection with given replica instance 
        auto ReplicaChannel = grpc::CreateChannel(joinReplicaAddr, grpc::InsecureChannelCredentials());
        KeyValueGRPC::Stub ReplicaStub(ReplicaChannel);

        //Forms data to send request
        ClientContext context;
        JoinClusterRequest request;
        request.set_ip_adress(grpc_adress_);
        JoinClusterResponse response;

        //Sends request
        Status status = ReplicaStub.JoinCluster(&context, request, &response);

        //Without estabilished connection with replica throws exception
        if (!status.ok()) {
            throw std::invalid_argument("Wrong adress of replica!");
        }

        //If conntection is succesfull - saving Stub and channel for future requests. Saving ip-adress of replica to send it by request later to new replica
        current_connections.push_back({ ReplicaStub, ReplicaChannel });
        storage_.AddReplicaAdress(joinReplicaAddr);

        //Fill kv-storage with key, values and ip-adresses of servers
        for (const auto& i : response.keys()) {
            for (const auto& j : response.values()) {
                storage_.AddEntry(i, j);
            }
        }
        for (const auto& i : response.ip_adresses()) {
            EstabilishConnection(i);
            storage_.AddReplicaAdress(i);
        }
        //Call function to send its own ip to every known server
        InformCluster();
    }

private:
    KeyValueStorage& storage_;
    std::string grpc_adress_;
    std::list<std::pair<KeyValueGRPC::Stub, std::shared_ptr<Channel>>> current_connections;
};
