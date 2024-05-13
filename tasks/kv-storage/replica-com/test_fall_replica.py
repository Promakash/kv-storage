import json
import random
import string

import pytest
import requests
import subprocess
import time

def init_servers():
    servers_processes = []
    #Clear all proccess in case of previous test failing
    subprocess.Popen(['killall', 'replica_com'])
    subprocess.Popen(['killall', 'replica_com'])
    time.sleep(2)
    #Start original server
    serverid_1 = subprocess.Popen(['build/replica_com', '11111', '22222'])
    time.sleep(1)
    servers_processes.append(serverid_1)

    #Start replica server
    serverid_2 = subprocess.Popen(['build/replica_com', '33333', '44444', '0.0.0.0:22222'])
    time.sleep(1)
    servers_processes.append(serverid_2)

    #Start replica of replica server
    serverid_3 = subprocess.Popen(['build/replica_com', '55555', '55556', '0.0.0.0:44444'])
    time.sleep(2)
    servers_processes.append(serverid_3)

    return servers_processes

def fill_data(kv, servers, index):
    for i in range(1, 10):
        number = random.randint(-10000000, 10000000)
        while kv.get(number, "") != "":
            number = random.randint(-10000000, 10000000)
        key = f'{number}_key'
        value = f'{number}_value'

        resp = requests.post(f'{servers[index]}/entry', json={"key": key, "value": value})
        assert resp.status_code == 200
        kv[key] = value
    return kv

def check_consistency(kv, servers):
    for server_to_check in servers:
        print(f'Currently checking: {server_to_check}')
        for key in kv:
            resp = requests.get(f'{server_to_check}/entry', params={"key": key})
            assert resp.status_code == 200
            value = json.loads(resp.content.decode())[-1]
            kv_value = kv.get(key)
            assert value == kv_value

def test_fall_replica():
    servers_processes = init_servers()

    servers = ["http://localhost:11111", "http://localhost:33333", "http://localhost:55555"]

    kv = dict()

    #send data to original server and check
    kv = fill_data(kv, servers, 0)
    check_consistency(kv, servers)

    #kill original server
    servers_processes[0].kill()
    servers.pop(0)

    #send data to replica of replica
    kv = fill_data(kv, servers, -1)
    check_consistency(kv, servers)

    #return original server
    servers_processes[0] = subprocess.Popen(['build/replica_com', '11111', '22222', '0.0.0.0:44444'])
    servers.insert(0, "http://localhost:11111")
    time.sleep(2)

    #kill replica
    servers_processes[1].kill()
    servers.pop(1)

    #send data to replica of replica
    kv = fill_data(kv, servers, -1)
    check_consistency(kv, servers)