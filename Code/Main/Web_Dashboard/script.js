// MAP
let map, marker, locationSocket;

function initMap() {
  map = L.map("map").setView([23.797695, 90.449677], 16);
  L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
    maxZoom: 19,
    attribution: "&copy; OpenStreetMap contributors",
  }).addTo(map);
  marker = L.marker([23.797695, 90.449677])
    .addTo(map)
    .bindPopup("Live Location")
    .openPopup();
}

function connectLocationWS() {
  const url = document.getElementById("wsLocationUrl").value.trim();
  const status = document.getElementById("mapStatus");

  if (!url.startsWith("ws://") && !url.startsWith("wss://")) {
    alert("Invalid WebSocket URL.");
    status.textContent = "Invalid URL";
    status.style.color = "red";
    return;
  }

  if (locationSocket) locationSocket.close();

  locationSocket = new WebSocket(url);
  status.textContent = "Connecting...";
  status.style.color = "orange";

  locationSocket.onopen = () => {
    status.textContent = "Connected";
    status.style.color = "lime";
  };

  locationSocket.onerror = () => {
    status.textContent = "Connection error";
    status.style.color = "red";
  };

  locationSocket.onclose = () => {
    status.textContent = "Disconnected";
    status.style.color = "red";
  };

  locationSocket.onmessage = (event) => {
    const match = event.data.match(/Lat:(-?\d+\.\d+),Lng:(-?\d+\.\d+)/);
    if (match) {
      const lat = parseFloat(match[1]);
      const lng = parseFloat(match[2]);
      marker.setLatLng([lat, lng]);
      map.setView([lat, lng]);
    }
  };
}

// BEACON
let beaconSocket;

function connectBeaconSocket() {
  const url = document.getElementById("beaconWsInput").value.trim();
  const status = document.getElementById("beaconSocketStatus");

  if (!url.startsWith("ws://") && !url.startsWith("wss://")) {
    status.textContent = "Invalid URL";
    status.style.color = "red";
    return;
  }

  if (beaconSocket) beaconSocket.close();

  beaconSocket = new WebSocket(url);
  status.textContent = "Connecting...";
  status.style.color = "orange";

  beaconSocket.onopen = () => {
    status.textContent = "Connected";
    status.style.color = "lime";
  };

  beaconSocket.onerror = () => {
    status.textContent = "Connection error";
    status.style.color = "red";
  };

  beaconSocket.onclose = () => {
    status.textContent = "Disconnected";
    status.style.color = "red";
  };
}

function dropBeacon() {
  if (beaconSocket && beaconSocket.readyState === WebSocket.OPEN) {
    beaconSocket.send("drop");
    const beaconStatus = document.getElementById("beaconStatus");
    beaconStatus.textContent = "Location Beacon Deployed";
    beaconStatus.classList.remove("text-danger");
    beaconStatus.classList.add("text-success");
  } else {
    alert("Beacon socket not connected.");
  }
}

// THERMAL
const canvas = document.getElementById("thermalCanvas");
const ctx = canvas.getContext("2d");
const scaleCanvas = document.getElementById("scaleCanvas");
const scaleCtx = scaleCanvas.getContext("2d");
let thermalSocket;

function connectThermalWS() {
  const ip = document.getElementById("ipInput").value.trim();
  const port = document.getElementById("portInput").value.trim();
  const status = document.getElementById("thermalStatus");

  const wsUrl = `ws://${ip}:${port}`;
  if (thermalSocket) thermalSocket.close();

  thermalSocket = new WebSocket(wsUrl);
  status.textContent = "Connecting...";
  status.style.color = "orange";

  thermalSocket.onopen = () => {
    status.textContent = "Connected";
    status.style.color = "lime";
  };

  thermalSocket.onerror = () => {
    status.textContent = "Connection error";
    status.style.color = "red";
  };

  thermalSocket.onclose = () => {
    status.textContent = "Disconnected";
    status.style.color = "red";
  };

  thermalSocket.onmessage = (event) => {
    const data = event.data.split(",").map(parseFloat);
    if (data.length !== 768) return;

    const min = Math.min(...data);
    const max = Math.max(...data);
    const imgData = ctx.createImageData(32, 24);

    for (let i = 0; i < data.length; i++) {
      const norm = (data[i] - min) / (max - min);
      const [r, g, b] = getThermalColor(norm);
      imgData.data[i * 4] = r;
      imgData.data[i * 4 + 1] = g;
      imgData.data[i * 4 + 2] = b;
      imgData.data[i * 4 + 3] = 255;
    }

    ctx.save();
    ctx.scale(-1, 1);
    ctx.translate(-canvas.width, 0);
    ctx.putImageData(imgData, 0, 0);
    ctx.restore();
    drawColorScale(min, max);
  };
}

function getThermalColor(v) {
  v = Math.min(1, Math.max(0, v));
  let r = 0,
    g = 0,
    b = 0;
  if (v < 0.25) {
    g = Math.round(4 * v * 255);
    b = 255;
  } else if (v < 0.5) {
    g = 255;
    b = Math.round(255 - 4 * (v - 0.25) * 255);
  } else if (v < 0.75) {
    r = Math.round(4 * (v - 0.5) * 255);
    g = 255;
  } else {
    r = 255;
    g = Math.round(255 - 4 * (v - 0.75) * 255);
    b = Math.round(4 * (v - 0.75) * 255);
  }
  return [r, g, b];
}

function drawColorScale(minTemp, maxTemp) {
  const gradient = scaleCtx.createLinearGradient(0, 0, 0, scaleCanvas.height);
  for (let i = 0; i <= 100; i++) {
    const t = i / 100;
    const [r, g, b] = getThermalColor(1 - t);
    gradient.addColorStop(t, `rgb(${r},${g},${b})`);
  }

  scaleCtx.fillStyle = gradient;
  scaleCtx.fillRect(0, 0, scaleCanvas.width, scaleCanvas.height);
  document.getElementById("minTemp").textContent = `${minTemp.toFixed(1)}°C`;
  document.getElementById("maxTemp").textContent = `${maxTemp.toFixed(1)}°C`;
}

// CAMERA
const video = document.getElementById("webcam");
const cameraSelect = document.getElementById("cameraSelect");
let currentStream;

async function getCameras() {
  const devices = await navigator.mediaDevices.enumerateDevices();
  cameraSelect.innerHTML = "";
  devices.forEach((device, index) => {
    if (device.kind === "videoinput") {
      const option = document.createElement("option");
      option.value = device.deviceId;
      option.text = device.label || `Camera ${index + 1}`;
      cameraSelect.appendChild(option);
    }
  });
  if (cameraSelect.options.length > 0) {
    switchCamera();
  }
}

async function switchCamera() {
  if (currentStream) {
    currentStream.getTracks().forEach((track) => track.stop());
  }

  const deviceId = cameraSelect.value;
  try {
    const stream = await navigator.mediaDevices.getUserMedia({
      video: { deviceId },
    });
    video.srcObject = stream;
    currentStream = stream;
  } catch (err) {
    console.error("Camera error:", err);
    alert("Unable to access camera.");
  }
}

// INIT
window.onload = () => {
  initMap();
  getCameras();
};
