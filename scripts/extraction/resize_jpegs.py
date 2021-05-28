#!/usr/bin/env python3

from PIL import Image 
import os 
import PIL 
import glob


import argparse
import os
import sys

#
# Terminal arguments
#
parser = argparse.ArgumentParser(description='Configures the project.')
parser.add_argument('input_dir', type=str, help='Input dir.')
parser.add_argument('output_dir', type=str, help='Output dir')

parser.add_argument('width', type=int, help='Width in pixels.')
parser.add_argument('height', type=int, help='Height in pixels.')
parser.add_argument('quality', type=int, help='JPEG quality.')


def handle_dir(in_dir, out, w, h, q):
    print("Processing the '{}' directory, output to '{}'...".format(in_dir, out))
    # Make sure that directory exists
    os.makedirs(out, exist_ok=True)

    for filename in os.listdir(in_dir): 
        record = os.path.join(in_dir, filename)
        # Call recursivelly on dirs
        if (os.path.isdir(record)):
            handle_dir(record, os.path.join(out, filename), w, h, q)
        elif (filename.endswith(".jpg") or filename.endswith(".jpeg")):
            with Image.open(os.path.join(in_dir,filename)) as im:
                im = im.resize((w,h), Image.ANTIALIAS)
                im.save(os.path.join(out,filename), "JPEG", quality=95)

        else:
            print(filename)
            raise Exception("Invalid file!")
            


def main(args):
    os.makedirs(args.output_dir, exist_ok=True)
    handle_dir(args.input_dir, args.output_dir, args.width, args.height, args.quality)

if __name__ == '__main__':
    args = parser.parse_args()

    cwd = os.getcwd()
    print("(!!!)")
    print("This script is running from the '{}' directory...".format(cwd))
    print("(!!!)")

    sys.exit(main(args))