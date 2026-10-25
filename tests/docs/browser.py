#!/usr/bin/env python3
"""Render Mermaid, exercise search and verify layout under the Pages subpath."""
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import re
import threading
from urllib.parse import urljoin, urlsplit

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


def check_traces(page, base, screenshots):
    response = page.request.get(base + "traceability.json")
    assert response.ok
    graph = response.json()
    manifest = page.request.get(base + "build.json").json()
    assert graph["revision"] == manifest["revision"]
    assert graph["source_sha256"] == manifest["source_sha256"]
    assert graph["working_tree_changes"] == manifest["working_tree_changes"]
    assert len(graph["stories"]) >= 116 and len(graph["criteria"]) >= 278 and len(graph["models"]) >= 80
    modules = sorted({story["id"][:3].upper() for story in graph["stories"]})
    for identity in modules:
        page.goto(base + f"trace/{identity}/")
        page.locator("[data-trace-ready]").wait_for()
        expected = [c for c in graph["criteria"] if c["id"][:3] == identity.lower()]
        assert page.locator(".trace-criterion").count() == len(expected), identity
        for criterion in expected:
            card = page.locator("#trace-" + criterion["id"])
            assert set(card.get_attribute("data-stories").split()) == set(criterion["stories"])
            assert card.get_by_role("link", name="Source criterion", exact=True).get_attribute("href").endswith("#" + criterion["id"])
    page.goto(base + "traceability/")
    assert page.locator('.md-content__inner a[href*="trace/"]').count() == len(modules)
    page.screenshot(path=str(screenshots / "trace-overview-desktop.png"))
    page.goto(base + "trace/M06/")
    story = page.get_by_label("Story", exact=True)
    story.select_option("m06-us02")
    assert page.locator(".trace-criterion:visible").count() == 2
    assert page.locator("[data-trace-count]").inner_text() == "Showing 2 of 8 acceptance criteria."
    query = page.get_by_label("Search this trace", exact=True)
    query.fill("SaveNeedsBoth")
    assert page.locator(".trace-criterion:visible").count() == 1
    assert page.locator("#trace-m06-ac04").is_visible()
    query.fill("no-such-trace-evidence-12345")
    assert page.locator(".trace-criterion:visible").count() == 0
    page.get_by_role("button", name="Reset", exact=True).focus()
    page.keyboard.press("Enter")
    assert page.locator(".trace-criterion:visible").count() == 8
    # A deep criterion link must reveal its target even with incompatible filters.
    page.goto(base + "trace/M06/?story=m06-us01#trace-m06-ac04")
    page.wait_for_function("!document.getElementById('trace-m06-ac04').hidden")
    assert page.get_by_label("Story", exact=True).input_value() == ""
    page.locator("#trace-m06-ac03").screenshot(path=str(screenshots / "trace-criterion-desktop.png"), animations="disabled", style="header, nav.md-tabs, .md-top, .md-sidebar { visibility: hidden !important; }")
    page.set_viewport_size({"width": 390, "height": 844})
    page.get_by_label("Story", exact=True).select_option("m06-us02")
    page.wait_for_function("document.documentElement.scrollWidth <= innerWidth + 1")
    page.screenshot(path=str(screenshots / "trace-mobile.png"), animations="disabled")
    page.locator("#trace-m06-ac03").screenshot(path=str(screenshots / "trace-criterion-mobile.png"), animations="disabled", style="header, nav.md-tabs, .md-top, .md-sidebar { visibility: hidden !important; }")
    model = page.locator("#trace-m06-ac03").get_by_role("link", name="M06Session.GuardedLeave", exact=True).get_attribute("href")
    page.goto(urljoin(page.url, model))
    page.locator("#def-m06session-guardedleave").wait_for(state="attached")
    definition = next(s for m in graph["models"] if m["path"] == "spec/dafny/M06-session.dfy"
                      for s in m["declarations"] if s["qualified"] == "M06Session.GuardedLeave")
    source = page.locator(f'main a[href$="/spec/dafny/M06-session.dfy#L{definition["line"]}"]').get_attribute("href")
    assert "/" + graph["revision"] + "/" in source
    page.set_viewport_size({"width": 1440, "height": 1000})
    backlink = page.get_by_role("link", name="Trace references", exact=True).get_attribute("href")
    assert "trace-models" in backlink
    page.goto(urljoin(page.url, backlink))
    references = page.locator("main details").filter(has=page.get_by_role("link", name="M06-AC03", exact=True, include_hidden=True)).first
    references.locator("summary").click()
    assert references.get_by_role("link", name="M06-AC03", exact=True).is_visible()
    page.goto(base + "trace-evidence/")
    assert page.locator('main a[href*="/tests/spec/check_s02_source.py"]').count() >= 1
    page.screenshot(path=str(screenshots / "trace-evidence-desktop.png"))
    no_js = page.context.browser.new_context(java_script_enabled=False, viewport={"width": 390, "height": 844})
    static = no_js.new_page()
    static.goto(base + "trace/M06/")
    assert static.locator(".trace-criterion:visible").count() == 8
    assert static.locator("[data-trace-controls]").is_hidden()
    assert static.locator('main a[href*="def-m06session-guardedleave"]').count() == 1
    no_js.close()
    print(f"Trace browser checks passed: {len(modules)} module views, all story/criterion edges, filters/reset, deep links, declaration/source lines, reverse links, mobile and no-JavaScript reading", flush=True)


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
            check_traces(page, base, screenshots)
            diagram_count = 0
            module_pages = 0
            sequence_count = 0
            data_count = 0
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
                sequence_count += sum(d.get("aria-roledescription") == "sequence" for d in rendered)
                data_count += sum(d.get("aria-roledescription") in {"class", "classDiagram", "er", "erDiagram"} for d in rendered)
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
            page.goto(base + "sequences/")
            sequence_links = page.locator('main a[href*="#"][href*="-seq-"]')
            assert sequence_links.count() >= 7, "Expected discovered player/host sequence links"
            targets = [link.get_attribute("href") for link in sequence_links.all()]
            page.screenshot(path=str(screenshots / "sequence-index-desktop.png"), full_page=True)
            for target in targets:
                page.goto(base + "sequences/" + target)
                anchor = target.split("#", 1)[1]
                page.locator(f'#{anchor}').wait_for(state="attached")
            page.goto(base + "data-models/")
            data_links = page.locator('main a[href*="#"][href*="-data-"]')
            assert data_links.count() >= 8, "Expected discovered shared-state/file diagrams including companions"
            targets = [link.get_attribute("href") for link in data_links.all()]
            page.screenshot(path=str(screenshots / "data-index-desktop.png"), full_page=True)
            for target in targets:
                page.goto(base + "data-models/" + target)
                page.locator('#' + target.split("#", 1)[1]).wait_for(state="attached")
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
            for route, name in (("", "home"), ("overview/", "overview"), ("sequences/", "sequences"), ("data-models/", "data-models"), ("diagrams/", "diagrams"), ("spec/M09-production-management/", "module")):
                page.set_viewport_size({"width": 390, "height": 844})
                page.goto(base + route)
                if route == "overview/":
                    page.wait_for_function("document.querySelectorAll('div.mermaid').length === 4")
                assert page.evaluate("document.documentElement.scrollWidth <= innerWidth + 1"), f"Horizontal page overflow: {route}"
                page.screenshot(path=str(screenshots / f"{name}-mobile.png"), full_page=True)
            browser.close()
            assert diagram_count >= 72 and module_pages >= 29 and sequence_count >= 7 and data_count >= 8, "Expected module, sequence and shared-state/file diagrams"
            assert not errors, "\n".join(errors)
            print(f"Browser checks passed: {diagram_count} Mermaid diagrams ({sequence_count} sequences, {data_count} data models) across {module_pages} specification pages, desktop/mobile layouts, generated indexes/anchors, search, requirement anchor, raw model download, and guide mobile layouts")
    finally:
        server.shutdown()
        server.server_close()


if __name__ == "__main__":
    main()
