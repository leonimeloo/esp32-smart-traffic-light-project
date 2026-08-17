# Trained Models

This directory contains the trained YOLO models developed during the Smart Traffic Light project.

Two model versions are currently included:

```text
models/
|
+-- training_v1.pt
+-- best.pt
+-- README.md
```

## Model Versions

| Model            | Description                                              | Status           |
| ---------------- | -------------------------------------------------------- | ---------------- |
| `training_v1.pt` | Initial experimental model                               | Previous version |
| `best.pt`        | Revised model trained with a more representative dataset | Current version  |

---

## `training_v1.pt`

`training_v1.pt` was the first experimental model developed during the project.

The purpose of this version was primarily to validate the complete training and inference pipeline.

The dataset used for this model had several limitations:

* Relatively small number of images.
* Limited representation of the actual prototype environment.
* A significant amount of images containing toy cars from outside the final project.
* Fewer images captured using the actual ESP32 cameras.
* Fewer images containing the specific toy cars used in the prototype.

As a result, this model was useful as a baseline but was not selected as the final model for the system.

---

## `best.pt`

`best.pt` is the latest model developed for the project and is the preferred model for inference.

The training dataset was revised to better represent the actual deployment environment.

The revised dataset included:

* More images from the physical prototype.
* Images captured using the ESP32 cameras.
* The specific toy cars used in the project.
* More representative camera perspectives.
* More varied positions and arrangements of the vehicles.

This resulted in a model that was better aligned with the conditions in which the system is expected to operate.

### Generalization

Although `best.pt` was specifically trained for the project's controlled environment and the toy cars used in the prototype, additional testing produced an interesting result.

The model also performed well when identifying toy cars that were not part of the original training set.

This suggests that the model learned visual characteristics associated with the target class rather than relying exclusively on memorized examples of the specific training vehicles.

This behavior was observed during independent testing and should not be interpreted as a guarantee of performance in substantially different environments.

---

## Intended Use

The models are intended for:

* Vehicle detection in the Smart Traffic Light prototype.
* Testing computer vision inference.
* Integration with the project's processing API.
* Evaluation of object detection performance under controlled conditions.

They are not intended to replace a production-grade traffic monitoring system without additional training, validation, and testing. 

---

## Model Selection

The latest model was selected based on its performance under the conditions most relevant to the final prototype.

The selection process prioritized:

1. Detection performance on images from the actual environment.
2. Detection of the specific vehicles used in the prototype.
3. Robustness to different vehicle positions.
4. Performance on images captured directly by the ESP32 cameras.
5. Generalization to previously unseen toy cars.

---

## Using a Model

The models can be tested independently using: [../inference.py](../inference.py)

For example:

```bash
python ai/inference.py --image path/to/image.jpg
```

The model used by the inference script can be configured according to the model being evaluated.

For the final system, `best.pt` should be used unless another model is explicitly being evaluated.

---

## Training

The models were trained using the workflow documented in: [../notebooks/Model-Training.ipynb](../notebooks/Model-Training.ipynb)

The training dataset and annotation information are documented in: [../dataset/README.md](../dataset/README.md)

---

## Model Configuration

The inference configuration used by the project includes parameters such as:

```text
Confidence threshold: 0.50
IoU threshold:        0.50
Image size:           640
Task:                 Object Detection
Target class:         car
```

These values may be adjusted depending on the evaluation scenario.

---

## Notes

The model files are included in this repository for reproducibility and demonstration purposes.

When deploying the API, make sure the model path configured in the application points to the intended model version.

For the current implementation, `best.pt` is the recommended model.
