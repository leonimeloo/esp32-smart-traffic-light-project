# Dataset

This directory contains documentation about the dataset used to train the vehicle detection model.

The image dataset is not stored directly in this repository due to its size. The dataset is hosted and managed through Roboflow.

## Dataset Source

[![Roboflow](https://img.shields.io/badge/Roboflow-Dataset-6706CE)](https://universe.roboflow.com/leoni-s-workspace/esp32-cars)

The dataset can be accessed through the Roboflow project:

**Roboflow:**
https://universe.roboflow.com/leoni-s-workspace/esp32-cars

## Dataset Purpose

The dataset was created specifically for the vehicle detection component of the Smart Traffic Light system.

The objective was to train a model capable of detecting the toy cars used in the physical prototype from images captured by the ESP32 cameras.

Because the final system operates in a controlled environment, particular attention was given to collecting images that represent the actual deployment conditions.

These include:

* The physical road/intersection used by the prototype.
* The actual toy cars used in the system.
* Different positions and orientations of the cars.
* Different numbers of cars in the scene.
* Images captured directly from the ESP32 cameras.
* Variations in the position of vehicles within the camera's field of view.

## Dataset Development

The dataset evolved during the development of the project.

An initial training dataset contained a relatively small number of images and included a significant number of images featuring toy cars that were different from the vehicles used in the final prototype.

This resulted in the first experimental model being less representative of the actual deployment environment.

The dataset was subsequently improved by incorporating more images from the actual prototype, including images captured using the ESP32 cameras and images containing the specific toy cars used in the project.

This revised dataset was used to train the latest model, [`best.pt`](../models/best.pt).

## Dataset Versions

The project contains two relevant stages of model development:

### Initial Dataset

The initial dataset:

* Contained fewer images.
* Included a larger proportion of external toy-car images.
* Was less representative of the final prototype.
* Was primarily used to validate the initial training pipeline.

This dataset produced the [`training_v1.pt`](../models/training_v1.pt) model.

### Revised Dataset

The revised dataset:

* Contains more images from the actual prototype environment.
* Includes the specific toy cars used in the project.
* Includes images captured using the ESP32 cameras.
* Better represents the conditions expected during inference.

This dataset was used to produce the latest [`best.pt`](../models/best.pt) model.

## Annotation

Vehicle annotations were created using Roboflow and reviewed manually.

The target object class used by the model is:

```text
car
```

Bounding-box annotations were used for object detection.

## Dataset Split

The dataset is divided into training, validation, and test subsets according to the dataset version used during training.

The exact split and dataset version are documented in the training notebook:

```text
../notebooks/Model-Training.ipynb
```

## Reproducing the Dataset

To reproduce the training process:

1. Access the Roboflow dataset.
2. Use the appropriate dataset version.
3. Export the dataset in the format expected by the YOLO training pipeline.
4. Follow the training notebook.
5. Evaluate the resulting model using images that were not used during training.

## Important Note

The dataset was intentionally optimized for the prototype environment rather than for a general-purpose vehicle detection application.

However, during testing, the latest model showed good performance when detecting toy cars that were not part of the training set.

This behavior was observed during independent tests and suggests a degree of generalization beyond the exact vehicles represented in the training images.

## Dataset Capture

The repository also includes the tools used to capture a new dataset using an ESP32-CAM.

These tools are useful if you want to **reproduce the dataset collection process**, collect additional images, or create a custom dataset for your own environment.

The capture system consists of:

* An ESP32-CAM responsible for capturing the images
* A web interface hosted by the ESP32-CAM for controlling the capture
* A Flask server running on a computer
* HTTP communication between the ESP32-CAM and the computer
* Automatic storage of the captured JPEG images

The complete capture system is available in [ai/dataset/dataset_capture/](./dataset_capture/)

For detailed instructions on configuring the ESP32-CAM, starting the Flask server, and collecting images, see the [Dataset Capture README](./dataset_capture/README.md).

### Capture Workflow

```text
ESP32-CAM
    │
    │ Capture image
    ▼
Web Interface
    │
    │ HTTP POST
    ▼
Laptop / Flask Server
    │
    ▼
Raw Dataset
```

The capture tools are independent from the model training pipeline. After collecting the images, the resulting dataset can be labeled and prepared for training using the workflow described in this directory.

> **Note:** The dataset capture firmware is intended for image collection and is not the firmware used in the final smart traffic-light prototype.
