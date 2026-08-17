# ESP32 Firmware

This directory contains the firmware used by the ESP32 devices in the Smart Traffic Light system.

The embedded system is divided into three main components:

* A main ESP32 responsible for traffic light control and system logic.
* An AI-Thinker ESP32-CAM used as one of the traffic cameras.
* A Freenove ESP32-S3-EYE used as the second traffic camera.

Each device has a different role in the system and communicates with the other components through Wi-Fi, HTTP, and ESP-NOW.

---

## Directory Structure

```text
esp32/
|
+-- camera-1/
|   +-- cam_aithinker.ino
|
+-- camera-2/
|   +-- cam_freenove.ino
|   +-- app_httpd.cpp
|   +-- board_config.h
|   +-- camera_index.h
|   +-- camera_pins.h
|
+-- main/
    +-- main_esp.ino
```

---

## System Architecture

The ESP32 devices interact with the computer vision API and the main traffic light controller.

```text
                   +----------------------+                   +----------------------+
                   | AI-Thinker ESP32-CAM |                   | AI-Thinker ESP32-CAM |
                   |       Camera #1      |                   |       Camera #1      |
                   +----------+-----------+                   +----------+-----------+
                              |                                          |
                              | HTTP                                     | HTTP
                              | Image                                    | Image
                              v <----------------------------------------
                     +-------------------+
                     |                   |
                     |  Computer Vision  |
                     |       API         |
                     |                   |
                     |      YOLO         |
                     +---------+---------+
                               |
                               | Detection
                               | result
                               v
                     +-------------------+
                     |                   |
                     |    Main ESP32     |
                     |                   |
                     | Traffic Control   |
                     |      Logic        |
                     +---------+---------+
                               |
                               |
                               v
                        Traffic Lights
```

The exact communication flow depends on the firmware configuration.

---

## Main ESP32

The main controller is located at:

```text
main/main_esp.ino
```

Its primary responsibilities include:

* Controlling the traffic light LEDs.
* Receiving traffic information from the camera processing pipeline.
* Managing the traffic priority logic.
* Handling pedestrian input.
* Controlling the buzzer.
* Communicating with the camera devices through the configured communication protocol.
* Managing the state transitions between the different traffic signals.

The main ESP32 is responsible for making the final traffic control decisions based on the information provided by the computer vision system.

---

## Camera 1 — AI-Thinker ESP32-CAM

The first camera uses the AI-Thinker ESP32-CAM board.

Its firmware is located at:

```text
camera-1/cam_aithinker.ino
```

Its main responsibilities are:

* Capturing images from the camera sensor.
* Encoding images as JPEG.
* Sending captured images to the configured processing server.
* Receiving the detection result.
* Forwarding the relevant traffic information to the main controller.

The AI-Thinker implementation uses its specific camera configuration and should not be assumed to be interchangeable with the Freenove implementation.

---

## Camera 2 — Freenove ESP32-S3-EYE

The second camera uses the Freenove ESP32-S3-EYE.

Its firmware is located at:

```text
camera-2/cam_freenove.ino
```

The supporting camera files are:

```text
camera-2/
├── app_httpd.cpp
├── board_config.h
├── camera_index.h
└── camera_pins.h
```

The Freenove camera has a different hardware architecture from the AI-Thinker ESP32-CAM.

For this reason, its camera initialization and configuration are maintained separately rather than sharing the same camera implementation.

> Note: I'd recommend using the same camera model if you can. The use of different cameras here was due to availability of university materials.

---

# Communication

The system uses different communication mechanisms for different parts of the architecture.

## HTTP

HTTP is used to send camera images to the computer vision API.

The general flow is:

```text
ESP32-CAM
    |
    | HTTP POST
    | JPEG image
    v
Computer Vision API
    |
    | JSON response
    v
ESP32
```

The API endpoint must be configured in the camera firmware.

---

## ESP-NOW

ESP-NOW is used for direct communication between ESP32 devices where configured by the firmware.

This provides a lightweight communication mechanism without requiring a traditional HTTP connection between the devices.

The peer configuration is defined in the main ESP32 firmware.

### MAC Address Configuration

Before uploading the firmware, the MAC address configured in:

```text
main/main_esp.ino
```

must match the MAC address of the ESP32 device being used as the peer.

For a different hardware setup, this value must be updated accordingly.

Using an incorrect MAC address can prevent ESP-NOW communication from working.

---

# Configuration

The firmware contains environment-specific configuration that must be adjusted before deployment.

Depending on the setup, the following values may need to be changed:

* Wi-Fi SSID
* Wi-Fi password
* API server URL
* Local computer IP address
* API port
* Main ESP32 MAC address
* ESP-NOW channel
* GPIO assignments
* Camera configuration

## Local API URL

When the computer vision API is running on a local machine, the ESP32 must use the computer's LAN IP address.

For example:

```text
http://192.168.1.100:8000
```

The following should not be used from an ESP32:

```text
http://localhost:8000
```

`localhost` refers to the device making the request. In this case, it would refer to the ESP32 itself rather than the computer running the API.

The computer running the API and the ESP32 devices must be connected to a network that allows communication between them.

---

# Uploading the Firmware

The firmware can be uploaded using the Arduino IDE or another compatible ESP32 development environment.

Before uploading:

1. Select the correct ESP32 board.
2. Configure the appropriate serial port.
3. Update Wi-Fi credentials.
4. Update the API server URL.
5. Update the required MAC address.
6. Verify the ESP-NOW channel configuration.
7. Verify GPIO assignments.
8. Upload the firmware.

The camera firmware must be uploaded to the corresponding camera board.

The AI-Thinker and Freenove boards use different configurations and should therefore be programmed using their respective source directories.

---

# Hardware Considerations

Two different camera platforms are used in this project:

| Device                | Role            | Firmware                     |
| --------------------- | --------------- | ---------------------------- |
| AI-Thinker ESP32-CAM  | Camera #1       | `camera-1/cam_aithinker.ino` |
| Freenove ESP32-S3-EYE | Camera #2       | `camera-2/cam_freenove.ino`  |
| ESP32                 | Main controller | `main/main_esp.ino`          |

The two camera implementations are intentionally kept separate because their hardware and camera configurations differ.

---

# Traffic Light Control

The main ESP32 controls the traffic light state according to the traffic priority logic implemented in the firmware.

The system considers traffic information obtained from the computer vision pipeline rather than relying exclusively on a fixed traffic cycle.

The prototype also includes:

* Traffic light LEDs
* Pedestrian button
* Buzzer

The timing parameters are configurable in the firmware.

The values used in the prototype were adapted for the short duration of the academic demonstration and do not represent realistic traffic light timing.

---

# Troubleshooting

## ESP-NOW communication failure

If ESP-NOW communication fails, check:

* Peer MAC address.
* ESP-NOW initialization.
* Wi-Fi channel.
* Peer registration.
* Whether both devices are operating on the expected channel.

## API connection failure

If the camera cannot reach the API, check:

* Wi-Fi connection.
* Computer IP address.
* API port.
* API process status.
* Local firewall/network restrictions.
* API URL configured in the firmware.

Do not use `localhost` as the API address on the ESP32.

## Camera initialization failure

Make sure the firmware corresponds to the correct camera hardware.

The AI-Thinker ESP32-CAM and Freenove ESP32-S3-EYE use different camera configurations and should not be programmed interchangeably.

---

# Related Documentation

For information about the computer vision API, see [API README](../api/README.md)

For information about the AI model, see [AI README](../ai/README.md)

For the complete system architecture, see [Architecture README](../docs/architecture.md)