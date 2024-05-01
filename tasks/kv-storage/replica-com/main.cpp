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
    std::string grpcPort = parser.get<std::string>("grpc-port");
    std::string grpc_adress = "0.0.0.0:"+grpcPort;

    std::string joinReplicaAddr = parser.get<std::string>("join-replica-addr");

    KeyValueStorage storage;
    KeyValueClient grpc_client(storage, grpc_adress);

    auto runHTTP = [&](){
        KeyValueHTTPServer server(httpPort, joinReplicaAddr, storage, grpc_adress);
        server.Start();
    };

    auto runGRPC = [&](){
        KeyValueServiceImpl grpc(storage, grpc_adress);
        grpc.RunService();
    };

    std::thread http(runHTTP);
    std::thread grpc(runGRPC);

    grpc.join();
    http.join();
}


//1 http-port 11111 grpc-port 22222
//2 http-port 33333 grpc-port 44444
//curl -X POST -H "Content-Type: application/json" -d '{"key": "52_key", "value":"31_value"}' http://0.0.0.0:8000/entry
//curl -X GET http://0.0.0.0:8000/entry?key=52_key

//curl -X GET  http://0.0.0.0:8000/entries

//curl -X POST -H "Content-Type: application/json" -d '{"key": "52_key", "value":"31_value"}' http://0.0.0.0:11111/entry

//curl -X POST -H "Content-Type: application/json" -d '{"key": "48_key", "value":"48_value"}' http://0.0.0.0:33333/entry

//curl -X GET  http://0.0.0.0:11111/entries
//curl -X GET  http://0.0.0.0:33333/entries
//curl -X GET  http://0.0.0.0:00001/entries

//TODO: разработать очистку если реплика отвалилась
//причесать все
//не позволять добавлять копии
//а если порт 00000 будет как обработать ситуацию?
