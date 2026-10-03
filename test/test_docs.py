#!/usr/bin/env python3
# Copyright (C) 2026 Qore Technologies, s.r.o.
# SPDX-License-Identifier: MIT
"""Check MessagePack guide navigation: python3 test/test_docs.py build -v."""

from html.parser import HTMLParser
from pathlib import Path
import sys
import unittest
from urllib.parse import unquote, urlsplit


HTML = Path(sys.argv.pop(1)).resolve() / 'docs/msgpack/html'
GUIDES = {
    'msgpack_operation_modes': 'Operation modes',
    'msgpack_extensions': 'MessagePack Extensions',
    'msgpackreleasenotes': 'Release Notes',
}


class Page(HTMLParser):
    def __init__(self, path):
        super().__init__()
        self.ids = set()
        self.links = []
        self.text = []
        self.current = None
        self.feed(path.read_text())

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if 'id' in attrs:
            self.ids.add(attrs['id'])
        if tag == 'a' and 'href' in attrs:
            self.current = [attrs['href'], '']

    def handle_data(self, data):
        self.text.append(data)
        if self.current is not None:
            self.current[1] += data

    def handle_endtag(self, tag):
        if tag == 'a' and self.current is not None:
            self.links.append(tuple(self.current))
            self.current = None


class DocsTest(unittest.TestCase):
    def test_no_duplicate_contents_or_visible_anchor_identifiers(self):
        page = Page(HTML / 'index.html')
        self.assertNotIn('Contents of this documentation:', ''.join(page.text))
        for href, label in page.links:
            self.assertNotIn(label, GUIDES, href)
            self.assertNotIn(href, ['index.html#' + name for name in GUIDES])

    def test_titled_guide_links_preserve_old_bookmarks(self):
        page = Page(HTML / 'index.html')
        self.assertIn('msgpackintro', page.ids)
        for anchor, title in GUIDES.items():
            with self.subTest(guide=anchor):
                self.assertIn(anchor, page.ids)
                self.assertEqual(1, page.links.count((anchor + 'guide.html', title)))

    def test_guide_section_links_and_fragments_resolve(self):
        paths = [HTML / 'index.html', *(HTML / (name + 'guide.html') for name in GUIDES)]
        for path in paths:
            for href, _ in Page(path).links:
                url = urlsplit(href)
                if url.scheme or url.netloc:
                    continue
                with self.subTest(page=path.name, href=href):
                    target = path.parent / unquote(url.path) if url.path else path
                    self.assertTrue(target.is_file(), href)
                    if url.fragment:
                        self.assertIn(unquote(url.fragment), Page(target).ids, href)


if __name__ == '__main__':
    unittest.main()
