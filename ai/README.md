# Artificial Intelligence

This directory contains the computer vision component of the Smart Traffic Light project.

The AI pipeline is responsible for training, evaluating, and running inference with a YOLO-based object detection model designed to identify cars in images captured by the ESP32 cameras.

## Pipeline

The complete workflow can be summarized as:

```text
Dataset
   |
   v
Annotation
   |
   v
Dataset Version
   |
   v
YOLO Training
   |
   v
Model Evaluation
   |
   v
Trained Model
   |
   v
Inference
   |
   v
Traffic Detection API
```

## Directory Structure

```text
ai/
|
+-- dataset/
|   +-- README.md
|
+-- models/
|   +-- best.pt
|   +-- training_v1.pt
|   +-- README.md
|
+-- notebooks/
|   +-- Model-Training.ipynb
|
+-- inference.py
|
+-- README.md
```

### `dataset/`

Contains documentation about the dataset used during model development.

The image dataset itself is not stored in this repository due to its size. Dataset information, annotation details, and access to the hosted dataset are documented separately.

See [`dataset/README.md`](dataset/README.md).

### `models/`

Contains the trained YOLO models used during the development of the project.

Two versions are currently included:

* `training_v1.pt` — initial experimental model.
* `best.pt` — latest and currently preferred model.

See [`models/README.md`](models/README.md) for details about each version.

### `notebooks/`

Contains the model training notebook.

`Model-Training.ipynb` documents the training workflow, including dataset preparation, model configuration, training, and evaluation.

### `inference.py`

Standalone inference script used to test the trained model independently from the API.

This allows images to be processed directly using a trained model without requiring the complete ESP32 or API infrastructure.

Example:

```bash
python inference.py --image path/to/image.jpg
```

## Model

The project uses a YOLO-based object detection model trained specifically for the prototype environment.

The latest model was trained using images that better represent the actual deployment scenario, including:

* Images captured using the ESP32 cameras.
* Images of the specific toy cars used in the prototype.
* Images representing the actual road/intersection environment.

Although the model was primarily developed for this controlled environment, testing showed good performance when detecting toy cars that were not included in the original training set.

This suggests that the model learned relevant visual characteristics of the target class rather than simply memorizing the specific training examples.

## Reproducing the Training

The training process is documented in:

```text
notebooks/Model-Training.ipynb
```

The general process is:

1. Collect images from the target environment.
2. Annotate the vehicles.
3. Generate a dataset version.
4. Train the YOLO model.
5. Evaluate the resulting model.
6. Select the best-performing model.
7. Test the model using independent images.
8. Integrate the model with the inference API.

## Dataset

The dataset was managed and annotated using Roboflow.

The dataset itself is intentionally kept outside the Git repository.

See [`dataset/README.md`](dataset/README.md) for additional information.

## Requirements

The model training and inference environment uses Python and the libraries specified by the project's AI dependencies.

Typical dependencies include:

```text
ultralytics
opencv-python
roboflow
```

Install the required dependencies according to the project environment before running the notebook or inference script.
