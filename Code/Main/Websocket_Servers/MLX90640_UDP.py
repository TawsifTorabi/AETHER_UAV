import socket
import numpy as np

UDP_IP = "0.0.0.0"
UDP_PORT = 8765

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

# Storage for thermal frame
frame = np.zeros((24, 32), dtype=np.float32)
received_rows = set()

print(f"Listening on UDP port {UDP_PORT}...")

while True:
    data, addr = sock.recvfrom(2048)  # buffer size
    try:
        text = data.decode().strip()
        if not text.startswith("R"):
            continue

        # Example line: R12:36.1,35.9,... (32 values)
        row_index_str, values_str = text.split(":")
        row_index = int(row_index_str[1:])
        temps = [float(v) for v in values_str.split(",")]

        if len(temps) == 32 and 0 <= row_index < 24:
            frame[row_index, :] = temps
            received_rows.add(row_index)

        if len(received_rows) == 24:
            print(f"\n✅ Complete frame received from {addr}")
            print(frame)
            received_rows.clear()

            # Optional: visualize
            import matplotlib.pyplot as plt
            plt.imshow(frame, cmap='inferno')
            plt.colorbar()
            plt.pause(0.01)
            plt.clf()

    except Exception as e:
        print(f"Error: {e}")
