#include "http.h"
#include "grpc.h"
#include "kv_storage.h"
#include "grpc_client.h"

#include <argparse/argparse.hpp>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("KV");

    parser
        .add_argument("http-port", "hp")
        .scan<'i', uint16_t>()
        .required();

    parser
        .add_argument("grpc-port", "gp")
        .required();

    parser
        .add_argument("join-replica-addr", "j")
        .default_value(std::string(""));

    parser.parse_args(argc, argv);

    uint16_t httpPort = parser.get<uint16_t>("http-port");
    //gRPC accepts only string for adress
    std::string grpcPort = parser.get<std::string>("grpc-port");

    //For local testing
    std::string grpc_adress = "0.0.0.0:"+grpcPort;

    std::string joinReplicaAddr = parser.get<std::string>("join-replica-addr");

    KeyValueStorage storage;
    KeyValueClient grpc_client(storage, grpc_adress);

    auto runHTTP = [&](){
        KeyValueHTTPServer server(httpPort, joinReplicaAddr, storage, grpc_client);
        server.Start();
    };

    auto runGRPC = [&](){
        KeyValueServiceImpl grpc(storage, grpc_adress, grpc_client);
        grpc.RunService();
    };

    std::thread http(runHTTP);
    std::thread grpc(runGRPC);

    grpc.join();
    http.join();
}


//commands for easy debug
//1 ./replica_com 11111 22222
//2 ./replica_com 33333 44444 0.0.0.0:22222
//3 ./replica_com 55555 55556 0.0.0.0:44444

//curl -X GET http://0.0.0.0:11111/entry?key=52_key

//curl -X POST -H "Content-Type: application/json" -d '{"key": "52_key", "value":"31_value"}' http://0.0.0.0:11111/entry

//curl -X POST -H "Content-Type: application/json" -d '{"key": "48_key", "value":"48_value"}' http://0.0.0.0:33333/entry

//curl -X GET  http://0.0.0.0:11111/entries
//curl -X GET  http://0.0.0.0:33333/entries
//curl -X GET  http://0.0.0.0:55555/entries