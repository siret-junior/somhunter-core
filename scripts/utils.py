#!/usr/bin/env python3

import json
import argparse
import os
import shutil
import hashlib
import sys

def checksum_file(filepath, SHA256_hex_str):
    BUF_SIZE = 65536
    SHA256 = hashlib.sha256()

    with open(filepath, 'rb') as f:
        while True:
            data = f.read(BUF_SIZE)
            if not data:
                break
            SHA256.update(data)

    if not (SHA256.hexdigest() == SHA256_hex_str):
        return False
    else:
        return True