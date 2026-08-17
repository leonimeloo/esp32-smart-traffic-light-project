# System Architecture

This document describes the architecture of the Smart Traffic Light system and how its hardware, computer vision, communication, and control components interact.

## Overview

The system combines embedded systems, computer vision, machine learning, wireless communication, and automated decision-making to create an adaptive traffic light prototype.

The system is divided into four main layers:

1. Image acquisition
2. Computer vision processing
3. Traffic control logic
4. Physical signaling

The complete architecture can be represented as:

```text
                        IMAGE ACQUISITION
                              |
             +----------------+----------------+
             |                                 |
             v                                 v
   +--------------------+            +----------------------+
   | AI-Thinker         |            | Freenove             |
   | ESP32-CAM          |            | ESP32-S3-EYE         |
   | Camera 1           |            | Camera 2             |
   +---------+----------+            +----------+-----------+
             |                                  |
             | HTTP                             | HTTP
             | JPEG image                       | JPEG image
             |                                  |
             +----------------+-----------------+
                              |
                              v
                   +------------------------+
                   |                        |
                   |   Computer Vision API  |
                   |                        |
                   |       FastAPI          |
                   |          +             |
                   |      YOLO Model        |
                   |                        |
                   +-----------+------------+
                               |
                               | Detection result
                               v
                    +----------------------+
                    |                      |
                    |      Main ESP32      |
                    |                      |
                    | Traffic Control Logic|
                    |                      |
                    +----------+-----------+
                               |
                +--------------+--------------+
                |              |              |
                v              v              v
           Traffic LEDs   Pedestrian LEDs   Buzzer
```

---

## 1. Image Acquisition

Two different camera platforms are used to monitor the two roads of the prototype:

- AI-Thinker ESP32-CAM
- Freenove ESP32-S3-EYE

The cameras are physically positioned to observe the traffic lanes represented in the prototype.

Each camera captures images of its respective road and sends the images to the computer vision processing layer.

The two camera implementations are maintained separately because the boards have different hardware and camera configurations.

---

## 2. Computer Vision Layer

The computer vision layer is implemented as a Python API using FastAPI.

The API receives images from the cameras and runs the trained YOLO object detection model.

The processing pipeline is:

```text
JPEG Image
    |
    v
FastAPI
    |
    v
YOLO Inference
    |
    v
Object Detection
    |
    v
Vehicle Count
    |
    v
JSON Response
```

The model is trained to identify the target vehicle class used in the prototype.

The latest model was trained using images that better represent the actual prototype environment, including images captured by the ESP32 cameras and images of the specific toy cars used in the project.

---

## 3. Communication

The system uses different communication mechanisms for different purposes.

### HTTP

HTTP is used for communication between the camera devices and the computer vision API.

The camera sends a captured image to the API:

```text
ESP32-CAM
    |
    | HTTP POST
    v
/detect
    |
    v
YOLO
```

The API then returns the detection information.

### ESP-NOW

ESP-NOW is used for wireless communication between ESP32 devices where configured by the firmware.

This provides a lightweight communication mechanism without requiring a traditional HTTP connection between the devices.

The peer configuration is defined in the main ESP32 firmware.

---

## 4. Main Controller

The main ESP32 acts as the central controller of the physical prototype.

Its responsibilities include:

- Receiving vehicle detection information.
- Maintaining the waiting time of each traffic queue.
- Calculating queue priority.
- Selecting which road receives the green signal.
- Controlling traffic light LEDs.
- Handling pedestrian requests.
- Activating the pedestrian buzzer.
- Transitioning between traffic signal states.

The main controller does not perform YOLO inference.

The computationally intensive computer vision processing is performed by the API.

This separation allows the ESP32 to focus on real-time control while the API handles machine learning inference.

---

## 5. Adaptive Traffic Control

The traffic light does not rely exclusively on a fixed cycle.

The controller calculates a priority value for each queue using both:

- Number of detected vehicles.
- Waiting time.

The prototype uses:

```text
P = 5 × N + T
```

Where:

- `P` = queue priority
- `N` = number of detected vehicles
- `T` = waiting time in seconds

This allows waiting time to influence the decision even when another road contains more vehicles.

For example:

```text
Road A:
1 vehicle
50 seconds waiting

P = 5 × 1 + 50
P = 55


Road B:
8 vehicles
10 seconds waiting

P = 5 × 8 + 10
P = 50
```

Road A therefore receives priority.

---

## 6. Traffic Light Operation

The green signal does not operate exclusively according to a fixed duration.

The active road remains open while its queue is still considered significant.

When the queue becomes sufficiently small, the system evaluates the current traffic conditions again.

A simplified decision flow is:

```text
Current Road is Green
        |
        v
Is the queue still significant?
        |
     +--+--+
     |     |
    YES    NO
     |     |
     |     v
     |  Yellow
     |     |
     |     v
     | Recalculate
     |   priority
     |     |
     +-----+
           |
           v
     Select next road
```

The implementation may consider the road sufficiently drained when the camera detects no vehicles or only an isolated vehicle while the other road has active demand.

---

## 7. Pedestrian System

The prototype includes:

- Pedestrian button.
- Pedestrian red LED.
- Pedestrian green LED.
- Buzzer.

When the pedestrian button is pressed, the system waits for the configured transition period before safely ending the current vehicle cycle.

The active vehicle signal changes to yellow, followed by a vehicle stop state.

The pedestrian green LED is then activated and the buzzer remains active during the crossing period.

After the crossing:

```text
Pedestrian Green OFF
        |
        v
Pedestrian Red ON
        |
        v
Recalculate Queue Priority
        |
        v
Resume Adaptive Control
```

---

## 8. High-Level Data Flow

The complete data flow can be summarized as:

```text
Camera
   |
   | Image
   v
Computer Vision API
   |
   | Vehicle Detection
   v
Traffic Information
   |
   | Wireless Communication
   v
Main ESP32
   |
   | Priority Calculation
   v
Traffic Light State
   |
   +---------> Vehicle LEDs
   |
   +---------> Pedestrian LEDs
   |
   +---------> Buzzer
```

---

## 9. Design Principle

The architecture separates the system into specialized components:

```text
ESP32 Cameras
    -> Image Acquisition

API
    -> Computer Vision

Main ESP32
    -> Decision Making

LEDs / Buzzer
    -> Physical Signaling
```

This separation makes each component easier to test, replace, and improve independently.

For example, the YOLO model can be updated without changing the traffic light control logic, while the traffic control algorithm can be modified without changing the camera capture implementation.

---

## 10. Current Limitations

The final architecture was intentionally designed around the available hardware and the project's academic scope.

Known limitations include:

- Camera image quality.
- Limited frame rate.
- Dependence on camera positioning.
- Dependence on lighting conditions.
- Potential false positives outside the controlled environment.
- Limited processing capabilities of the embedded devices.
- Simplified intersection geometry.

These limitations provide clear directions for future iterations.

---