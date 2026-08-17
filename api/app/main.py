from fastapi import FastAPI, File, UploadFile, HTTPException, Form
from fastapi.responses import HTMLResponse, StreamingResponse
from ultralytics import YOLO
from PIL import Image
import io
import base64
import cv2
import numpy as np
import threading
import time
import os

app = FastAPI(
    title="Car Detection API",
    version="3.0.0"
)

model = YOLO(os.environ.get["MODEL_PATH"])

lock = threading.Lock()

traffic_state = {
    "avenue": {
        "cars": 0,
        "wait": 0,
        "weight": 0,
        "priority": False
    },
    "secondary": {
        "cars": 0,
        "wait": 0,
        "weight": 0,
        "priority": False
    },
    "lights": {
        "avenue": "GREEN",
        "secondary": "RED",
        "state": "AVENUE_GREEN"
    },
    "last_update": 0
}

images = {
    1: {
        "original": None,
        "processed": None,
        "timestamp": 0,
        "count": 0
    },
    2: {
        "original": None,
        "processed": None,
        "timestamp": 0,
        "count": 0
    }
}

history = []
MAX_HISTORY = 60

@app.get("/")
def home():
    return {
        "status": "online",
        "model": model.ckpt_path,
        "dashboard": "/dashboard"
    }

@app.post("/estado")
async def receive_state(
    avenue_cars: int = Form(...),
    avenue_wait: int = Form(...),
    avenue_weight: int = Form(...),
    secondary_cars: int = Form(...),
    secondary_wait: int = Form(...),
    secondary_weight: int = Form(...),
    light_state: str = Form(...),
    avenue_priority: bool = Form(False),
    secondary_priority: bool = Form(False)
):
    now = time.time()
    with lock:
        traffic_state["avenue"]["cars"] = avenue_cars
        traffic_state["avenue"]["wait"] = avenue_wait
        traffic_state["avenue"]["weight"] = avenue_weight
        traffic_state["avenue"]["priority"] = avenue_priority

        traffic_state["secondary"]["cars"] = secondary_cars
        traffic_state["secondary"]["wait"] = secondary_wait
        traffic_state["secondary"]["weight"] = secondary_weight
        traffic_state["secondary"]["priority"] = secondary_priority

        traffic_state["lights"]["state"] = light_state

        if light_state == "AVENUE_GREEN":
            traffic_state["lights"]["avenue"] = "GREEN"
            traffic_state["lights"]["secondary"] = "RED"
        elif light_state == "AVENUE_YELLOW":
            traffic_state["lights"]["avenue"] = "YELLOW"
            traffic_state["lights"]["secondary"] = "RED"
        elif light_state == "ROAD_GREEN":
            traffic_state["lights"]["avenue"] = "RED"
            traffic_state["lights"]["secondary"] = "GREEN"
        elif light_state == "ROAD_YELLOW":
            traffic_state["lights"]["avenue"] = "RED"
            traffic_state["lights"]["secondary"] = "YELLOW"
        elif light_state == "ALL_RED":
            traffic_state["lights"]["avenue"] = "RED"
            traffic_state["lights"]["secondary"] = "RED"
        elif light_state == "PEDESTRIAN_GREEN":
            traffic_state["lights"]["avenue"] = "RED"
            traffic_state["lights"]["secondary"] = "RED"

        traffic_state["last_update"] = now

        history.append({
            "time": now,
            "avenue": avenue_cars,
            "secondary": secondary_cars,
            "weight_avenue": avenue_weight,
            "weight_secondary": secondary_weight
        })
        if len(history) > MAX_HISTORY:
            history.pop(0)

    return {"success": True, "message": "State received"}

@app.get("/estado")
def get_state():
    with lock:
        return {
            "success": True,
            "state": traffic_state,
            "history": history
        }

