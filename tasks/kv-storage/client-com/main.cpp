#include "http.h"
#include "kv_storage.h"

int main() {
    KeyValueStorage storage;
    KeyValueHTTPServer server(8000, storage);
    server.Start();
}
