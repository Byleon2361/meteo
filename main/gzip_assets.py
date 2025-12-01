import gzip
import hashlib
from pathlib import Path


def gzip_file(src: Path, dst: Path):
    data = src.read_bytes()
    with gzip.open(dst, "wb", compresslevel=9) as f:
        f.write(data)


def main():
    root = Path(__file__).parent  # Исправлено: __file__
    data_dir = root / "data"

    tpl_html = data_dir / "page.template.html"
    out_html = data_dir / "page.html"
    css_file = data_dir / "style.css"
    js_file = data_dir / "script.js"

    # Проверяем существование файлов
    if not tpl_html.exists():
        print(f"Error: Template not found: {tpl_html}")
        return
    if not css_file.exists():
        print(f"Error: CSS file not found: {css_file}")
        return
    if not js_file.exists():
        print(f"Error: JS file not found: {js_file}")
        return

    # hashes
    css_hash = hashlib.sha1(css_file.read_bytes()).hexdigest()[:8]
    js_hash = hashlib.sha1(js_file.read_bytes()).hexdigest()[:8]

    # generate page.html from template
    html = tpl_html.read_text(encoding="utf-8")
    html = html.replace("STYLE_HASH", css_hash)
    html = html.replace("SCRIPT_HASH", js_hash)
    out_html.write_text(html, encoding="utf-8")
    print(f"GZIP assets: page.html (style={css_hash}, script={js_hash})")

    # gzip all three: page.html, style.css, script.js
    gzip_file(out_html, data_dir / "page.html.gz")
    gzip_file(css_file, data_dir / "style.css.gz")
    gzip_file(js_file, data_dir / "script.js.gz")
    print("GZIP done.")


if __name__ == "__main__":  # Исправлено: __name__ и __main__
    main()