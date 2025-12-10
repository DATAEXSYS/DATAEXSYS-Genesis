import os
from pathlib import Path
from bs4 import BeautifulSoup

# --- Paths ---
BASE_DIR = Path(__file__).resolve().parent
HTML_FILE = BASE_DIR / "index.html"
ASSETS_DIR = BASE_DIR / "assets"

# --- Custom CSS for Cybersecurity / Networking theme ---
CUSTOM_CSS = """
:root {
    --primary: #1b263b;
    --secondary: #ff6f61;
    --accent: #0a9396;
    --background: #f5f5f5;
    --text: #111;
    --code-bg: #e0e0e0;
    --border-radius: 6px;
    --spacing: 1rem;
}

body {
    font-family: "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
    font-size: 1rem;
    line-height: 1.6;
    color: var(--text);
    background-color: var(--background);
    margin: 0;
    padding: var(--spacing);
    max-width: 100%;
    overflow-x: hidden;
}

/* Headings */
h1, h2, h3, h4, h5 {
    color: var(--primary);
    margin-top: 1.5em;
    margin-bottom: 0.5em;
}

/* Links */
a {
    color: var(--secondary);
    text-decoration: none;
}
a:hover {
    text-decoration: underline;
}

/* Tables */
table {
    border-collapse: collapse;
    width: 100%;
    margin: var(--spacing) 0;
}
th, td {
    border: 1px solid #ccc;
    padding: 0.5em 0.75em;
    text-align: left;
}
th {
    background-color: var(--primary);
    color: white;
}
tr:nth-child(even) {
    background-color: #e9ecef;
}

/* Code blocks */
pre, code {
    background-color: var(--code-bg);
    padding: 0.5em;
    border-radius: var(--border-radius);
    overflow-x: auto;
    font-family: "Fira Code", Consolas, monospace;
}

/* Images: responsive, max 50% width and height, no scrollbars */
img, figure img, embed {
    display: block;
    margin: var(--spacing) auto;
    max-width: 50%;
    max-height: 50%;
    width: auto;
    height: auto;
    object-fit: contain;
}

/* Figures */
figure {
    margin: var(--spacing) 0;
    text-align: center;
}
figcaption {
    font-size: 0.9em;
    color: #555;
    margin-top: 0.25em;
}

/* Disable overflow / scrollbars */
html, body {
    overflow-x: hidden;
}

/* Responsive adjustments */
@media (max-width: 768px) {
    img, figure img, embed {
        max-width: 100%;
        max-height: auto;
    }
    body {
        padding: 0.5rem;
    }
}

/* MathJax styling */
.MathJax {
    font-size: 1.1em !important;
    max-width: 100%;
    overflow-x: auto;
}
"""

def enhance_html(html_file_path: Path):
    """Enhance the existing HTML with custom CSS and fix image display."""
    if not html_file_path.exists():
        print(f"[Error] HTML file not found: {html_file_path}")
        return

    with open(html_file_path, "r", encoding="utf-8") as f:
        soup = BeautifulSoup(f.read(), "html.parser")

    # Add viewport meta tag if missing
    if not soup.find("meta", {"name": "viewport"}):
        viewport = soup.new_tag("meta", name="viewport", content="width=device-width, initial-scale=1.0")
        soup.head.append(viewport)

    # Inject custom CSS
    style_tag = soup.new_tag("style")
    style_tag.string = CUSTOM_CSS
    soup.head.append(style_tag)

    # Strip width/height inline attributes from images and embeds
    for tag in soup.find_all(["img", "embed"]):
        if tag.has_attr("width"): del tag["width"]
        if tag.has_attr("height"): del tag["height"]
        tag["style"] = "max-width:50%;max-height:50%;width:auto;height:auto;display:block;margin:1rem auto;object-fit:contain;"

    # Save enhanced HTML
    with open(html_file_path, "w", encoding="utf-8") as f:
        f.write(str(soup))

    print(f"✅ Post-processing complete: {html_file_path}")

if __name__ == "__main__":
    enhance_html(HTML_FILE)
