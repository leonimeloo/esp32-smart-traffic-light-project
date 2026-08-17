# Computer Vision API

This directory contains the backend API responsible for processing images captured by the ESP32 cameras.

The API receives an image, runs the trained YOLO model, processes the resulting detections, and returns structured information that can be used by the traffic light control system.

The API can run locally during development or be packaged as a Docker container for cloud deployment.

---

## Architecture

The API acts as the bridge between the ESP32 cameras and the traffic control system.

```text
+----------------------+
|      ESP32-CAM       |
|                      |
|  Capture JPEG image  |
+----------+-----------+
           |
           | HTTP POST
           v
+----------------------+
|                      |
|      FastAPI         |
|                      |
|   Image Processing   |
+----------+-----------+
           |
           v
+----------------------+
|                      |
|     YOLO Model       |
|                      |
|  Vehicle Detection   |
+----------+-----------+
           |
           v
+----------------------+
|    Detection Result  |
|                      |
|  Vehicle count       |
|  Confidence          |
|  Bounding boxes      |
+----------+-----------+
           |
           | JSON
           v
      Main ESP32
```

---

## Directory Structure

```text
api/
|
+-- app/
|   +-- main.py
|
+-- .env.example
+-- .gitignore
+-- Dockerfile
+-- requirements.txt
+-- README.md
```

---

## Responsibilities

The API is responsible for:

* Receiving images from ESP32 cameras.
* Validating incoming requests.
* Running YOLO inference.
* Extracting detected objects.
* Calculating the number of detected vehicles.
* Returning structured JSON responses.
* Providing an HTTP interface for the embedded system.

The API does not control the traffic lights directly.

The final traffic control decisions are handled by the main ESP32.

---

# Requirements

The API requires:

* Python 3.x
* The dependencies listed in `requirements.txt`
* A trained YOLO model
* Network connectivity between the API server and the ESP32 devices

Install the Python dependencies with:

```bash
pip install -r requirements.txt
```

---

# Configuration

The API uses environment-specific configuration.

An example environment file is provided:

```text
.env.example
```

Copy it to:

```text
.env
```

and configure the required values for the local environment.

---

# Running Locally

From the API directory:

```bash
cd api
```

Install the dependencies:

```bash
pip install -r requirements.txt
```

Start the server:

```bash
uvicorn app.main:app --reload
```

The API will be available at:

```text
http://localhost:8000
```

FastAPI automatically provides interactive API documentation at:

```text
http://localhost:8000/docs
```

The OpenAPI schema is also available through:

```text
http://localhost:8000/openapi.json
```

---

# Detection Endpoint

The primary endpoint receives an image and performs vehicle detection.

```text
POST /detect
```

The request uses `multipart/form-data`.

Example:

```bash
curl -X POST \
  "http://localhost:8000/detect" \
  -H "accept: application/json" \
  -F "imagem=@image.jpg"
```

Replace `image.jpg` with the image you want to process.

---

# Response

The API returns a JSON response containing information about the processed image, model configuration, and detected objects.

A simplified example:

```json
{
  "sucesso": true,
  "imagem": {
    "nome": "image.jpg",
    "tipo": "image/jpeg",
    "largura": 160,
    "altura": 120
  },
  "modelo": {
    "arquivo": "best.pt",
    "tipo": "YOLO",
    "classes": {
      "0": "car"
    }
  },
  "configuracao": {
    "confianca_minima": 0.2,
    "iou": 0.5,
    "imgsz": 640
  },
  "resultado": {
    "quantidade_deteccoes": 1,
    "deteccoes": []
  }
}
```

The exact response structure depends on the current implementation in:

```text
app/main.py
```

---

# Model

The API uses the trained YOLO model developed in the `ai` directory.

The current preferred model is:

```text
best.pt
```

The model is trained to detect the target vehicle class used by the project.

Model documentation is available at:

```text
../ai/models/README.md
```

The API should be configured to load the correct model path before starting the server.

---

# Running with Docker

The API includes a Dockerfile for containerized execution.

From the `api` directory, build the image:

```bash
docker build -t smart-traffic-light-api .
```

Run the container:

```bash
docker run -p 8000:8000 smart-traffic-light-api
```

The API will then be available at:

```text
http://localhost:8000
```

Interactive API documentation:

```text
http://localhost:8000/docs
```

---

# ESP32 Integration

The ESP32 cameras communicate with the API through HTTP.

The general flow is:

```text
ESP32-CAM
    |
    | POST image
    v
POST /detect
    |
    v
YOLO inference
    |
    v
JSON response
    |
    v
ESP32
```

When the API is running on a local computer, the ESP32 must connect to the computer using its local network IP address.

For example:

```text
http://192.168.1.100:8000/detect
```

Do not configure the ESP32 to use:

```text
http://localhost:8000/detect
```

because `localhost` would refer to the ESP32 itself.

The configured API URL can be found in the corresponding camera firmware under:

```text
../esp32/camera-1/
../esp32/camera-2/
```

---

# Cloud Deployment

The API was designed to support containerized deployment and was deployed during the project using Google Cloud Run.

The deployment architecture is:

```text
ESP32-CAM
    |
    | HTTPS
    v
+----------------------+
|    Google Cloud Run  |
|                      |
|       FastAPI        |
|          +           |
|      YOLO model      |
+----------+-----------+
           |
           | JSON
           v
       ESP32 system
```

The Docker configuration used for deployment is located at:

```text
Dockerfile
```

The same container can also be used for local testing.

---

# Local vs. Cloud API

The camera firmware can be configured to communicate with either a local API instance or a deployed API.

### Local development

```text
ESP32
  |
  | HTTP
  v
Local Computer
  |
  +-- FastAPI
  +-- YOLO
```

Example:

```text
http://192.168.1.100:8000/detect
```

### Cloud deployment

```text
ESP32
  |
  | HTTPS
  v
Cloud Run
  |
  +-- FastAPI
  +-- YOLO
```

The appropriate endpoint must be configured in the ESP32 firmware.

---

# Performance Considerations

The inference process is performed outside the ESP32.

The ESP32 is responsible for:

* Capturing images.
* Encoding images.
* Sending images.
* Receiving detection results.

The computationally heavier YOLO inference is performed by the API server.

This architecture allows the project to use a larger computer vision model than would be practical to execute directly on the ESP32.

---

# Troubleshooting

## API does not start

Check that:

* Python dependencies are installed.
* The configured model exists.
* The correct working directory is being used.
* The configured port is available.

## ESP32 cannot reach the API

Check:

* ESP32 Wi-Fi connection.
* Computer's local IP address.
* API port.
* Firewall rules.
* Network isolation.
* API endpoint configured in the firmware.

The ESP32 and the local API server must be able to communicate over the network.

## No detections

If the API returns zero detections:

* Verify that the correct model is being loaded.
* Check the confidence threshold.
* Check the image resolution.
* Verify that the image represents the conditions used during training.
* Test the image independently using [`ai/inference.py`](../ai/inference.py).

---

# Related Documentation

For information about the trained models, see [MODELS README](../ai/models/README.md)

For the complete AI pipeline, see [AI README](../ai/README.md)

For the ESP32 firmware, see [ESP32 FIRMWARE README](../esp32/README.md)

For the overall system architecture, see [ARCHITECTURE DOCS](../docs/architecture.md)
