import json
import os
import random
import string

import pytest
import requests


def test_http_kv():
    kv_addr = "http://localhost:8000"

    kv = dict()

    for i in range(1, 11):
        key = f'{i}_key'
        value = f'{i}_value'

        resp = requests.post(f'{kv_addr}/entry', json={"key": key, "value": value})
        assert resp.status_code == 200

    for i in range(1, 11):
        key = f'{i}_key'
        value = f'{i}_value'

        resp = requests.get(f'{kv_addr}/entry', params={"key": key})
        assert resp.status_code == 200

        assert value == json.loads(resp.content.decode())["value"]

    resp = requests.get(f'{kv_addr}/entry', params={"key": "lolkek228"})
    assert resp.status_code == 404

test_http_kv();