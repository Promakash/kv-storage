import json
import os
import random
import string

import pytest
import requests


def test_http_kv():
    kv_addr = "http://localhost:8000"

    kv = dict()

    #Tests for json method
    for i in range(1, 11):
        key = f'{i}_key'
        value = f'{i}_value'

        resp = requests.post(f'{kv_addr}/entry', json={"key": key, "value": value})
        assert resp.status_code == 200
        kv[key] = value

    #Tests for get method of single element
    for i in range(1, 11):
        key = f'{i}_key'
        value = f'{i}_value'

        resp = requests.get(f'{kv_addr}/entry', params={"key": key})
        assert resp.status_code == 200

        assert value == json.loads(resp.content.decode())[-1]

    #Test for case when entry is requested that is not in storage
    resp = requests.get(f'{kv_addr}/entry', params={"key": "lolkek228"})
    assert resp.status_code == 404

    #Test for get method of all entries at once
    resp = requests.get(f'{kv_addr}/entries')
    entries = json.loads(resp.content.decode())
    for entry in entries:
        del kv[entry['key']]
    assert resp.status_code == 200
    assert len(kv) == 0