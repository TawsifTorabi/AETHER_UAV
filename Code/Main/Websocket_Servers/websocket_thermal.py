import asyncio
import websockets
import socket

#In windows systems, The firewall might block the Websocket Server from LAN.

PORT = 8765

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

connected_clients = set()

async def handler(websocket):
    client_ip = websocket.remote_address[0]
    print(f"[+] Client connected: {client_ip}")
    connected_clients.add(websocket)
    try:
        async for message in websocket:
            # Broadcast received message to all other clients
            for client in connected_clients:
                if client != websocket:
                    try:
                        await client.send(message)
                    except Exception:
                        pass  # Ignore errors from disconnected clients
    except websockets.exceptions.ConnectionClosedOK:
        print(f"[-] Client {client_ip} disconnected normally.")
    except Exception as e:
        print(f"[!] Error with {client_ip}: {e}")
    finally:
        connected_clients.remove(websocket)
        print(f"[-] Client disconnected: {client_ip}")

async def main():
    local_ip = get_local_ip()
    print("Starting WebSocket server...")
    print(f"→ ws://localhost:{PORT}")
    print(f"→ ws://{local_ip}:{PORT} (LAN)")
    print("Waiting for connections...\n")

    async with websockets.serve(handler, "0.0.0.0", PORT):
        await asyncio.Future()  # run forever

if __name__ == "__main__":
    asyncio.run(main())