@app.post("/detectar")
async def detect_cars(
    image: UploadFile = File(...),
    camera_id: int = Form(1)
):
    if not image.content_type or not image.content_type.startswith("image/"):
        raise HTTPException(
            status_code=400,
            detail="Uploaded file must be an image."
        )

    if camera_id not in [1, 2]:
        raise HTTPException(
            status_code=400,
            detail="camera_id must be 1 or 2."
        )

    try:
        content = await image.read()
        image_b64 = base64.b64encode(content).decode("utf-8")
        pil_image = Image.open(io.BytesIO(content)).convert("RGB")

        results = model.predict(
            source=pil_image,
            conf=0.6,
            imgsz=640,
            iou=0.5,
            verbose=False
        )
        result = results[0]

        frame = cv2.cvtColor(np.array(pil_image), cv2.COLOR_RGB2BGR)
        detections = []

        for box in result.boxes:
            class_id = int(box.cls[0])
            class_name = model.names[class_id]
            confidence = float(box.conf[0])
            x1, y1, x2, y2 = map(int, box.xyxy[0])

            cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
            label = f"{class_name} {confidence:.2f}"
            cv2.putText(frame, label, (x1, max(y1 - 10, 20)),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

            x1n, y1n, x2n, y2n = map(float, box.xyxyn[0])
            detections.append({
                "class": {"id": class_id, "name": class_name},
                "confidence": confidence,
                "bounding_box": {
                    "x1": float(x1), "y1": float(y1),
                    "x2": float(x2), "y2": float(y2)
                },
                "normalized_box": {
                    "x1": x1n, "y1": y1n,
                    "x2": x2n, "y2": y2n
                }
            })

        width, height = pil_image.size

        count_text = f"Cars detected: {len(detections)}"
        cv2.putText(frame, count_text, (20, 40),
                    cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 255, 255), 2)

        success, buffer = cv2.imencode(".jpg", frame)
        if not success:
            raise Exception("Failed to encode processed image.")

        processed_bytes = buffer.tobytes()
        now = time.time()

        with lock:
            images[camera_id]["original"] = content
            images[camera_id]["processed"] = processed_bytes
            images[camera_id]["timestamp"] = now
            images[camera_id]["count"] = len(detections)

        return {
            "success": True,
            "camera_id": camera_id,
            "image": {
                "name": image.filename,
                "type": image.content_type,
                "base64": image_b64,
                "width": width,
                "height": height
            },
            "model": {
                "file": "best.pt",
                "type": "YOLO",
                "classes": model.names
            },
            "config": {
                "min_confidence": 0.2,
                "iou": 0.5,
                "imgsz": 640
            },
            "result": {
                "detection_count": len(detections),
                "detections": detections
            }
        }

    except Exception as e:
        raise HTTPException(
            status_code=500,
            detail=f"Error processing image: {str(e)}"
        )

@app.get("/imagem/{camera_id}/{type}")
def get_image(camera_id: int, type: str):
    if camera_id not in [1, 2]:
        raise HTTPException(status_code=400, detail="Invalid camera.")
    if type not in ["original", "processed"]:
        raise HTTPException(status_code=400, detail="Invalid type.")

    with lock:
        img = images[camera_id][type]

    if img is None:
        raise HTTPException(status_code=404, detail="No image yet.")

    return StreamingResponse(io.BytesIO(img), media_type="image/jpeg")

