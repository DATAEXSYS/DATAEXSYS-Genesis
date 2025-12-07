import os
import shutil
from pathlib import Path
from bs4 import BeautifulSoup
import subprocess

# --- Paths ---
BASE_DIR = Path(__file__).resolve().parent
TEX_FILE = BASE_DIR / "main.tex"
OUTPUT_DIR = BASE_DIR
ASSETS_DIR = OUTPUT_DIR / "assets"

# Ensure assets folder exists
ASSETS_DIR.mkdir(exist_ok=True)

CUSTOM_CSS = """
html, body {
    margin:0 !important;
    padding:0 !important;
    width:100% !important;
    background:white !important;
}
body {
    max-width:100% !important;
    padding:20px !important;
}
img, figure img {
    width:100% !important;
    height:auto !important;
    max-width:100% !important;
    display:block !important;
    margin:20px 0 !important;
    object-fit:contain !important;
}
div, figure {
    max-width:100% !important;
    width:100% !important;
}
.table-container { overflow: visible !important; }
p, li, td { font-size:1rem !important; line-height:1.6 !important; }
"""

# --- Convert LaTeX to HTML using Pandoc ---
def convert_tex_to_html():
    html_file = OUTPUT_DIR / "index.html"
    cmd = [
        "pandoc",
        str(TEX_FILE),
        "-f", "latex",
        "-t", "html5",
        "--standalone",
        "--toc",
        "--mathjax",
        "-o", str(html_file)
    ]
    subprocess.run(cmd, check=True)
    return html_file

# --- Enhance HTML: embed all images + convert PDFs to PNG ---
def enhance_html(html_file):
    soup = BeautifulSoup(open(html_file, "r", encoding="utf-8"), "html.parser")

    # Add viewport
    if not soup.find("meta", {"name": "viewport"}):
        viewport = soup.new_tag("meta", name="viewport", content="width=device-width, initial-scale=1.0")
        soup.head.append(viewport)

    # Force custom CSS
    style = soup.new_tag("style")
    style.string = CUSTOM_CSS
    soup.head.append(style)

    # Process <img> tags
    for img in soup.find_all("img"):
        src = img.get("src")
        if src:
            src_path = BASE_DIR / src
            if src_path.exists():
                target = ASSETS_DIR / src_path.name
                shutil.copy2(src_path, target)
                img["src"] = f"assets/{src_path.name}"
                img["style"] = "width:100%;height:auto;display:block;"

    # Process <embed> PDFs
    for embed in soup.find_all("embed"):
        src = embed.get("src")
        if src and src.lower().endswith(".pdf"):
            pdf_path = BASE_DIR / src
            if pdf_path.exists():
                # Convert PDF to PNG
                png_name = pdf_path.stem + ".png"
                png_path = ASSETS_DIR / png_name
                subprocess.run([
                    "pdftoppm",
                    "-singlefile",
                    "-png",
                    str(pdf_path),
                    str(png_path.with_suffix(""))
                ], check=True)

                # Replace embed with img tag
                new_img = soup.new_tag("img", src=f"assets/{png_name}")
                embed.replace_with(new_img)

    # Save modified HTML
    with open(html_file, "w", encoding="utf-8") as f:
        f.write(str(soup))

# --- Main ---
if __name__ == "__main__":
    print("Converting LaTeX to HTML...")
    html_file = convert_tex_to_html()
    print("Enhancing HTML and embedding images...")
    enhance_html(html_file)
    print(f"\n✅ Website generated: {html_file}")
