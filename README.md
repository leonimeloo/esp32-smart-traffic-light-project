# Smart Traffic Light with ESP32

> An adaptive traffic light system that uses computer vision, YOLO, ESP32, and real-time traffic analysis to dynamically control traffic flow at an intersection.

[![Python](https://img.shields.io/badge/Python-3.x-blue?logo=python\&logoColor=white)](https://www.python.org/)
[![ESP32](https://img.shields.io/badge/ESP32-IoT-red?logo=espressif\&logoColor=white)](https://www.espressif.com/)
[![YOLO](https://img.shields.io/badge/YOLO-Computer%20Vision-purple)](https://github.com/ultralytics/ultralytics)
[![FastAPI](https://img.shields.io/badge/FastAPI-API-009688?logo=fastapi\&logoColor=white)](https://fastapi.tiangolo.com/)
[![Roboflow](https://img.shields.io/badge/Roboflow-Dataset-6706CE)](https://universe.roboflow.com/leoni-s-workspace/esp32-cars)

---

## Overview

Smart Traffic Light is an embedded computer vision project designed to demonstrate how artificial intelligence and IoT devices can be combined to create an adaptive traffic control system.

Instead of relying on fixed traffic light timings, the system uses cameras to monitor vehicle queues at an intersection. Images captured by ESP32-CAM devices are sent to a computer vision API, where a YOLO-based model detects vehicles and estimates the traffic demand on each road.

The main ESP32 uses this information to dynamically determine which road should receive priority.

The system was developed as an academic prototype, but its architecture was designed with real-world applications in mind.

---

## Motivation

Traditional traffic lights commonly operate using predefined timing cycles. While this approach is simple and reliable, it does not adapt to changes in traffic demand.

For example:

* One road may have several vehicles waiting while another is almost empty.
* A single vehicle may have been waiting significantly longer than vehicles that recently arrived.
* Fixed timing can result in unnecessary waiting and inefficient use of the intersection.

This project explores an alternative approach:

> Use computer vision to estimate traffic demand and dynamically adapt signal priority.

The system considers both the **number of vehicles** and their **waiting time**, rather than relying exclusively on vehicle count.

---

## System Architecture

The project is divided into four main components:

```text
                    +---------------------+
                    |      ESP32-CAM      |
                    |      Camera #1      |
                    +----------+----------+
                               |
                               | Image
                               v
                    +---------------------+
                    |                     |
                    |   Computer Vision   |
                    |        API          |
                    |                     |
                    |      YOLO Model     |
                    +----------+----------+
                               |
                               | Detection results
                               v
                    +---------------------+
                    |                     |
                    |    Main ESP32       |
                    |                     |
                    | Traffic Management  |
                    |      Logic          |
                    +----------+----------+
                               |
                               v
                       +---------------+
                       | Traffic Light |
                       +---------------+


                    +---------------------+
                    |      ESP32-CAM      |
                    |      Camera #2      |
                    +----------+----------+
                               |
                               | Image
                               v
                         Computer Vision
                               API
```

The complete system consists of:

1. **ESP32-CAM cameras** — capture images of the roads.
2. **Computer Vision API** — processes images using the trained YOLO model.
3. **Main ESP32** — receives traffic information and controls the traffic lights.
4. **Traffic light hardware** — represents the physical intersection.

---

## Hardware

Two different camera platforms were used in the project:

* **AI-Thinker ESP32-CAM**
* **Freenove ESP32-S3-EYE**

The cameras perform the same general role in the system, but their camera configurations and hardware implementations are different. The corresponding firmware is therefore kept separately in the `esp32/` directory.

The system also includes a main ESP32 responsible for the traffic light control logic and communication between the different components.

---

## How It Works

### 1. Image Capture

Each ESP32-CAM continuously captures images from its corresponding road.

```text
Road -> ESP32-CAM -> JPEG image
```

The cameras send the captured images to the processing API.

### 2. Vehicle Detection

The API receives the image and runs the trained YOLO model.

The model identifies vehicles and returns information such as:

* Number of detected vehicles
* Detection confidence
* Bounding boxes
* Image dimensions
* Model configuration

Example response:

```json
{
  "success": true,
  "detections": [
    {
      "class": "car",
      "confidence": 0.91,
      "bbox": [120, 80, 250, 180]
    }
  ]
}
```

### 3. Traffic Queue Estimation

The detection results are used to estimate the traffic demand on each road.

The system does not consider only the number of vehicles.

Waiting time is also taken into account.

For example:

```text
Road A:
1 vehicle waiting for 50 seconds

Road B:
8 vehicles waiting for 10 seconds
```

Even though Road B contains more vehicles, Road A has a significantly higher waiting time.

The priority calculation therefore considers both factors.

### 4. Priority Selection

The main ESP32 determines which road should receive the green signal based on the current traffic conditions.

The system uses increasing priority levels based on waiting time.

```text
          Traffic Demand
                |
                v
       +-----------------+
       | Priority Score  |
       +--------+--------+
                |
        +-------+-------+
        v               v
     Road A           Road B
        |               |
        +-------+-------+
                |
                v
        Highest Priority
                |
                v
          Green Signal
```

### 5. Queue Drainage

The green light does not simply remain active for a completely fixed duration.

Instead, the system monitors the queue and can keep the signal open while vehicles are still being processed.

Once the queue is considered sufficiently reduced, control can return to the other road.

---

## Technology Stack

| Component          | Technology                                   |
| ------------------ | -------------------------------------------- |
| Microcontrollers   | ESP32                                        |
| Cameras            | AI-Thinker ESP32-CAM / Freenove ESP32-S3-EYE |
| Computer Vision    | YOLO                                         |
| Model Training     | Roboflow + Python                            |
| API                | FastAPI                                      |
| Image Processing   | OpenCV                                       |
| Communication      | HTTP / REST                                  |
| Containerization   | Docker                                       |
| Cloud Deployment   | Google Cloud Run                             |
| Languages          | Python / C++                                 |
| Dataset Annotation | Roboflow                                     |

---

## Project Structure

```text
smart-traffic-light/
|
+-- ai/
|   +-- notebooks/
|   +-- models/
|   +-- dataset/
|   +-- inference/
|   +-- requirements.txt
|   +-- README.md
|
+-- esp32/
|   +-- main/
|   +-- camera-1/
|   +-- camera-2/
|   +-- README.md
|
+-- api/
|   +-- app/
|   +-- models/
|   +-- requirements.txt
|   +-- Dockerfile
|   +-- README.md
|
+-- docs/
|   +-- architecture/
|   +-- images/
|   +-- report/
|
+-- README.md
+-- LICENSE
+-- .gitignore
```

### `ai/`

Contains everything related to the computer vision model, including:

* Training notebooks
* Dataset information
* Trained model files
* Inference scripts
* Evaluation results

The complete image dataset is not stored directly in this repository due to its size.

### `esp32/`

Contains the firmware for the embedded devices:

* Main ESP32
* AI-Thinker ESP32-CAM
* Freenove ESP32-S3-EYE

### `api/`

Contains the computer vision backend responsible for:

* Receiving images
* Running YOLO inference
* Processing detections
* Returning structured detection results

### `docs/`

Contains project documentation, diagrams, prototype images, and the academic report.

---

# Getting Started

## Prerequisites

Before running the project, make sure you have:

* Python 3.x
* Git
* Docker
* Arduino IDE or PlatformIO
* ESP32 board support installed
* An AI-Thinker ESP32-CAM
* A Freenove ESP32-S3-EYE
* Access to the trained YOLO model
* Access to the dataset if reproducing the training pipeline

---

## 1. Clone the Repository

```bash
git clone https://github.com/YOUR_USERNAME/smart-traffic-light.git

cd smart-traffic-light
```

---

## 2. AI Environment

Create a Python virtual environment:

```bash
python3 -m venv .venv
```

Activate it on macOS/Linux:

```bash
source .venv/bin/activate
```

On Windows:

```powershell
.venv\Scripts\activate
```

Install the AI dependencies:

```bash
pip install -r ai/requirements.txt
```

The training notebooks can be found in:

```text
ai/notebooks/
```

---

## 3. Dataset and Model

The dataset used for training was annotated and managed using Roboflow.

[![Roboflow](https://img.shields.io/badge/Roboflow-Dataset-6706CE)](https://universe.roboflow.com/leoni-s-workspace/esp32-cars)

The complete dataset is not stored in this repository due to its size.

Dataset information and the corresponding version used for training are documented in:

```text
ai/dataset/README.md
```

The trained model is documented in:

```text
ai/models/README.md
```

---

## 4. Run the Computer Vision API

Navigate to the API directory:

```bash
cd api
```

Install the dependencies:

```bash
pip install -r requirements.txt
```

Start the API locally:

```bash
uvicorn app.main:app --reload
```

The API will be available at:

```text
http://localhost:8000
```

FastAPI's interactive documentation can be accessed at:

```text
http://localhost:8000/docs
```

Make sure the trained YOLO model is available at the path expected by the API.

---

## 5. Run the API with Docker

Build the image:

```bash
docker build -t smart-traffic-light-api .
```

Run the container:

```bash
docker run -p 8000:8000 smart-traffic-light-api
```

The API can then be accessed at:

```text
http://localhost:8000
```

---

## 6. ESP32 Configuration

Before uploading the ESP32 firmware, some project-specific configuration must be changed.

The main ESP32 firmware contains configuration related to communication with the other devices. In particular, the **MAC address of the ESP32 peer must be updated** to match the hardware being used.

The camera firmware also requires the appropriate server endpoint to be configured.

Depending on the environment, the following values may need to be changed:

* Wi-Fi SSID
* Wi-Fi password
* Main ESP32 MAC address
* ESP32-CAM MAC address
* Local API URL
* Local state/server URL
* Cloud API URL, if applicable
* ESP-NOW channel configuration
* GPIO pin assignments

These values are intentionally not hard-coded in this README because they depend on the local network and hardware configuration.

### Local server URLs

When running the API locally, replace the configured endpoint with the IP address of the computer running the server.

For example:

```text
http://192.168.1.100:8000
```

Do not use:

```text
http://localhost:8000
```

from the ESP32.

`localhost` refers to the ESP32 itself, not to the computer running the API.

The computer and ESP32 devices must also be connected to a network that allows communication between them.

### ESP-NOW

If ESP-NOW is enabled, make sure the configured peer MAC address and Wi-Fi channel match the actual hardware configuration.

The relevant configuration can be found in the main ESP32 firmware under:

```text
esp32/main/
```

---

## 7. Upload the ESP32 Firmware

Open the appropriate firmware in Arduino IDE or PlatformIO.

The project contains separate firmware for the different devices:

```text
esp32/
|
+-- main/
|   +-- main.ino
|
+-- camera-1/
|   +-- camera-1.ino
|
+-- camera-2/
    +-- camera-2.ino
```

Configure the required network and device parameters before uploading.

The expected system flow is:

```text
ESP32-CAM #1
      |
      |
ESP32-CAM #2
      |
      v
Computer Vision API
      |
      v
Main ESP32
      |
      v
Traffic Light Controller
```

---

# API Deployment

The computer vision API was designed as a containerized service and can be deployed to cloud infrastructure.

The original deployment environment uses Google Cloud Run.

```text
ESP32-CAM
    |
    | HTTP POST
    v
+----------------------+
|      Cloud Run       |
|                      |
|      FastAPI         |
|         +            |
|      YOLO model      |
+----------+-----------+
           |
           | JSON
           v
      Main ESP32
```

The deployment configuration can be found in:

```text
api/Dockerfile
```

For local development, running the API directly with Uvicorn or through Docker is recommended.

---

# Project Images

The following section contains images of the physical prototype, cameras, system architecture, and other relevant components.

## Prototype

![Smart Traffic Light Prototype](docs/images/prototype.jpg)

## System Architecture

![System Architecture](docs/images/architecture.png)

## Camera Setup

![Camera Setup](docs/images/camera-setup.jpg)

---

# Demo

A complete demonstration of the system will be available below.

[Watch the Smart Traffic Light Demo](YOUR_VIDEO_LINK)

The demonstration shows:

* Vehicle detection
* Camera image acquisition
* API processing
* Traffic queue estimation
* Dynamic priority selection
* Traffic light changes
* Physical prototype operation

---

# Results

The project demonstrates the integration of:

**Computer Vision + IoT + Embedded Systems + Cloud Computing**

The trained model is capable of detecting the target vehicles in the prototype environment, while the ESP32-based controller uses the resulting traffic information to dynamically manage the intersection.

The system was designed to demonstrate the concept of adaptive traffic control rather than to represent a production-ready traffic management system.

---

# Prototype Considerations

The timing parameters used in the prototype are **not representative of a real-world traffic light system**.

They were intentionally adapted to the short presentation time available for the academic demonstration.

The timing thresholds and traffic control parameters are configurable in the ESP32 firmware and can be adjusted for longer and more realistic traffic scenarios.

---

# Future Improvements

Possible improvements include:

* [ ] Multi-class vehicle detection
* [ ] Improved vehicle tracking across frames
* [ ] More robust queue-length estimation
* [ ] Automatic calibration of traffic thresholds
* [ ] Pedestrian detection
* [ ] Pedestrian crossing integration
* [ ] Emergency vehicle prioritization
* [ ] Historical traffic analytics
* [ ] Web dashboard for live monitoring
* [ ] Improved edge inference
* [ ] Deployment of the vision model closer to the cameras
* [ ] Larger and more diverse training datasets
* [ ] Real-world camera testing

---

# Documentation

Additional documentation is available in:

```text
docs/
```

Including:

* System architecture
* Hardware documentation
* Software architecture
* AI/model documentation
* Prototype images
* Academic report

---

# Author

**Leoni Reis**

Computer Engineering / Software Engineering / Computer Science

This project was developed as an academic prototype exploring the integration of:

* Artificial Intelligence
* Computer Vision
* Embedded Systems
* IoT
* Cloud Computing
* REST APIs

---

# License

This project is licensed under the terms of the license included in this repository.

See [`LICENSE`](LICENSE) for more information.
