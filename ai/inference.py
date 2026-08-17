import argparse
import cv2
from ultralytics import YOLO


def detect_carts(image_path, model="./models/best.pt", output="result.jpg", confidence=0.5, classes=None):
    """
    Perform object detection on an image and save the annotated result.

    Args:
        image_path (str): Path to the input image.
        model (str): YOLO model file name.
        output (str): Path to save the annotated output image.
        confidence (float): Detection confidence threshold.
        classes (list[int]): List of class IDs to filter (optional).

    Returns:
        int: Number of detected objects.
    """
    model_instance = YOLO(model)

    # Run inference
    results = model_instance.predict(source=image_path, conf=confidence, classes=classes, imgsz=640, iou=0.5)
    result = results[0]

    # Annotate and save the image
    annotated_image = result.plot()
    cv2.imwrite(output, annotated_image)

    # Count detections
    count = len(result.boxes)
    print(f"Detected objects: {count}")
    print(f"Annotated image saved to: {output}")

    # Print details for each detection
    for box in result.boxes:
        class_name = model_instance.names[int(box.cls[0])]
        conf_value = float(box.conf[0])
        print(f"  - {class_name} (confidence: {conf_value:.2f})")

    return count


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="YOLO object detection script")
    parser.add_argument("--image", required=True, help="Path to the input image")
    parser.add_argument("--model", default="best.pt", help="YOLO model file (default: best.pt)")
    parser.add_argument("--output", default="result.jpg", help="Output image path (default: result.jpg)")
    parser.add_argument("--conf", type=float, default=0.5, help="Confidence threshold (default: 0.5)")

    args = parser.parse_args()

    detect_carts(
        image_path=args.image,
        model=args.model,
        output=args.output,
        confidence=args.conf,
        classes=args.classes
    )