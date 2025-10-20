#!/usr/bin/env python3
import cv2
from ultralytics import YOLO
import torch
from flask import Flask, Response, render_template_string

device = "mps" if torch.backends.mps.is_available() else "cpu"
model = YOLO("yolov8n.pt")

cap = cv2.VideoCapture(0, cv2.CAP_AVFOUNDATION)

app = Flask(__name__)

def gen_frames():
    while True:
        success, frame = cap.read()
        if not success:
            break

        results = model.predict(frame, imgsz=640, conf=0.4, classes=[0],
                                device=device, verbose=False)
        annotated_frame = results[0].plot()

        ret, buffer = cv2.imencode('.jpg', annotated_frame)
        frame = buffer.tobytes()
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')

@app.route('/video_feed')
def video_feed():
    return Response(gen_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/')
def index():
    # quick test page
    return render_template_string('''
        <html><body style="background:black;">
        <h3 style="color:white;text-align:center;">YOLOv8 Human Detection Feed</h3>
        <div style="display:flex;justify-content:center;">
            <img src="/video_feed" width="80%">
        </div></body></html>
    ''')

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5050)
