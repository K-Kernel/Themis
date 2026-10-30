"""Drives the shell in a headless browser and checks the picks, so the demo can be
re-checked in a few seconds after any change.

One-off setup, inside your venv:
    pip install playwright && python -m playwright install chromium

Start the example in one terminal, from the repository root:
    ./build/themis-meet-serve data/real/airports.dat data/real/routes.dat NCL EDI LHR CDG FRA JFK ORD SFO
then, in another:
    python tools/check_shell.py
"""
import sys

from playwright.sync_api import sync_playwright

URL = "http://127.0.0.1:8080/"
# (weights, method, expected pick); the same picks themis-meet prints
CASES = [
    ([5, 5, 5], "chebyshev", "KEF"),
    ([5, 5, 5], "weighted_sum", "DUB"),
    ([0, 10, 0], "chebyshev", "YHZ"),
    ([0, 0, 10], "chebyshev", "LHR"),
    ([10, 0, 0], "chebyshev", "DUB"),
]


def main() -> int:
    failures = 0
    with sync_playwright() as p:
        browser = p.chromium.launch()
        page = browser.new_page(viewport={"width": 1280, "height": 860})
        errors = []
        page.on("pageerror", lambda e: errors.append(str(e)))
        page.goto(URL)
        page.wait_for_selector("#pick strong", timeout=10000)
        counts = {
            "sliders": page.locator("#sliders input").count(),
            "front dots": page.locator("#front circle").count(),
            "map dots": page.locator("#map circle").count(),
            "detail rows": page.locator("#detail tr").count(),
        }
        print("title:", page.inner_text("#title"), counts)
        for weights, method, want in CASES:
            page.check(f"input[value={method}]")
            page.evaluate(
                """ws => { const s = document.querySelectorAll('#sliders input');
                           ws.forEach((w, k) => { s[k].value = w; s[k].dispatchEvent(new Event('input')); }); }""",
                weights,
            )
            try:
                page.wait_for_function(
                    "l => document.querySelector('#pick strong').textContent === l", arg=want, timeout=5000
                )
                print(f"ok    {weights} {method}: {want}")
            except Exception:
                failures += 1
                print(f"FAIL  {weights} {method}: want {want}, page shows {page.inner_text('#pick strong')}")
        page.screenshot(path="shell.png", full_page=True)
        browser.close()
    if errors:
        failures += 1
        print("page errors:", errors)
    print("screenshot: shell.png")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
