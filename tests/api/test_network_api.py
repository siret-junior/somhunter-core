import http.server
import socketserver
import os
import argparse
import functools
import json
import pprint
pp = pprint.PrettyPrinter(indent=4)
import urllib3
http = urllib3.PoolManager()


def validate_GET__settings(data):
    return "api" in data

def validate_GET__user_context(data):
    return (
        "search" in data
        and "history" in data
        and "bookmarkedFrames" in data
    )

def get_GET_requests(cfg):
    core_host = "127.0.0.1:{}".format(cfg["port"])

    eps = {}

    for ep, value in cfg["endpoints"].items():
        if ("get" in value):
            gdict = value["get"]
            data = None
            if ("examples" in gdict) and (len(gdict["examples"]) > 0):
                data = gdict["examples"][0]

            eps[ep] = {
                "URL": core_host + gdict["url"],
                "data": data,
                "ex_code": 200,
                "val_fn": lambda _ : True
            }

    print("Getting {} GET endpoints...".format(len(eps)))

    # Custom validators
    eps["settings"]["val_fn"] = validate_GET__settings
    eps["userContext"]["val_fn"] = validate_GET__user_context

    return eps

def test_GET(URL, ex_code, val_fn, data=None):
    extraargs = {
        "fields": data
    }
    try:
        print("---")
        print("Testing GET request to '{}'...".format(URL))
        print("Data:")
        pp.pprint(data)
        with http.request('GET', URL, preload_content=False, **extraargs) as res:
            data_JSON = json.loads(res.data.decode('utf-8'))
            print("\tcode = {}".format(res.status))
            #pp.pprint(data_JSON)
            #assert(res.status == ex_code)
            assert(val_fn(data_JSON))
            return True
            
    except Exception as e:
        print("\tERROR: GET request to '{}' failed: ".format(URL))
        print(e)
        return False

def main(config, API_spec):
    cfg = config["api"]
    core_host = "127.0.0.1:{}".format(cfg["port"])
    API = API_spec

    print("Runnning external API tests...")
    result = True

    GET_requests = get_GET_requests(cfg)
    
    for k, r in GET_requests.items():
        result = result and test_GET(r["URL"], r["ex_code"], r["val_fn"], data=r["data"]) 
        
    if (result):
        print("External API tests succeeded!") 
    else:
        print("External API tests failed!") 
    return result

