import os

from flask import Flask, request, jsonify

app = Flask(__name__)


@app.route("/", methods=["GET"])
def feed():
    return jsonify({"msg": "hello world"})


@app.route("/image", methods=["POST"])
def upload_file():
    try:
        # change directory as preferred
        directory = "./images"
        os.makedirs(directory, exist_ok=True)

        if 'image' not in request.files:
            return jsonify({"message": "No file part"}), 400

        image = request.files['image']

        if image.filename == '':
            return jsonify({"message": "No selected file"}), 400

        # Save the file to a specified location
        file_path = os.path.join(directory, image.filename)
        image.save(file_path)

        return jsonify({"message": "File uploaded successfully"}), 200

    except Exception as e:
        return jsonify({"message": f"Could not upload file. Error: {str(e)}"}), 500


if __name__ == "__main__":
    app.run(debug=True)
