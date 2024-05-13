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
private:
    KeyValueStorage& Storage_;
    std::string gRPC_Address_;
    std::list<KeyValueGRPC::Stub> Current_Connections_;
public:

    KeyValueClient(KeyValueStorage& Storage, const std::string& gRPC_Address)
        : Storage_(Storage)
    {
        gRPC_Address_ = gRPC_Address;
    }

private:
    
    //Asks every known gRPC server to open a channel with Client's instance
    void InformCluster() {
        InformClusterRequest Request;
        InformClusterResponse Response;

        //Set special symbol at end of adress to point that request is not asking for copying storage.
        Request.set_ip_adress(gRPC_Address_);
        auto it = Current_Connections_.begin();

        //Increase iterator because begin() points to a replica that has already been connected
        it++;

        while (it != Current_Connections_.end()) {

            ClientContext Context;
            //Sends request
            Status Status = (*it).InformCluster(&Context, Request, &Response);

            if (!Status.ok()) {
                //erase() method returns next iterator after deleted iterator. Delete adress from storage_.
                it = Current_Connections_.erase(it);
                Storage_.DeleteReplicaAddress(Context.peer());

                //No need to increment iterator because of erase() method
                continue;
            }
            it++;
        }
    }

public:
    //Creates new gRPC's connection to given instance
    void EstabilishConnection(const std::string& Ip_address) {
        auto New_Channel = grpc::CreateChannel(Ip_address, grpc::InsecureChannelCredentials());
        KeyValueGRPC::Stub New_Stub(New_Channel);
        Current_Connections_.push_back(New_Stub);
    }

    //Sends gRPC's request to copy new Entry from HTTP server to every instance of known gRPC servers
    void CopyEntry(const std::string& Key, const std::string& Value) {

        //Forms info for request
        ReplicateEntryRequest Request;
        ReplicateEntryResponse Response;

        //Points at first element of current_connections
        auto it = Current_Connections_.begin();

        //Iterates through every connection and sends data
        while (it != Current_Connections_.end()) {
            //Setting info to send
            ClientContext Context;
            Request.set_key(Key);
            Request.set_value(Value);

            Status Status = (*it).ReplicateEntry(&Context, Request, &Response);
            
            //If connection is lost - deletes it. Deletion happens locally.
            if (!Status.ok()) {

                //Debug info 
                std::cout << "List size: " << Current_Connections_.size() << std::endl;
                std::cout << "One of replicas was shut down. " << Context.peer() << std::endl;
                std::cout << "Starting deletion" << std::endl;

                //erase() method returns next iterator after deleted iterator. Delete adress from storage_.
                it = Current_Connections_.erase(it);
                Storage_.DeleteReplicaAddress(Context.peer());

                std::cout << "Other connections left: ";
                for (auto j : Storage_.GetAddresses()) {
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
    void CopyClusterInfo(const std::string& JoinReplicaAddr) {

        //Returns if no Replica's adress is given
        if (JoinReplicaAddr == "") {
            return;
        }

        //Creating connection with given replica instance 
        auto ReplicaChannel = grpc::CreateChannel(JoinReplicaAddr, grpc::InsecureChannelCredentials());
        KeyValueGRPC::Stub ReplicaStub(ReplicaChannel);

        //Forms data to send request
        ClientContext Context;
        JoinClusterRequest Request;
        Request.set_ip_adress(gRPC_Address_);
        JoinClusterResponse Response;

        //Sends request
        Status Status = ReplicaStub.JoinCluster(&Context, Request, &Response);

        //Without estabilished connection with replica throws exception
        if (!Status.ok()) {
            throw std::invalid_argument("Wrong address of replica!");
        }

        //If conntection is succesfull - saving Stub and channel for future requests. Saving ip-adress of replica to send it by request later to new replica
        Current_Connections_.push_back(ReplicaStub);
        Storage_.AddReplicaAddress(JoinReplicaAddr);

        //Fill kv-storage with key, values and ip-adresses of servers
        for (const auto& i : Response.keys()) {
            for (const auto& j : Response.values()) {
                Storage_.AddEntry(i, j);
            }
        }
        for (const auto& i : Response.ip_adresses()) {
            EstabilishConnection(i);
            Storage_.AddReplicaAddress(i);
        }
        //Call function to send its own ip to every known server
        InformCluster();
    }
};