from websocket_server import WebsocketServer
from socket import *
addr = ('192.168.233.135', 34952)
udpClient = socket(AF_INET, SOCK_DGRAM)

def recv_message(client, server, message):
    udpClient.sendto(str(message), addr)

server = WebsocketServer(0x6666, host='127.0.0.1')
server.set_fn_message_received(recv_message)
server.run_forever()
