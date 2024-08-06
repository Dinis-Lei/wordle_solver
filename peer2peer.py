import socket
import pickle
import threading
import logging
import sys

class Peer2Peer(threading.Thread):

    def __init__(self, address, callback) -> None:
        threading.Thread.__init__(self)
        self.address = address
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.running = True
        self.logger = logging.getLogger("Peer2Peer")
        self.callback = callback
        self.ready = False


    def send(self, address, msg: str):
        """ Send msg to address. """
        #payload = pickle.dumps(msg)
        payload = msg.encode()
        self.socket.sendto(payload, address)

    def recv(self):
        """ Retrieve msg payload and from address."""
        try:
            payload, addr = self.socket.recvfrom(1024)
        except socket.timeout:
            return None, None

        if len(payload) == 0:
            return None, addr
        return payload, addr

    def run(self):
        print(f"Listening on {self.address}")
        self.socket.bind(self.address)
        
        if self.address[1] != 5000:
            self.send(("localhost", 5000), "ACK")
            print("Sent ACK to 5000")

        while self.running:
            data, address = self.recv()
            if data is not None:
                #output = pickle.loads(data)
                output = data.decode()
                if output == "ACK":
                    self.logger.info(f"Received: {output}")
                    print("Ready to receive messages.")
                    self.send(address, "INIT")
                    continue

                if output == "q":
                    self.running = False
                    break

                self.logger.info(f"Received: {output}")
                msg = self.callback(output)
                self.send(address, msg)

        

if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG)
    def callback(msg):
        return msg

    p2p = Peer2Peer(("localhost", int(sys.argv[1])), callback)
    p2p.start()
    p2p.join()