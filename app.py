from flask import Flask, jsonify, render_template
import threading
import time
import serial

app = Flask(__name__)

# Shared data to store sensor readings
sensor_data = {"ph": 0, "tds": 0, "turbidity": 0, "flowrate": 0}

# Initialize the serial connection
try:
    ser = serial.Serial('COM3', 115200)  # Adjust 'COM5' and baud rat*e as needed
    print("Serial connection established.")
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
    ser = None

def generate_sensor_readings():
    global sensor_data
    while ser:  # Ensure the serial connection is valid
        try:
            # Read a line of data from the serial port
            line = ser.readline().decode('utf-8').strip()
            
            # Process the line if it contains expected data
            if "FlowRate:" in line:
                # Parse the data into key-value pairs
                data = line.split(",")
                parsed_data = {}
                for item in data:
                    key, value = item.split(":")
                    parsed_data[key] = float(value)  # Convert to float

                # Update the shared sensor data dictionary
                sensor_data["ph"] = parsed_data.get("Ph", 0.0)
                sensor_data["tds"] = parsed_data.get("TDS", 0.0)
                sensor_data["turbidity"] = parsed_data.get("TBS", 0.0)
                sensor_data["flowrate"] = parsed_data.get("FlowRate", 0.0)

                # Print the parsed data in the console
                print("Sensor Data (Console Log):")
                for key, value in sensor_data.items():
                    print(f"  {key}: {value:.2f}")

        except Exception as e:
            print(f"Error reading serial data: {e}")


# API route to fetch the current sensor readings
@app.route("/api/sensor-data")
def get_sensor_data():
    return jsonify(sensor_data)

# Route to serve the webpage
@app.route("/")
def index():
    return render_template("data.html")

if __name__ == "__main__":
    if ser:  # Only start the thread if the serial connection is valid
        print("Starting serial data reading thread...")
        # Start the sensor data generation in a separate thread
        threading.Thread(target=generate_sensor_readings, daemon=True).start()
    else:
        print("No valid serial connection. Only serving Flask app.")

    # Start the Flask application
    print("Starting Flask server...")
    app.run(debug=True, use_reloader=False)  # Turn off reloader to avoid conflicts with threads
