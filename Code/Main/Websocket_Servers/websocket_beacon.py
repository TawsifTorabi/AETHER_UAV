import asyncio
import websockets
import socket
import serial
import threading

# Configuration
PORT = 8766
SERIAL_PORT = 'COM45'       # Change to your correct port
SERIAL_BAUDRATE = 115200

connected_clients = set()

def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
    except Exception:
        ip = "127.0.0.1"
    finally:
        s.close()
    return ip

async def handler(websocket):
    client_ip = websocket.remote_address[0]
    print(f"[+] Client connected: {client_ip}")
    connected_clients.add(websocket)
    try:
        async for message in websocket:
            for client in connected_clients:
                if client != websocket:
                    try:
                        await client.send(message)
                    except Exception:
                        pass
    except websockets.exceptions.ConnectionClosed:
        print(f"[-] Client {client_ip} disconnected.")
    finally:
        connected_clients.remove(websocket)

async def broadcast(message):
    to_remove = set()
    for client in connected_clients:
        try:
            await client.send(message)
        except:
            to_remove.add(client)
    connected_clients.difference_update(to_remove)

def serial_reader_thread(loop):
    try:
        ser = serial.Serial(SERIAL_PORT, SERIAL_BAUDRATE, timeout=1)
        print(f"[✓] Serial port {SERIAL_PORT} opened at {SERIAL_BAUDRATE} baud.")
        while True:
            if ser.in_waiting:
                line = ser.readline().decode(errors='ignore').strip()
                if line:
                    print(f"[Serial] {line}")
                    asyncio.run_coroutine_threadsafe(broadcast(line), loop)
    except serial.SerialException as e:
        print(f"[!] Serial error: {e}")

async def main():
    local_ip = get_local_ip()
    print("Starting WebSocket server for...")
    print(f"→ ws://localhost:{PORT}")
    print(f"→ ws://{local_ip}:{PORT} (LAN)")
    print("Waiting for connections...\n")

    loop = asyncio.get_running_loop()
    threading.Thread(target=serial_reader_thread, args=(loop,), daemon=True).start()

    async with websockets.serve(handler, "0.0.0.0", PORT):
        await asyncio.Future()  # run forever

if __name__ == "__main__":
    asyncio.run(main())
