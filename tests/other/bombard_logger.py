#!/usr/bin/env python3

import argparse
import os
import sys
import json

import urllib3

http = urllib3.PoolManager(20000)

from threading import Thread

threads=list()
import time
import concurrent
import concurrent.futures

#
# Terminal arguments
#
parser = argparse.ArgumentParser(description='Configures the project.')
parser.add_argument('core_URL', type=str, help='Where the core is running.')

NUM_REQS = 5000

def do_BOTH(x):
    do_POST(x[0])
    do_GET(x[1])

def do_POST(x):
    res = http.request(
        "POST",
        x[0],
        headers={"Content-Type": "application/json"},
        body=json.dumps(x[1]).encode("utf-8")
    )
    return res


def do_GET(x):
    return http.request(
        "get",
        x[0],
        fields=x[1]
    )

def log_all(URL:str):

    endpoint_submit = "{}eval-server/submit/".format(URL)
    endpoint_log = "{}log/text-query-change/".format(URL)

    fst = [(endpoint_submit, {"frameId": 0} ) for i in range(NUM_REQS)]
    snd = [(endpoint_log, {"query": "ahoj >> svete"} ) for i in range(NUM_REQS)]

    xx = list(zip(fst, snd))
    
    with concurrent.futures.ThreadPoolExecutor(max_workers=12) as pool:
        results = pool.map(do_BOTH, xx)
        #concurrent.futures.wait(results)

if __name__ == '__main__':
    args = parser.parse_args()

    cwd = os.getcwd()
    print("(!!!)")
    print("This script is running from the '{}' directory...".format(cwd))
    print("(!!!)")

    URL = args.core_URL

    print("Bombarding with logs: {}".format(URL))

    startTime = time.time()
    log_all(URL)
    executionTime = (time.time() - startTime)
    print('Total of {} parallel requests took {}s to finish.'.format(NUM_REQS * 2, executionTime ))


    sys.exit(0)