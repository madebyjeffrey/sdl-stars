#!/usr/bin/env python3
"""Render Mermaid, exercise search and verify layout under the Pages subpath."""
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import re
import threading
from urllib.parse import urlsplit

from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[2]
SITE = ROOT / "dist/handbook/site"
PREFIX = urlsplit(os.environ.get("HANDBOOK_SITE_URL", "https://madebyjeffrey.github.io/sdl-stars/")).path.rstrip("/") + "/"


class Handler(SimpleHTTPRequestHandler):
    def do_GET(self):
        if not self.path.startswith(PREFIX):
            self.send_error(404)
            return
        self.path = "/" + self.path[len(PREFIX):]
        super().do_GET()

    def log_message(self, *_):
        pass


def main():
    server = ThreadingHTTPServer(("127.0.0.1", 0), partial(Handler, directory=str(SITE)))
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    base = f"http://127.0.0.1:{server.server_port}{PREFIX}"
    errors = []
    screenshots = ROOT / "dist/handbook/screenshots"
    screenshots.mkdir(parents=True, exist_ok=True)
    try:
        with sync_playwright() as playwright:
            browser = playwright.chromium.launch()
            page = browser.new_page(viewport={"width": 1440, "height": 1000}, device_scale_factor=1)
            page.on("pageerror", lambda error: errors.append(str(error)))
            page.on("requestfailed", lambda request: print(f"Request failed: {request.url}: {request.failure}", flush=True))
            page.on("response", lambda response: errors.append(f"HTTP {response.status}: {response.url}")
                    if response.status >= 400 and response.url.startswith(base) else None)
            diagram_count = 0
            module_pages = 0
            for path in sorted(SITE.rglob("*.html")):
                count = len(re.findall(r'class="mermaid"', path.read_text(encoding="utf-8")))
                if not count:
                    continue
                relative = path.relative_to(SITE).as_posix().removesuffix("index.html")
                page.goto(base + relative)
                print(f"Rendering {count} diagrams: {relative}", flush=True)
                # Zensical renders SVG into closed shadow roots, replacing each pre.
                try:
                    page.wait_for_function("expected => document.querySelectorAll('div.mermaid').length === expected", arg=count)
                    page.wait_for_function("expected => document.querySelectorAll('section.handbook-diagram').length === expected", arg=count)
                except Exception:
                    print(f"Diagram errors on {relative}: {errors}", flush=True)
                    page.screenshot(path=str(screenshots / f"{path.parent.name}-failure.png"), full_page=True)
                    raise
                session = page.context.new_cdp_session(page)
                document = session.send("DOM.getDocument", {"depth": -1, "pierce": True})
                def diagrams(node):
                    attrs = dict(zip(node.get("attributes", [])[::2], node.get("attributes", [])[1::2]))
                    found = [attrs] if node.get("nodeName", "").lower() == "svg" and attrs.get("id", "").startswith("__mermaid_") else []
                    for child in node.get("children", []) + node.get("shadowRoots", []):
                        found += diagrams(child)
                    return found
                rendered = diagrams(document["root"])
                assert len(rendered) == count and all(d.get("aria-roledescription") != "error" for d in rendered), path
                session.detach()
                if path.relative_to(SITE).parts[0] == "spec":
                    module_pages += 1
                    assert all(not d.get("width", "").endswith("%") for d in rendered), "Expected natural-size module SVGs"
                    for index in range(count):
                        page.locator("section.handbook-diagram").nth(index).screenshot(path=str(screenshots / f"{path.parent.name}-{index}-desktop.png"), style="header, nav.md-tabs, .md-top, .md-sidebar { visibility: hidden !important; }")
                    page.set_viewport_size({"width": 390, "height": 844})
                    page.wait_for_function("document.documentElement.scrollWidth <= innerWidth + 1")
                    for index in range(count):
                        frame = page.locator("section.handbook-diagram").nth(index)
                        frame.get_by_role("button", name="Actual size", exact=True).click()
                        diagram = frame.locator("div.mermaid")
                        assert diagram.evaluate("e => Number(e.style.zoom) === 1 && e.tabIndex === 0")
                        assert page.evaluate("document.documentElement.scrollWidth <= innerWidth + 1"), path
                        diagram.evaluate("e => { e.scrollLeft = e.scrollWidth; }")
                        assert diagram.evaluate("e => e.scrollWidth <= e.clientWidth || e.scrollLeft > 0"), path
                        diagram.evaluate("e => { e.scrollLeft = (e.scrollWidth - e.clientWidth) / 2; }")
                        frame.screenshot(path=str(screenshots / f"{path.parent.name}-{index}-mobile.png"), style="header, nav.md-tabs, .md-top, .md-sidebar { visibility: hidden !important; }")
                        frame.get_by_role("button", name="Fit to width", exact=True).click()
                        assert frame.get_by_role("button", name="Fit to width", exact=True).get_attribute("aria-pressed") == "true"
                    page.set_viewport_size({"width": 1440, "height": 1000})
                else:
                    page.screenshot(path=str(screenshots / f"{path.parent.name}-desktop.png"), full_page=True)
                diagram_count += count
            page.goto(base)
            page.screenshot(path=str(screenshots / "home-desktop.png"), full_page=True)
            page.get_by_role("button", name=re.compile("Search")).click()
            search = page.locator("input:visible").first
            search.fill("M09-AC01")
            page.locator('dialog[open] a[href*="M09-production-management"]').first.wait_for()
            page.screenshot(path=str(screenshots / "search-desktop.png"))
            page.keyboard.press("Escape")
            page.goto(base + "spec/M09-production-management/#m09-ac01")
            page.locator("#m09-ac01").wait_for(state="attached")
            page.goto(base + "models/M18-generation-specification/")
            assert page.locator("main").inner_text().find("M18-generation-specification.dfy") >= 0
            raw = page.locator('main a[href$=".dfy"]').first.get_attribute("href")
            response = page.request.get(base + "models/M18-generation-specification/" + raw)
            assert response.ok and response.body() == (ROOT / "spec/M18-generation-specification.dfy").read_bytes()
            page.goto(base)
            page.locator('label[title="Switch to light mode"]').click()
            page.screenshot(path=str(screenshots / "home-light.png"), full_page=True)
            for route, name in (("", "home"), ("overview/", "overview"), ("spec/M09-production-management/", "module")):
                page.set_viewport_size({"width": 390, "height": 844})
                page.goto(base + route)
                if route == "overview/":
                    page.wait_for_function("document.querySelectorAll('div.mermaid').length === 4")
                assert page.evaluate("document.documentElement.scrollWidth <= innerWidth + 1"), f"Horizontal page overflow: {route}"
                page.screenshot(path=str(screenshots / f"{name}-mobile.png"), full_page=True)
            browser.close()
            assert diagram_count >= 57 and module_pages >= 26, "Expected workflow and state diagrams in all module entries"
            assert not errors, "\n".join(errors)
            print(f"Browser checks passed: {diagram_count} Mermaid diagrams across {module_pages} module pages, module mobile layouts, search, requirement anchor, raw model download, and three guide mobile layouts")
    finally:
        server.shutdown()
        server.server_close()


if __name__ == "__main__":
    main()
