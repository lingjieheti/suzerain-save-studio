"""Bundle the built Windows editor, companion plugin and offline instructions."""
import argparse
import hashlib
import re
from pathlib import Path
from tempfile import TemporaryDirectory
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo


def package(output: Path) -> Path:
    root = Path(__file__).resolve().parents[1]
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
    output.mkdir(parents=True, exist_ok=True)
    with TemporaryDirectory(dir=output) as scratch:
        archive = Path(scratch) / f"{name}-Live-Preview-win-x64.zip"
        with ZipFile(archive, "w") as bundle:
            for filename, source in entries.items():
                # Stable entry metadata makes rebuilding from the same inputs verifiable.
                bundle.writestr(ZipInfo(filename), (root / source).read_bytes(),
                                compress_type=ZIP_DEFLATED, compresslevel=9)
        with ZipFile(archive) as bundle:
            for filename, source in entries.items():
                if bundle.read(filename) != (root / source).read_bytes():
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
