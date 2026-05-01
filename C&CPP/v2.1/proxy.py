import socket
import urllib.request
import re
import time

def strip_tags(html):
    text = re.sub('<style.*?</style>', '', html, flags=re.DOTALL|re.IGNORECASE)
    text = re.sub('<script.*?</script>', '', text, flags=re.DOTALL|re.IGNORECASE)
    text = re.sub('<[^<]+>', '\n', text)
    text = re.sub('\n+', '\n', text)
    return text.strip()

print("Waiting for QEMU to start on port 4444...")

while True:
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(('127.0.0.1', 4444))
        print("Connected to TiwutOS!")
        buffer = ""
        while True:
            data = s.recv(1024)
            if not data: break
            buffer += data.decode('utf-8', errors='ignore')
            if '\n' in buffer:
                url = buffer.strip()
                buffer = ""
                if not url.startswith('http'):
                    url = 'https://' + url
                print(f"TiwutOS requested: {url}")
                try:
                    req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64)'})
                    with urllib.request.urlopen(req, timeout=5) as response:
                        html = response.read().decode('utf-8', errors='ignore')
                        text = strip_tags(html)
                        text = text[:3800] # Limit to avoid OS buffer overflow
                        s.sendall((text + "\n").encode('utf-8'))
                        print("Response sent to TiwutOS.")
                except Exception as e:
                    print("Error:", e)
                    s.sendall(f"Error fetching {url}: {str(e)}\n".encode('utf-8'))
    except ConnectionRefusedError:
        time.sleep(2)
    except Exception as e:
        print("Lost connection, retrying...", e)
        time.sleep(2)
