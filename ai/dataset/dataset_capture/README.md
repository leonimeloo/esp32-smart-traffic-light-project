# Dataset Capture

This directory contains the tools used to capture images for the vehicle detection dataset.

The capture system uses an **ESP32-CAM** to take photographs and a **Flask server running on a notebook** to receive and store the images over the local Wi-Fi network.

This setup was created to simplify the process of collecting multiple images of the toy cars in different positions and configurations on the track.

## Architecture

The capture workflow is:

```text
ESP32-CAM
    │
    │ Wi-Fi / HTTP
    ▼
Notebook
    │
    └── Flask Server
          │
          └── Captured Images
```

The ESP32-CAM hosts a web interface that allows the user to control the image capture. Each captured JPEG image is sent directly to the computer through an HTTP `POST` request.

## Directory Structure

```text
dataset_capture/
├── README.md
├── esp32/
│   └── datasetphotos.ino
└── server/
    ├── main.py
    └── requirements.txt
```

### `esp32/datasetphotos.ino`

Firmware used by the ESP32-CAM during dataset collection.

It provides a web interface for controlling the camera and supports:

* Single image capture
* Sequential image capture
* Configurable number of photos
* Configurable interval between photos
* Starting and stopping capture sequences
* Camera preview
* Capture status monitoring

The camera captures images in JPEG format and sends them to the server over HTTP on local network.

### `server/main.py`

Flask server running on the computer.

It receives the images sent by the ESP32-CAM through:

```text
POST /upload
```

The ESP32-CAM sends the filename and sequence information through HTTP headers:

```text
X-Filename
X-Sequence
```

The server then saves the received image to the configured dataset directory.

## Requirements

### ESP32-CAM

* ESP32-CAM
* Wi-Fi network
* Arduino IDE or compatible ESP32 development environment

### Server

* Python 3
* Flask

Install the Python dependency with:

```bash
pip install -r requirements.txt
```

## Configuration

Before uploading the firmware to the ESP32-CAM, configure the Wi-Fi credentials and the address of the server in `datasetphotos.ino`.

The server URL should point to the computer's local IP address:

```cpp
http://COMPUTER_IP:8080/upload
```

The ESP32-CAM and computer must be connected to the same network, unless the network configuration provides another way for the ESP32-CAM to reach the server. You can also deploy the server on cloud and use this code.

## Running the Server

From the `server` directory:

```bash
cd ai/dataset/dataset_capture/server
```

Install the dependencies:

```bash
pip install -r requirements.txt
```

Start the server:

```bash
python main.py
```

The server listens on port `8080`.

The Flask application is configured to listen on all network interfaces, allowing the ESP32-CAM to connect to the computer through its local IP address.

## Capturing Images

After uploading the firmware:

1. Start the Flask server on your computer.
2. Connect the ESP32-CAM to the Wi-Fi network.
3. Open the ESP32-CAM's IP address in a web browser.
4. Position the toy cars on the track.
5. Use the web interface to capture individual images or sequences.
6. The ESP32-CAM sends each captured image to the computer.
7. The Flask server stores the received images.

This allows the dataset to be collected directly from the camera without manually transferring the photographs from the ESP32-CAM.

## Capture Sequences

The capture interface supports sequential image acquisition with configurable parameters such as:

```text
Photos per sequence
Interval between photos
```

This is useful for generating multiple variations of the same traffic configuration while changing the position or arrangement of the toy cars.

## Image Storage

The Flask server stores the received images in the configured dataset directory.

The ESP32-CAM also sends sequence and filename information with each request. The server logs information about received images, for example:

```text
[OK] seq_001/photo_001.jpg (12345 bytes)
```

## Purpose

These scripts are **dataset collection tools**, not part of the final traffic-light firmware.

They were created specifically to facilitate the generation of the training dataset used by the vehicle detection model.

Users who want to reproduce the project or create their own dataset can use and adapt these tools to perform a new image collection.
