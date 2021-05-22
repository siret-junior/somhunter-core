import json
import argparse
import os
import shutil
import hashlib
import sys
import zipfile
import urllib3

http = urllib3.PoolManager()

import utils


def main(args):
    config = None
    try:
        with open(args.config_file, 'r') as ifs:
            # Load data into json file
            config = json.load(ifs)
    except OSError:
        print("Could not open/read file: {}".format(args.config_file))
        sys.exit(1)

    install_config = None
    try:
        with open(args.install_config_file, 'r') as ifs:
            install_config = json.load(ifs)
    except OSError:
        print("Could not open/read file: {}".format(args.install_config_file))
        sys.exit(1)

    installing_libtorch_res = True
    print("\n>>> Installing libtorch... >>>")

    cfg = install_config["dependencies"]["libtorch"]

    platform = "win" if (args.platform == "win") else "UNIX"
    cuda = "CUDA" if (args.cuda) else "CPU"

    print("\tInstalling {}, {}...".format(platform, cuda))

    URL_rel = cfg[platform][cuda]["prebuild_lib_URL"]
    URL_deb = cfg[platform][cuda]["prebuild_lib_URL_debug"]

    install_dir_rel = os.path.join(".", cfg["install_dir"]["release"])
    zip_filename_rel = "libtorch-Release.zip"
    zip_filename_rel_filepath = os.path.join(install_dir_rel, zip_filename_rel)

    install_dir_deb = os.path.join(".", cfg["install_dir"]["debug"])
    zip_filename_deb = "libtorch-Debug.zip"
    zip_filename_deb_filepath = os.path.join(install_dir_deb, zip_filename_deb)

    if (not os.path.isdir(install_dir_rel)):
        os.makedirs(install_dir_rel)

    if (not os.path.isdir(install_dir_deb)):
        os.makedirs(install_dir_deb)

    MODELS_TO_DOWNLOAD = [
        {
            "filename": zip_filename_deb,
            "filepath": zip_filename_deb_filepath,
            "URL": URL_deb,
            "SHA_sum": cfg[platform][cuda]["prebuild_lib_URL_debug_SHA256"],
            "instal_dir": install_dir_deb
        },
        {
            "filename": zip_filename_rel,
            "filepath": zip_filename_rel_filepath,
            "URL": URL_rel,
            "SHA_sum": cfg[platform][cuda]["prebuild_lib_URL_SHA256"],
            "instal_dir": install_dir_rel,
        },
    ]

    for m in MODELS_TO_DOWNLOAD:
        print("Processing '{}'...".format(m["filename"]))
        if not (os.path.isfile(m["filepath"])):
            try:
                print("\tDownloading ZIP from '{}'...".format(m["URL"]))
                with http.request('GET', m["URL"],
                                  preload_content=False) as r, open(
                                      m["filepath"], 'wb') as ofs:
                    shutil.copyfileobj(r, ofs)
            except Exception as e:
                print("\tERROR: Downloading ZIP from '{}' failed: ".format(
                    m["URL"]))
                print(e)
                installing_libtorch_res = False

            print("\tZIP downloaded to '{}'. ".format(m["filepath"]))
        else:
            print("\tZIP already present at '{}'. ".format(m["filepath"]))

        if not (utils.checksum_file(m["filepath"], m["SHA_sum"])):
            print("\tERROR: ZIP checksum does not match at '{}'. ".format(
                m["filepath"]))
            installing_libtorch_res = False
        else:
            print("\tZIP checksum OK at '{}'. ".format(m["filepath"]))

    if (not installing_libtorch_res):
        sys.exit(2)

    for m in MODELS_TO_DOWNLOAD:
        print("Extracting '{}'...".format(m["filename"]))
        try:
            with zipfile.ZipFile(m["filepath"], 'r') as zip_ref:
                zip_ref.extractall(m["instal_dir"])
        except Exception as e:
            print("\tERROR: Extracting ZIP '{}' failed: ".format(
                m["filename"]))
            print(e)
            installing_libtorch_res = False

    if (installing_libtorch_res):
        print("<<< libtorch installed. <<<\n")
    else:
        print("<<< ERROR: libtorch is NOT ready. <<<\n")
        sys.exit(1)