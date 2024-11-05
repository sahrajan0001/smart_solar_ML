# SMART_SOLAR

# ESP32 MPPT Data Acquisition and Web Server

This project is designed to read data from an MPPT (Maximum Power Point Tracking) controller using an ESP32 and serve the collected data over a WiFi network as JSON through a built-in web server. This enables real-time monitoring of solar panel and battery parameters such as battery voltage, panel voltage, charge current, panel power, daily yield, total yield, and charging status. The data can be easily accessed and displayed in a browser.

## Features

- MPPT Communication: Uses ESP32’s HardwareSerial to read data from the MPPT controller at 19200 baud rate.
- WiFi Connectivity: Connects to a WiFi network and starts a server on port 80.

- Data Parsing: Reads and parses data labels such as
   `V` (battery voltage),
   `I` (charge current),
   `VPV` (panel voltage),
   `PPV` (panel power),
   `H19` (yield total),
   `H20` (yield today),
   `CS` (charge status).
  
- JSON Output: Serves data as JSON, making it easy to integrate with other applications.
- CORS Support: Allows cross-origin resource sharing (CORS) to enable external applications to access the data.

 Hardware Requirements

- ESP32 Board
- MPPT Controller (compatible with UART/Serial output)
- WiFi Network

 Software Requirements

- Arduino IDE (with ESP32 board support)
- ArduinoJson Library

## Setup Instructions

1. Clone the Repository:
   $bash : git clone https://github.com/sagarmollah/SMART_SOLAR.git
   

2. Configure WiFi Credentials:
   Modify the `ssid` and `password` variables in the source code to match your WiFi network.

3. Upload Code:
   Use the Arduino IDE to upload the code to your ESP32.

4. Connect MPPT Controller:
   Connect the MPPT controller to the ESP32’s Serial1 (TX: GPIO 25, RX: GPIO 26).

5. Run the Server:
   After uploading the code, the ESP32 will connect to WiFi and start a server on port 80. The device’s IP address will be displayed in the serial monitor.

6. Access Data:
   Navigate to `http://<ESP32_IP_address>` in a web browser to view the JSON output.

## JSON Output Format

The JSON response from the server will include the following fields:
json{
  "data": [
    {"sensor": "battery_voltage", "value": <value>, "unit": "Volts"},
    {"sensor": "panel_voltage", "value": <value>, "unit": "Volts"},
    {"sensor": "charge_current", "value": <value>, "unit": "Amps"},
    {"sensor": "panel_power", "value": <value>, "unit": "Watts"},
    {"sensor": "yield_today", "value": <value>, "unit": "kWh"},
    {"sensor": "yield_total", "value": <value>, "unit": "kWh"},
    {"sensor": "charge_status", "value": <value>, "unit": ""}
  ]
}


Each sensor field provides a value, unit, and error status.

## Troubleshooting

- WiFi Connection Issues: Ensure WiFi credentials are correct and that the ESP32 is within range of your same network .
- MPPT Communication Issues: Check wiring connections and confirm the MPPT controller's baud rate matches the code.

## Code Location

- The code for this project can be found in the following Google Drive link: (https://drive.google.com/file/d/1S79HAE6f-MLt2vkfSXu4jMtberxtDwCm/view?usp=sharing)


---
