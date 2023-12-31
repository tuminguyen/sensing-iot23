import socket
import time

def receive_udp_packets():
    # Create a UDP socket
    sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)

    # Bind the socket to the port
    sock.bind(('', 8888))

    while True:
        # Receive data from a client
        data, address = sock.recvfrom(1024)

        # Print the received data
        print('Received from {}: {}'.format(address, data.decode()))

        # Wait for a short period before checking for new data
        time.sleep(0.1)

if __name__ == '__main__':
    # Start receiving UDP packets
    receive_udp_packets()