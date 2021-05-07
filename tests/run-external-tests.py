import http.server
import socketserver
import os
import argparse
import functools
import json

import api.test_network_api as API

parser = argparse.ArgumentParser()
parser.add_argument("CONFIG", type=str)
parser.add_argument("API_SPEC", type=str)

def main(config, API_spec):
    API.main(config)

if __name__ == "__main__":
    args = parser.parse_args()

    config_filepath = os.path.abspath(args.CONFIG)
    API_spec_filepath = os.path.abspath(args.API_SPEC)
    
    config = None
    try:
        with open(config_filepath) as ifs:
            config = json.load(ifs)
    except Exception as e:  
        print(e)

    main(config, API_spec_filepath)