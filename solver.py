import pandas as pd
from peer2peer import Peer2Peer

class SolverBase:

    def __init__(self, address, callback) -> None:
        
        self.p2p = Peer2Peer(address, callback)

        self.df = pd.read_csv('word_stats.csv')
        self.guess = ""
        self.tries = 0
        with open("stats.txt", 'w') as file:
            file.write("")

    def reset(self):
        self.df = pd.read_csv('word_stats.csv')
        self.tries = 0

    def start(self):
        self.p2p.start()
        # self.p2p.send(("localhost", 5000), "ACK")
        self.p2p.join()


    def solve(self, msg):
        if msg == "ooooo":
            with open("stats.txt", 'a') as file:
                file.write(f"{self.tries}\n")
            self.reset()
            msg = ""
        return msg


if __name__ == "__main__":
    solver = SolverBase(("localhost", 5001))
    solver.start()