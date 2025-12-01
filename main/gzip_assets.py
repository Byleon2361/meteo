import gzip
import hashlib
from pathlib import Path


def gzip_file(src: Path, dst: Path):
    """Сжимает файл в gzip формат"""
    data = src.read_bytes()
    with gzip.open(dst, "wb", compresslevel=9) as f:
        f.write(data)
    print(f"  Сжат: {src.name} -> {dst.name}")


def main():
    # Получаем директорию, где находится этот скрипт
    root = Path(__file__).parent  # Один уровень вверх (папка с gzip_assets.py)
    data_dir = root / "data"
    
    # Проверяем существование директории data
    if not data_dir.exists():
        print(f"Ошибка: Директория не найдена: {data_dir}")
        print("Текущая структура проекта:")
        for item in root.iterdir():
            print(f"  {item.name}/" if item.is_dir() else f"  {item.name}")
        return

    tpl_html = data_dir / "page.template.html"
    out_html = data_dir / "page.html"
    css_file = data_dir / "style.css"
    js_file = data_dir / "script.js"
    # chart_file = data_dir / "chart.min.js"  # УДАЛИЛИ chart.js

    # Проверяем существование обязательных файлов
    required_files = [tpl_html, css_file, js_file]
    missing_files = []
    
    for file in required_files:
        if not file.exists():
            missing_files.append(file)
    
    if missing_files:
        print("Ошибка: Отсутствуют файлы:")
        for file in missing_files:
            print(f"  - {file.name}")
        print("\nСодержимое папки data:")
        for item in data_dir.iterdir():
            print(f"  {item.name}")
        return

    # Создаем хеши для кэширования
    css_hash = hashlib.sha1(css_file.read_bytes()).hexdigest()[:8]
    js_hash = hashlib.sha1(js_file.read_bytes()).hexdigest()[:8]
    
    # chart_hash больше не нужен
    # chart_hash = ""
    # if chart_file.exists():
    #     chart_hash = hashlib.sha1(chart_file.read_bytes()).hexdigest()[:8]

    # Генерируем page.html из шаблона
    print("Чтение шаблона...")
    html = tpl_html.read_text(encoding="utf-8")
    html = html.replace("__STYLE_HASH__", css_hash)
    html = html.replace("__SCRIPT_HASH__", js_hash)
    # Удаляем все упоминания о chart hash
    html = html.replace("__CHART_HASH__", "")  # Просто удаляем
    
    out_html.write_text(html, encoding="utf-8")
    print(f"✓ Сгенерирован: page.html (css={css_hash}, js={js_hash})")

    # Сжимаем все файлы в gzip
    print("\nСжатие файлов:")
    
    # Основные файлы (без chart.js)
    files_to_compress = [
        (out_html, "page.html.gz"),
        (css_file, "style.css.gz"),
        (js_file, "script.js.gz")
    ]
    
    # НЕ добавляем chart.js
    
    # Сжимаем все файлы
    for src_file, gz_name in files_to_compress:
        gzip_file(src_file, data_dir / gz_name)
    
    # Создаем список файлов для CMakeLists.txt
    print("\n" + "="*50)
    print("Для CMakeLists.txt добавьте эти файлы в EMBED_FILES:")
    
    gz_files = [f[1] for f in files_to_compress]
    for gz_file in gz_files:
        print(f'    "data/{gz_file}"')
    
    print("\nДля webserver.c добавьте обработчики:")
    for src_file, gz_name in files_to_compress:
        name = src_file.name
        var_name = name.replace(".", "_")
        print(f'  // Для {name}:')
        print(f'  extern const uint8_t _binary_{var_name}_gz_start[];')
        print(f'  extern const uint8_t _binary_{var_name}_gz_end[];')
        print(f'  // URI: /{name}')
        print()
    
    print("="*50)
    print("✓ Все файлы успешно обработаны!")


if __name__ == "__main__":
    main()