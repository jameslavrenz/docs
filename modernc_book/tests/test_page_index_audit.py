#!/usr/bin/env python3
"""Regression tests for chapter-page indexing of generated PDFs."""
import importlib.util
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "page_index_audit.py"
spec = importlib.util.spec_from_file_location("page_index_audit", SCRIPT)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class IndexSectionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for directory in ("chapters", "appendices"):
            (self.root / directory).mkdir()
        self.finder = self.root / "appendices" / "F-topic-finder.md"
        self.pages = ["Title page", "Contents"]
        for n in range(1, 20):
            title = f"{n}. Example chapter {n}"
            (self.root / "chapters" / f"{n:02d}-chapter.md").write_text(
                "# " + title + "\n", encoding="utf-8"
            )
            self.pages.append(title + "\nExample text.")
            self.pages.append("Some continuation text.")
        for letter in "ABCDEF":
            title = f"Appendix {letter}. Example appendix {letter}"
            (self.root / "appendices" / f"{letter}-appendix.md").write_text(
                "# " + title + "\n", encoding="utf-8"
            )
            self.pages.append(title + "\nExample text.")
        # A numbered list can begin a page; it must not overwrite Chapter 2.
        self.pages[31] = "2. Types: what are the promotions?\nAn ordinary numbered list"

    def test_list_item_is_not_chapter(self):
        bounds = module.section_bounds(self.pages, self.finder)
        self.assertEqual(bounds["Ch2"][0], 5)
        self.assertEqual(bounds["Ch15"][0], 31)
        self.assertLess(bounds["Ch2"][1], bounds["Ch3"][0])

    def test_missing_chapter_title_is_rejected(self):
        self.pages[4] = "2. Unrelated heading"
        with self.assertRaisesRegex(ValueError, "Missing PDF"):
            module.section_bounds(self.pages, self.finder)

    def test_duplicate_heading_is_rejected(self):
        self.pages[31] = "2. Example chapter 2"
        with self.assertRaisesRegex(ValueError, "Duplicate"):
            module.section_bounds(self.pages, self.finder)


if __name__ == "__main__":
    unittest.main()