@app.get("/dashboard", response_class=HTMLResponse)
def dashboard():
    return """
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Smart Traffic Light</title>
<style>
* { box-sizing: border-box; }
body { margin: 0; font-family: Arial, sans-serif; background: #111827; color: white; }
header { padding: 20px; text-align: center; background: #1f2937; }
header h1 { margin: 0; }
.container { padding: 20px; max-width: 1400px; margin: auto; }
.filas { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-bottom: 20px; }
.card { background: #1f2937; border-radius: 12px; padding: 20px; }
.card h2 { margin-top: 0; }
.metricas { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; }
.metrica { background: #374151; border-radius: 8px; padding: 15px; text-align: center; }
.metrica .valor { font-size: 30px; font-weight: bold; }
.semaforo { display: flex; gap: 15px; align-items: center; margin-top: 15px; }
.lampada { width: 30px; height: 30px; border-radius: 50%; background: #374151; }
.ligada.verde { background: #22c55e; }
.ligada.amarelo { background: #eab308; }
.ligada.vermelho { background: #ef4444; }
.imagens { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }
.camera { background: #1f2937; padding: 15px; border-radius: 12px; }
.camera h2 { margin-top: 0; }
.camera img { width: 100%; max-height: 450px; object-fit: contain; background: black; border-radius: 8px; }
.titulo-imagem { margin-top: 15px; margin-bottom: 5px; color: #9ca3af; }
.status { text-align: center; margin-top: 15px; color: #9ca3af; }
@media(max-width: 900px) {
  .filas, .imagens { grid-template-columns: 1fr; }
  .metricas { grid-template-columns: 1fr; }
}
</style>
</head>
<body>
<header>
<h1>🚦 Smart Traffic Light</h1>
<div class="status" id="status">Connecting...</div>
</header>
<div class="container">
<div class="filas">
<div class="card">
<h2>🚗 Avenue</h2>
<div class="metricas">
<div class="metrica"><div>Cars</div><div class="valor" id="avenida-carros">0</div></div>
<div class="metrica"><div>Wait</div><div class="valor" id="avenida-espera">0 s</div></div>
<div class="metrica"><div>Weight</div><div class="valor" id="avenida-peso">0</div></div>
</div>
<div class="semaforo">
<div class="lampada" id="avenida-vermelho"></div>
<div class="lampada" id="avenida-amarelo"></div>
<div class="lampada" id="avenida-verde"></div>
<strong id="avenida-estado">RED</strong>
</div>
</div>
<div class="card">
<h2>🚗 Secondary Road</h2>
<div class="metricas">
<div class="metrica"><div>Cars</div><div class="valor" id="secundaria-carros">0</div></div>
<div class="metrica"><div>Wait</div><div class="valor" id="secundaria-espera">0 s</div></div>
<div class="metrica"><div>Weight</div><div class="valor" id="secundaria-peso">0</div></div>
</div>
<div class="semaforo">
<div class="lampada" id="secundaria-vermelho"></div>
<div class="lampada" id="secundaria-amarelo"></div>
<div class="lampada" id="secundaria-verde"></div>
<strong id="secundaria-estado">RED</strong>
</div>
</div>
</div>
<div class="imagens">
<div class="camera">
<h2>📷 Camera 1 — Avenue</h2>
<div class="titulo-imagem">YOLO + OpenCV processing</div>
<img id="processada-1" src="">
<div class="titulo-imagem">Raw image</div>
<img id="original-1" src="">
</div>
<div class="camera">
<h2>📷 Camera 2 — Secondary</h2>
<div class="titulo-imagem">YOLO + OpenCV processing</div>
<img id="processada-2" src="">
<div class="titulo-imagem">Raw image</div>
<img id="original-2" src="">
</div>
</div>
</div>
<script>
function updateLight(prefix, state) {
    const red = document.getElementById(prefix + "-vermelho");
    const yellow = document.getElementById(prefix + "-amarelo");
    const green = document.getElementById(prefix + "-verde");
    const text = document.getElementById(prefix + "-estado");
    red.className = "lampada";
    yellow.className = "lampada";
    green.className = "lampada";
    text.innerText = state;
    if (state === "GREEN") {
        green.classList.add("ligada", "verde");
    } else if (state === "YELLOW") {
        yellow.classList.add("ligada", "amarelo");
    } else {
        red.classList.add("ligada", "vermelho");
    }
}
async function update() {
    try {
        const resp = await fetch("/estado");
        const data = await resp.json();
        const state = data.state;
        document.getElementById("avenida-carros").innerText = state.avenue.cars;
        document.getElementById("avenida-espera").innerText = state.avenue.wait + " s";
        document.getElementById("avenida-peso").innerText = state.avenue.weight;
        updateLight("avenida", state.lights.avenue);
        document.getElementById("secundaria-carros").innerText = state.secondary.cars;
        document.getElementById("secundaria-espera").innerText = state.secondary.wait + " s";
        document.getElementById("secundaria-peso").innerText = state.secondary.weight;
        updateLight("secundaria", state.lights.secondary);
        const ts = Date.now();
        document.getElementById("original-1").src = "/imagem/1/original?t=" + ts;
        document.getElementById("processada-1").src = "/imagem/1/processada?t=" + ts;
        document.getElementById("original-2").src = "/imagem/2/original?t=" + ts;
        document.getElementById("processada-2").src = "/imagem/2/processada?t=" + ts;
        document.getElementById("status").innerText = "System connected";
    } catch (e) {
        document.getElementById("status").innerText = "⚠️ API disconnected";
    }
}
update();
setInterval(update, 1000);
</script>
</body>
</html>
"""