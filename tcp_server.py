from flask import Flask, request

app = Flask(__name__)

@app.route('/upload', methods=['POST'])
def upload():
    # Tenta ler como texto primeiro
    content_type = request.headers.get('Content-Type')
    
    if content_type == 'text/plain':
        data = request.data.decode('utf-8')
        print(f"Texto recebido: {data}")
        return "Texto recebido com sucesso!", 200
    
    # Caso você envie bytes depois
    img_bytes = request.data
    print(f"Dados binários recebidos! Tamanho: {len(img_bytes)} bytes")
    return "Dados binários recebidos!", 200

if __name__ == '__main__':
    # Certifique-se de que o IP do seu PC é acessível pelo ESP32
    print("Servidor aguardando conexões...")
    app.run(host='0.0.0.0', port=5000, debug=False)