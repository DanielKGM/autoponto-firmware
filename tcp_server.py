import socket
import struct
import cv2
import numpy as np

HOST = "0.0.0.0"
PORT = 9000

def recv_exact(sock, size):
    data = b''
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            return None
        data += chunk
    return data

print("=== TCP CAMERA VIEWER ===")
print("Aguardando conexão...")

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, PORT))
    s.listen(1)

    conn, addr = s.accept()
    print(f"Conectado: {addr}")

    with conn:
        while True:
            # 1) recebe tamanho do frame
            header = recv_exact(conn, 4)
            if not header:
                break

            (length,) = struct.unpack("<I", header)

            # sanity check
            if length < 100 or length > 300_000:
                print(f"Tamanho inválido: {length}")
                break

            # 2) recebe JPEG
            jpeg_data = recv_exact(conn, length)
            if not jpeg_data:
                break

            print(f"JPEG recebido: {length} bytes")

            # 3) decodifica JPEG -> imagem
            jpg_array = np.frombuffer(jpeg_data, dtype=np.uint8)
            frame = cv2.imdecode(jpg_array, cv2.IMREAD_COLOR)

            if frame is None:
                print("Erro ao decodificar JPEG")
                continue

            # 4) exibe
            cv2.imshow("ESP32 Camera", frame)

            # ESC para sair
            if cv2.waitKey(1) & 0xFF == 27:
                break

cv2.destroyAllWindows()
print("Encerrado")
