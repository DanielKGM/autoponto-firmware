import socket
import cv2
import numpy as np

def main():
    s = socket.socket()
    s.bind(("0.0.0.0", 5000))
    s.listen(1)

    print("Aguardando ESP32...")
    conn, _ = s.accept()

    buffer = b''

    while True:
        data = conn.recv(4096)
        if not data:
            break

        buffer += data

        # JPEG termina com FFD9
        end = buffer.find(b'\xff\xd9')
        if end != -1:
            jpg = buffer[:end+2]
            buffer = buffer[end+2:]

            img = cv2.imdecode(
                np.frombuffer(jpg, dtype=np.uint8),
                cv2.IMREAD_COLOR
            )

            if img is not None:
                cv2.imshow("ESP32", img)
                cv2.waitKey(1)

if __name__ == "__main__":
    main()
