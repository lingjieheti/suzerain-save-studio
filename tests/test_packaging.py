"""Exercise the Windows release packager with the editor built by build.ps1."""
import hashlib
import re
import shutil
import tempfile
import unittest
from pathlib import Path
from zipfile import ZipFile

from tools.package_release import package


class PackagingTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[1]
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.output = self.root / "dist"
        for name in ("src/app.rc", "build/SuzerainSaveStudio.exe", "docs/QUICKSTART.md",
                     "live/README.zh-CN.md", "LICENSE", "src/THIRD-PARTY-NOTICES.txt"):
            target = self.root / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(repo / name, target)
        plugin = self.root / "build/live/SuzerainSaveStudio.Live.dll"
        plugin.parent.mkdir()
        plugin.write_bytes(b"offline packager test fixture")

    def test_guide_links_work_on_github_and_offline(self):
        guide = self.root / "docs/QUICKSTART.md"
        online_target = re.search(r"\[实时面板说明\]\(([^)]+)\)", guide.read_text("utf-8")).group(1)
        self.assertEqual((guide.parent / online_target).resolve(), self.root / "live/README.zh-CN.md")
        archive = package(self.output, root=self.root)
        with ZipFile(archive) as bundle:
            offline_guide = bundle.read("使用说明.md").decode("utf-8")
            target = re.search(r"\[实时面板说明\]\(([^)]+)\)", offline_guide).group(1)
            self.assertEqual(bundle.read(target), (self.root / "live/README.zh-CN.md").read_bytes())
        sums, = self.output.glob("*-SHA256SUMS.txt")
        self.assertEqual(sums.read_text("ascii").split(), [hashlib.sha256(archive.read_bytes()).hexdigest(), archive.name])

    def test_version_mismatch_does_not_publish_a_package(self):
        resource = self.root / "src/app.rc"
        resource.write_text('VALUE "ProductVersion", "99.98.97"', encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "Rebuild the desktop editor"):
            package(self.output, root=self.root)
        self.assertFalse(self.output.exists())
