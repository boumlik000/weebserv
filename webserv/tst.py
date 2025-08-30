import socket
import time

print("[CLIENT] Connecting to server...")
# 1. نتكونيكطاو
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('127.0.0.1', 8080))
print("[CLIENT] Connected.")

# 2. نصيفطو بيانات ناقصة (ماشي طلب كامل)
print("[CLIENT] Sending partial data...")
s.send(b'GET /incomplete_request ')
print("[CLIENT] Partial data sent.")

# 3. نبقاو مكونيكطين للأبد بلا ما نصيفطو والو آخر
print("[CLIENT] Now keeping connection alive forever... Server should be frozen.")
print("[CLIENT] Press Ctrl+C to stop me.")

try:
    while True:
        # هاد الحلقة الخاوية كتخلي السكريبت خدام والإتصال مفتوح
        time.sleep(1)
except KeyboardInterrupt:
    print("\n[CLIENT] OK, closing connection now.")
    s.close()