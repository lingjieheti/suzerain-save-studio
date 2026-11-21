"""Bundle the built Windows editor, companion plugin and offline instructions."""
import argparse
import hashlib
import re
import struct
from pathlib import Path
from tempfile import TemporaryDirectory
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo


def verify_editor_version(editor: bytes, version: str) -> None:
    major, minor, patch = map(int, version.split("."))
    version_ms, version_ls = (major << 16) | minor, patch << 16
    # Win32 VERSIONINFO starts with its UTF-16 key, DWORD padding, and
    # VS_FIXEDFILEINFO (signature, structure version, file and product versions).
    key = "VS_VERSION_INFO\0".encode("utf-16-le") + b"\0\0"
    fixed = struct.pack("<6I", 0xFEEF04BD, 0x10000, version_ms, version_ls, version_ms, version_ls)
    if key + fixed not in editor:
        raise ValueError(f"Rebuild the desktop editor for {version} before packaging.")


def package(output: Path, *, root: Path = Path(__file__).resolve().parents[1]) -> Path:
    resource = (root / "src/app.rc").read_text(encoding="utf-8")
    version = re.search(r'VALUE "ProductVersion", "([\d.]+)"', resource).group(1)
    name = f"SuzerainSaveStudio-{version}"
    entries = {
        "SuzerainSaveStudio.exe": "build/SuzerainSaveStudio.exe",
        "SuzerainSaveStudio.Live.dll": "build/live/SuzerainSaveStudio.Live.dll",
        "使用说明.md": "docs/QUICKSTART.md",
        "实时面板.md": "live/README.zh-CN.md",
        "LICENSE.txt": "LICENSE",
        "THIRD-PARTY-NOTICES.txt": "src/THIRD-PARTY-NOTICES.txt",
    }
    contents = {filename: (root / source).read_bytes() for filename, source in entries.items()}
    verify_editor_version(contents["SuzerainSaveStudio.exe"], version)
    # The two guides are siblings in the ZIP and live in different folders on GitHub.
    contents["使用说明.md"] = contents["使用说明.md"].replace(
        b"(../live/README.zh-CN.md)", "(实时面板.md)".encode("utf-8"))
    output.mkdir(parents=True, exist_ok=True)
    with TemporaryDirectory(dir=output) as scratch:
        archive = Path(scratch) / f"{name}-Live-Preview-win-x64.zip"
        with ZipFile(archive, "w") as bundle:
            for filename, data in contents.items():
                # Stable entry metadata makes rebuilding from the same inputs verifiable.
                bundle.writestr(ZipInfo(filename), data,
                                compress_type=ZIP_DEFLATED, compresslevel=9)
        with ZipFile(archive) as bundle:
            for filename, data in contents.items():
                if bundle.read(filename) != data:
                    raise ValueError(f"Packaged file differs from build input: {filename}")
        digest = hashlib.sha256(archive.read_bytes()).hexdigest()
        sums = Path(scratch) / f"{name}-SHA256SUMS.txt"
        sums.write_bytes(f"{digest}  {archive.name}\n".encode("ascii"))
        archive.replace(output / archive.name)
        sums.replace(output / sums.name)
    return output / archive.name


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output_directory", type=Path)
    print(package(parser.parse_args().output_directory))
