#!/usr/bin/env python3

import json
import argparse
import os
import shutil
import hashlib
import sys
import urllib3

http = urllib3.PoolManager()

import install_libtorch
import utils

#
# CMD args
#
parser = argparse.ArgumentParser(description='Configures the project.')
parser.add_argument('platform', type=str, help='For what platform?')
parser.add_argument('config_core_file',
                    type=str,
                    help='Filepath of the config JSON.')
parser.add_argument('config_install_file',
                    type=str,
                    help='Filepath of the install config JSON.')
parser.add_argument(
    'config_auth_file',
    type=str,
    help='Filepath of the JSON with authentication credentials.')
parser.add_argument('build_dir',
                    type=str,
                    help='Directory where the build is placed.')
parser.add_argument('third_party_dir',
                    type=str,
                    help='Directory where third-party deps are placed.')
parser.add_argument('build_type', type=str, help='Build type wanted.')
parser.add_argument('--cuda', type=bool, default=False, help='With CUDA?.')


def install_models(args):

    #
    # Parse "config-core"
    #
    config = None
    try:
        with open(args.config_core_file, 'r') as ifs:
            # Load data into json file
            config = json.load(ifs)
    except OSError:
        print("Could not open/read file: {}".format(args.config_core_file))
        sys.exit(1)

    install_config = None
    try:
        with open(args.config_install_file, 'r') as ifs:
            install_config = json.load(ifs)
    except OSError:
        print("Could not open/read file: {}".format(args.config_install_file))
        sys.exit(1)

    auth_config = None
    try:
        with open(args.config_auth_file, 'r') as ifs:
            auth_config = json.load(ifs)
    except OSError:
        print("Could not open/read file: {}".format(args.config_auth_file))
        sys.exit(1)

    config = config["core"]

    #
    # 1) Download the model files
    #
    download_models_res = True
    print("\n>>> Downloading the models... >>>")

    INSTAL_DIR_MODELS = os.path.join(".", config["models_dir"])
    MODELS_TO_DOWNLOAD = [
        {
            "filename": "traced_Resnet152.pt",
            "filepath": os.path.join(INSTAL_DIR_MODELS, "traced_Resnet152.pt"),
            "URL": config["model_ResNet_URL"],
            "SHA_sum": config["model_ResNet_SHA256"]
        },
        {
            "filename": "traced_Resnext101.pt",
            "filepath": os.path.join(INSTAL_DIR_MODELS,
                                     "traced_Resnext101.pt"),
            "URL": config["model_ResNext_URL"],
            "SHA_sum": config["model_ResNext_SHA256"]
        },
    ]

    remote_credentials = auth_config["remotes"]["herkules"]

    # Create auth headers
    headers = urllib3.make_headers(basic_auth="{}:{}".format(
        remote_credentials["username"], remote_credentials["password"]))

    for m in MODELS_TO_DOWNLOAD:

        print("Processing '{}'...".format(m["filename"]))
        if not (os.path.isfile(m["filepath"])):
            try:
                print("\tDownloading model from '{}'...".format(m["URL"]))
                with http.request('GET',
                                  m["URL"],
                                  headers=headers,
                                  preload_content=False) as r:
                    # If Unauthorized
                    if (r.status == 401):
                        print(
                            "!!! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! !!!"
                        )
                        print(
                            "!!! Unauthorized! Do you have correct credentials inside config/config-auth.json? !!!"
                        )
                        print(
                            "!!! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! !!!"
                        )
                        raise Exception("Unauthorized!")
                    elif (r.status != 200):
                        print("!!! Request returned code {}!".format(r.status))
                        raise Exception(r.status)
                    else:
                        with open(m["filepath"], 'wb') as ofs:
                            shutil.copyfileobj(r, ofs)
            except Exception as e:
                print("\tERROR: Downloading model from '{}' failed: ".format(
                    m["URL"]))
                print(e)
                download_models_res = False
                continue

            print("\tModel downloaded to '{}'. ".format(m["filepath"]))
        else:
            print("\tModel already present at '{}'. ".format(m["filepath"]))

        if not (utils.checksum_file(m["filepath"], m["SHA_sum"])):
            print("\tERROR: Model checksum does not match at '{}'. ".format(
                m["filepath"]))
            download_models_res = False
        else:
            print("\tModel checksum OK at '{}'. ".format(m["filepath"]))

    if (download_models_res):
        print("<<< All models are ready. <<<\n")
    else:
        print("<<< ERROR: Models are NOT ready. <<<\n")
        sys.exit(1)


if __name__ == '__main__':
    args = parser.parse_args()

    install_models(args)
    install_libtorch.main(args)
    sys.exit(0)