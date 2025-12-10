import os
import shutil
from pathlib import Path
from bs4 import BeautifulSoup

# --- Paths ---
BASE_DIR = Path(__file__).resolve().parent
TEX_FILE = BASE_DIR / "main.tex"
CSS_FILE = BASE_DIR.parent / "Theme" / "theme.css"
OUTPUT_DIR = BASE_DIR
ASSETS_DIR = OUTPUT_DIR / "assets"

# --- Ensure assets directory exists ---
ASSETS_DIR.mkdir(exist_ok=True)

# --- CSS overrides ---
CUSTOM_OVERRIDE_CSS = """
/* Light background and full page */
html, body {
    margin: 0 !important;
    padding: 0 !important;
    width: 100% !important;
    background: #ffffff !important;
    font-size: 1rem !important;
    line-height: 1.6 !important;
}

/* Images: max 50% width & height, centered, no scroll */
img, figure img {
    max-width: 50% !important;
    max-height: 50vh !important;
    width: auto !important;
    height: auto !important;
    display: block !important;
    margin: 20px auto !important;
    object-fit: contain !important;
}

/* Remove wrappers shrinking images */
div, figure {
    max-width: 100% !important;
    width: 100% !important;
}

/* Tables: scrollable if necessary */
.table-container {
    overflow-x: auto;
    width: 100%;
}

/* Keep background light */
@media (prefers-color-scheme: dark) {
    body {
        background: white !important;
        color: black !important;
    }
}
"""

def convert_tex_to_html():
    os.chdir(BASE_DIR)
    output_html = OUTPUT_DIR / "index.html"
    
    cmd = f"""
    pandoc "{TEX_FILE}" \
    -f latex \
    -t html5 \
    --standalone \
    --mathjax \
    --toc \
    --toc-depth=3 \
    --css="{CSS_FILE}" \
    -o "{output_html}"
    """
    os.system(cmd)
    print(f"HTML created → {output_html}")
    return output_html

def enhance_html(html_file):
    with open(html_file, "r", encoding="utf-8") as f:
        soup = BeautifulSoup(f.read(), "html.parser")
    
    # Add viewport meta
    if not soup.find("meta", {"name": "viewport"}):
        viewport = soup.new_tag("meta", name="viewport", content="width=device-width, initial-scale=1.0")
        soup.head.append(viewport)

    # Add custom CSS
    style = soup.new_tag("style")
    style.string = CUSTOM_OVERRIDE_CSS
    soup.head.append(style)

    # Strip width/height from images to prevent stretching
    for img in soup.find_all("img"):
        if img.has_attr("width"): del img["width"]
        if img.has_attr("height"): del img["height"]
        img["style"] = "max-width:50%; max-height:50vh; width:auto; height:auto; display:block; margin:20px auto; object-fit:contain;"

    with open(html_file, "w", encoding="utf-8") as f:
        f.write(str(soup))

    print("HTML enhanced: images constrained to max 50% width/height")

def copy_images():
    print("Copying images...")
    folders_to_scan = [BASE_DIR, BASE_DIR / "Figures"]
    extensions = (".png", ".jpg", ".jpeg", ".gif", ".svg")
    
    for folder in folders_to_scan:
        if not folder.exists():
            continue
        for img in folder.rglob("*"):
            if img.suffix.lower() in extensions:
                target = ASSETS_DIR / img.name
                if img.resolve() != target.resolve():  # avoid SameFileError
                    shutil.copy2(img, target)
    print("Images copied to assets/")

if __name__ == "__main__":
    html = convert_tex_to_html()
    enhance_html(html)
    copy_images()
    
    print("\n✅ DONE: Website version successfully generated!")
    print(f"📍 Location: {html}")
