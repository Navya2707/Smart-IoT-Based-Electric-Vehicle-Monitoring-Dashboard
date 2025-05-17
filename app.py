from flask import Flask, request, jsonify, render_template
from flask_cors import CORS
import logging
import socket

app = Flask(__name__)
CORS(app)  

latest_data = {"voltage": 0, "percentage": 0, "charging": False}

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

def get_local_ip():
    """Get the local IP address of the machine"""
    try:
        host_name = socket.gethostname()
        host_ip = socket.gethostbyname(host_name)
        return host_ip
    except:
        return "Unable to get IP"

@app.route('/')
def home():
    return render_template('index.html')

@app.route('/data', methods=['POST'])
def receive_data():
    global latest_data
    try:
        data = request.get_json()
        if not data:
            logger.warning("No data received")
            return jsonify({"message": "No data received"}), 400
        
        voltage = float(data.get("voltage", 0))
        percentage = int(data.get("percentage", 0))
        
        latest_data = {
            "voltage": round(voltage, 2),
            "percentage": max(0, min(100, percentage)),
            "charging": data.get("charging", False)
        }

        logger.info(f"Received: {latest_data}")
        return jsonify({"status": "success"}), 200
        
    except Exception as e:
        logger.error(f"Error processing data: {str(e)}")
        return jsonify({"error": str(e)}), 500

@app.route('/latest-data', methods=['GET'])
def get_latest_data():
    return jsonify(latest_data)

if __name__ == "__main__":
    local_ip = get_local_ip()
    logger.info(f"Local IP address: {local_ip}")
    app.run(host="0.0.0.0", port=5000, debug=True)