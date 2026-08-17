from flask import Flask, request
import os

app = Flask(__name__)

BASE_FOLDER = "dataset"

os.makedirs(BASE_FOLDER, exist_ok=True)


@app.route("/upload", methods=["POST"])
def upload():

    sequence = request.headers.get(
        "X-Sequence",
        "000"
    )

    filename = request.headers.get(
        "X-Filename",
        "foto.jpg"
    )

    filepath = os.path.join(
        BASE_FOLDER,
        filename
    )

    with open(filepath, "wb") as f:
        f.write(request.data)

    print(
        f"[OK] seq_{sequence}/{filename} "
        f"({len(request.data)} bytes)"
    )

    return "OK", 200


if __name__ == "__main__":

    print("Server started")
    print("Waiting images...")

    app.run(
        host="0.0.0.0",
        port=8080,
        debug=True
    )