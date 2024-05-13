import json
import random
import string

import pytest
import requests
import subprocess
import time

def init_servers():
    #Clear all proccess in case of previous test failing
    subprocess.Popen(['killall', 'replica_com'])
    subprocess.Popen(['killall', 'replica_com'])
    time.sleep(2)
    #Start original server
    subprocess.Popen(['build/replica_com', '11111', '22222'])
    time.sleep(1)

    #Start replica server
    subprocess.Popen(['build/replica_com', '33333', '44444', '0.0.0.0:22222'])
    time.sleep(1)

    #Start replica of replica server
    subprocess.Popen(['build/replica_com', '55555', '55556', '0.0.0.0:44444'])
    time.sleep(1)

def test_communication():
    
    init_servers()

    servers = ["http://localhost:11111", "http://localhost:33333", "http://localhost:55555"]

    kv = dict()

    #Check that sended info to one server ends up on every server
    for server in servers:
        #Fill server with data
        for i in range(1, 20):
            number = random.randint(-10000000, 10000000)
            while kv.get(number, "") != "":
                number = random.randint(-10000000, 10000000)
            key = f'{number}_key'
            value = f'{number}_value'

            resp = requests.post(f'{servers[0]}/entry', json={"key": key, "value": value})
            assert resp.status_code == 200
            kv[key] = value

        for server_to_check in servers:
            for key in kv:
                resp = requests.get(f'{server_to_check}/entry', params={"key": key})
                assert resp.status_code == 200
                value = json.loads(resp.content.decode())[-1]
                kv_value = kv.get(key)
                assert value == kv_value