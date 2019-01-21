#!python3.7

import argparse
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as et


QT_SHARED = pathlib.Path(os.environ["LOCALAPPDATA"], "Qt")

QT_SPEC = ("5.12.0", "msvc2017_64")
QT_ROOT = QT_SHARED.joinpath(*QT_SPEC)

PROJECT_ROOT = pathlib.Path(__file__).parent.parent.resolve()


PROJECT_CONFIGURATION_KEY = "ProjectExplorer.ProjectConfiguration.Id"
DISPLAY_NAME_KEY = "ProjectExplorer.ProjectConfiguration.DefaultDisplayName"
BUILD_DIRECTORY_KEY = "ProjectExplorer.BuildConfiguration.BuildDirectory"


def _find_visual_studio():
    program_files = os.environ.get("ProgramFiles(x86)")
    program_files = program_files or os.environ.get("ProgramFiles")
    vswhere = pathlib.Path(
        program_files, "Microsoft Visual Studio", "Installer", "vswhere.exe",
    )
    args = [
        str(vswhere),
        "-latest",
        "-prerelease",
        "-products",
        "*",
        "-property",
        "installationPath",
    ]
    return subprocess.check_output(args, encoding="mbcs").strip()


def _get_build_dir(project_root):
    with project_root.joinpath("komiq.pro.user").open() as f:
        doc = et.parse(f)

    build_dir = None
    for el in doc.iterfind(f".//value[@key={PROJECT_CONFIGURATION_KEY!r}]/.."):
        confid = el.find(f"./value[@key={PROJECT_CONFIGURATION_KEY!r}]").text
        if confid != "Qt4ProjectManager.Qt4BuildConfiguration":
            continue
        if el.find(f"./value[@key={DISPLAY_NAME_KEY!r}]").text != "Release":
            continue
        build_dir = el.find(f"./value[@key={BUILD_DIRECTORY_KEY!r}]").text
        break
    if build_dir is None:
        raise ValueError("failed to parse .pro.user")

    return pathlib.Path(build_dir)


BUILT_SCRIPT_TEMPLATE = """
@echo off
call "{qt_root}/bin/qtenv2.bat" >NUL
call "{vs_root}/VC/Auxiliary/Build/vcvars64.bat" >NUL
cd "{cwd}"
"{qt_root}/bin/qmake.exe" "{pro}" >NUL
"{make}" {make_args} >NUL
"""


def _build_exe(project_root, qt_root, vs_root, make, make_args):
    build_dir = _get_build_dir(project_root)
    print(f"Building in {build_dir}")

    shutil.rmtree(str(build_dir), ignore_errors=True)
    build_dir.mkdir(exist_ok=True, parents=True)

    pro = project_root.joinpath("komiq.pro")
    print(f"Processing {pro}")

    fd, filename = tempfile.mkstemp(suffix=".bat", text=True)
    with open(fd, "w") as f:
        f.write(BUILT_SCRIPT_TEMPLATE.format(
            qt_root=qt_root, vs_root=vs_root,
            make=make, make_args=" ".join(make_args),
            pro=pro, cwd=build_dir,
        ))
    subprocess.check_call(["cmd.exe", "/c", filename], cwd=build_dir)
    os.unlink(filename)

    return build_dir.joinpath("release", "komiq.exe")


DEFAULT_JOM = QT_SHARED.joinpath("Tools", "QtCreator", "bin", "jom.exe")

DEFAULT_TARGET = pathlib.Path(os.environ["LOCALAPPDATA"], "Programs", "Komiq")


def main():
    if os.name != "nt":
        print("Only supported on Windows.")
        sys.exit(1)

    parser = argparse.ArgumentParser()
    parser.add_argument("--qt-root", type=pathlib.Path, default=QT_ROOT)
    parser.add_argument("--make", type=pathlib.Path, default=DEFAULT_JOM)
    parser.add_argument("--target", type=pathlib.Path, default=DEFAULT_TARGET)
    options = parser.parse_args()

    project_root = PROJECT_ROOT.joinpath("src")
    vs_root = _find_visual_studio()
    if options.make.stem.lower() == "jom":
        make_args = ["/nologo"]
    else:
        make_args = []

    source_exe = _build_exe(
        project_root, options.qt_root, vs_root, options.make, make_args,
    )
    if not source_exe or not source_exe.is_file():
        raise ValueError("build failed")

    options.target.mkdir(parents=True, exist_ok=True)
    target_exe = options.target.joinpath(source_exe.name)
    shutil.copy2(str(source_exe), str(target_exe))

    print(f"Copying {target_exe}")
    subprocess.check_call(
        ["windeployqt", str(target_exe)],
        stdout=subprocess.DEVNULL,
    )

    print("Done.")


if __name__ == "__main__":
    main()
