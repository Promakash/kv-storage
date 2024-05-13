import json
import random
import string

import pytest
import requests
import subprocess
import time

def init_server():
    #Clear all proccess in case of previous test failing
    subprocess.Popen(['killall', 'replica_com'])
    subprocess.Popen(['killall', 'replica_com'])
    time.sleep(2)

    #Start original server
    subprocess.Popen(['build/replica_com', '11111', '22222'])
    time.sleep(1)

def test_replication():

    init_server()
    
    servers = ["http://localhost:11111", "http://localhost:33333"]

    kv = dict()

    for i in range(1, 20):
        number = random.randint(-10000000, 10000000)
        while kv.get(number, "") != "":
            number = random.randint(-10000000, 10000000)
        key = f'{number}_key'
        value = f'{number}_value'

        resp = requests.post(f'{servers[0]}/entry', json={"key": key, "value": value})
        assert resp.status_code == 200
        kv[key] = value

    # Start replica server
    subprocess.Popen(['build/replica_com', '33333', '44444', '0.0.0.0:22222'])
    time.sleep(1)

    for key in kv:
        resp = requests.get(f'{servers[1]}/entry', params={"key": key})
        assert resp.status_code == 200
        value = json.loads(resp.content.decode())[-1]
        kv_value = kv.get(key)
        assert value == kv_value
