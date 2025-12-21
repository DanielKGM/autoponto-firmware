from flask import Flask, request
import cv2
import numpy as np

app = Flask(__name__)

@app.route('/upload', methods=['POST'])
def upload():
    # 1. Recebe os bytes brutos do corpo da requisição (fb->buf do ESP32)
    img_bytes = request.data
    
    if img_bytes:
        # 2. Converte os bytes para um array numpy
        nparr = np.frombuffer(img_bytes, np.uint8)
        
        # 3. Decodifica o JPEG para o formato BGR (padrão do OpenCV)
        # É aqui que o reconhecimento facial começaria
        img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
        
        if img is not None:
            # 4. Exibe a imagem em uma janela
            cv2.imshow("ESP32-CAM Stream", img)
            
            # Pequeno delay para o OpenCV atualizar a janela
            if cv2.waitKey(1) & 0xFF == ord('q'):
                return "Fechando...", 200
            
            print(f"Frame recebido! Tamanho: {len(img_bytes)} bytes")
            return "OK", 200
        
    print("Falha ao receber frame")
    return "Falha", 400

if __name__ == '__main__':
    # Rode no IP local da sua máquina (0.0.0.0 aceita conexões de outros dispositivos)
    # A porta padrão é 5000
    app.run(host='0.0.0.0', port=5000, debug=False)