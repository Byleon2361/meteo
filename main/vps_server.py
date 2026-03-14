import socket
import threading
import queue
from concurrent.futures import ThreadPoolExecutor

TUNNEL_PORT = 9000
WEB_PORT    = 8080
MAX_WORKERS = 10

esp_pool = queue.Queue()
executor = ThreadPoolExecutor(max_workers=MAX_WORKERS)

def handle_browser(browser_sock, addr):
    try:
        browser_sock.settimeout(10)
        request = browser_sock.recv(8192)
        if not request:
            return

        first_line = request.split(b'\n')[0].decode(errors='replace').strip()
        print(f"[Browser {addr}] Запрос: {first_line}")

        try:
            esp_sock = esp_pool.get(timeout=5)
        except queue.Empty:
            browser_sock.sendall(b"HTTP/1.1 503 Service Unavailable\r\n\r\nESP32 not connected")
            return

        try:
            esp_sock.sendall(request)
            esp_sock.settimeout(15)
            response = b""
            while True:
                try:
                    chunk = esp_sock.recv(4096)
                    if not chunk:
                        break
                    response += chunk
                except socket.timeout:
                    break

            if response:
                browser_sock.sendall(response)
            else:
                browser_sock.sendall(b"HTTP/1.1 502 Bad Gateway\r\n\r\nEmpty response")

        except Exception as e:
            print(f"[Browser {addr}] Ошибка: {e}")
        finally:
            esp_sock.close()

    except Exception as e:
        print(f"[Browser {addr}] Ошибка: {e}")
    finally:
        browser_sock.close()

def tunnel_listener():
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(('0.0.0.0', TUNNEL_PORT))
    srv.listen(20)
    print(f"Жду ESP32 на порту {TUNNEL_PORT}...")
    while True:
        conn, addr = srv.accept()
        print(f"ESP32 подключился: {addr}  |  соединений в пуле: {esp_pool.qsize() + 1}")
        esp_pool.put(conn)

def web_listener():
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(('0.0.0.0', WEB_PORT))
    srv.listen(20)
    print(f"Веб-интерфейс доступен на порту {WEB_PORT}")
    while True:
        conn, addr = srv.accept()
        executor.submit(handle_browser, conn, addr)

threading.Thread(target=tunnel_listener, daemon=True).start()
web_listener()
