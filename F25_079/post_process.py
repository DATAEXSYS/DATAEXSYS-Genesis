import os
from pathlib import Path
from bs4 import BeautifulSoup
from pdf2image import convert_from_path

# --- Paths ---
BASE_DIR = Path(__file__).resolve().parent
HTML_FILE = BASE_DIR / "index.html"
FIGURES_DIR = BASE_DIR / "Figures"
ASSETS_DIR = BASE_DIR / "assets"

ASSETS_DIR.mkdir(exist_ok=True)

# --- Convert PDFs to high-res PNGs ---
print("Converting PDFs to high-res PNGs...")
for pdf_file in FIGURES_DIR.glob("*.pdf"):
    images = convert_from_path(str(pdf_file), dpi=400)  # higher DPI for clarity
    png_path = ASSETS_DIR / f"{pdf_file.stem}.png"
    images[0].save(png_path, "PNG")
    print(f"{pdf_file.name} -> {png_path.name}")

# --- Update HTML ---
print("Updating HTML...")
with open(HTML_FILE, "r", encoding="utf-8") as f:
    soup = BeautifulSoup(f, "html.parser")

for fig in soup.find_all("figure"):
    if "data-latex-placement" in fig.attrs:
        del fig["data-latex-placement"]
    
    embed = fig.find("embed")
    if embed:
        pdf_src = embed.get("src")
        png_name = Path(pdf_src).stem + ".png"
        img_tag = soup.new_tag("img", src=f"assets/{png_name}")
        # Original size, responsive, max 50% width/height
        img_tag["style"] = (
            "display:block;"
            "margin:20px auto;"
            "width:auto;"
            "height:auto;"
            "max-width:50%;"
            "max-height:50vh;"
        )
        embed.replace_with(img_tag)

# --- Ensure all <img> follow same style ---
for img in soup.find_all("img"):
    img["style"] = (
        "display:block;"
        "margin:20px auto;"
        "width:auto;"
        "height:auto;"
        "max-width:50%;"
        "max-height:50vh;"
    )

# Save HTML
with open(HTML_FILE, "w", encoding="utf-8") as f:
    f.write(str(soup))

print("✅ Post-processing done! Images replaced with high-res PNGs, no scrollbars.")
