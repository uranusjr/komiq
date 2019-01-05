#!python3.7

import argparse
import os
import pathlib
import shutil
import subprocess
import sys


def parse_path(value):
    path = pathlib.Path(value).resolve()
    if not path.exists():
        raise argparse.ArgumentError(f"{value} does not exist")
    return path


DEFAULT_TARGET = pathlib.Path(os.environ["LOCALAPPDATA"], "Programs", "Komiq")


def main():
    if os.name != "nt":
        print("Only supported on Windows.")
        sys.exit(1)

    parser = argparse.ArgumentParser()
    parser.add_argument("exe", type=parse_path)
    parser.add_argument("--target", type=pathlib.Path, default=DEFAULT_TARGET)
    options = parser.parse_args()

    options.target.mkdir(parents=True, exist_ok=True)

    target_exe = options.target.joinpath(options.exe.name)
    shutil.copy2(str(options.exe), str(target_exe))
    subprocess.run(
        ["windeployqt", str(target_exe)],
        check=True, stdout=subprocess.DEVNULL,
    )

    print(f"Deployed to {target_exe}")


if __name__ == "__main__":
    main()
