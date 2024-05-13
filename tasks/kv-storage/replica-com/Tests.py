import json
import random
import string

import pytest
import requests
import subprocess
import time

class Tests:

    def check_server(self, address):
        
        while True:
            try:
                resp = requests.get(f'{address}/readiness')
                if (resp.status_code == 200):
                    break
            except:
                continue

    def init_servers(self, servers, count):

        servers = ["http://localhost:11111", "http://localhost:33333", "http://localhost:55555"]
        servers_processes = []
        #Clear all proccess in case of previous test failing
        proccess1 = subprocess.Popen(['killall', 'replica_com'])
        proccess2 = subprocess.Popen(['killall', 'replica_com'])
        while (proccess1.poll() is None and proccess2.poll() is None):
            continue

        #Start original server
        serverid_1 = subprocess.Popen(['build/replica_com', '11111', '22222'])
        self.check_server(servers[0])
        servers_processes.append(serverid_1)

        if count == 1:
            return

        #Start replica server
        serverid_2 = subprocess.Popen(['build/replica_com', '33333', '44444', '0.0.0.0:22222'])
        self.check_server(servers[1])
        servers_processes.append(serverid_2)

        #Start replica of replica server
        serverid_3 = subprocess.Popen(['build/replica_com', '55555', '55556', '0.0.0.0:44444'])
        self.check_server(servers[2])
        servers_processes.append(serverid_3)
        
        return servers_processes
    
    def fill_data(self, kv, server):
        
        for i in range(1, 100):
            number = random.randint(0, 10000000)
            while kv.get(number, "") != "":
                number = random.randint(0, 10000000)
            key = f'{number}_key'
            value = f'{number}_value'

            resp = requests.post(f'{server}/entry', json={"key": key, "value": value})
            assert resp.status_code == 200
            kv[key] = value
            
        return kv
    
    def check_consistency(self, kv, server):
        
        for key in kv:
            resp = requests.get(f'{server}/entry', params={"key": key})
            assert resp.status_code == 200
            value = json.loads(resp.content.decode())[-1]
            kv_value = kv.get(key)
            assert value == kv_value
    
    def test_communication(self):
        
        servers = ["http://localhost:11111", "http://localhost:33333", "http://localhost:55555"]
        kv = dict()
        self.init_servers(servers, 3)
        #Check that sended info to one server ends up on every server
        for server in servers:
            #Fill server with data
            kv = self.fill_data(kv, server)

            for server_to_check in servers:
                #check values
                self.check_consistency(kv, server_to_check)
    
    def test_replication(self):
        
        servers = ["http://localhost:11111", "http://localhost:33333", "http://localhost:55555"]
        kv = dict()
        self.init_servers(servers, 1)
        kv = self.fill_data(kv, servers[0])

        # Start replica server
        subprocess.Popen(['build/replica_com', '33333', '44444', '0.0.0.0:22222'])
        self.check_server(servers[1])

        #check values
        self.check_consistency(kv, servers[1])
        
    def test_fall_replica(self):
        
        servers = ["http://localhost:11111", "http://localhost:33333", "http://localhost:55555"]
        kv = dict()
        #send data to original server and check
        servers_processes = self.init_servers(servers, 3)
        kv = self.fill_data(kv, servers[0])
        for server_to_check in servers:
            self.check_consistency(kv, server_to_check)

        #kill original server
        servers_processes[0].kill()
        servers.pop(0)

        #send data to replica of replica
        kv = self.fill_data(kv, servers[-1])
        for server_to_check in servers:
            self.check_consistency(kv, server_to_check)

        #return original server
        servers_processes[0] = subprocess.Popen(['build/replica_com', '11111', '22222', '0.0.0.0:44444'])
        servers.insert(0, "http://localhost:11111")
        self.check_server(servers[0])

        #kill replica
        servers_processes[1].kill()
        servers.pop(1)

        #send data to replica of replica
        kv = self.fill_data(kv, servers[-1])
        for server_to_check in servers:
            self.check_consistency(kv, server_to_check)