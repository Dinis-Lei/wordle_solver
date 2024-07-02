import pandas as pd
from peer2peer import Peer2Peer

class Solver:

    def __init__(self, address) -> None:
        self.p2p = Peer2Peer(address, self.solve)

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
            

        if msg:
            guess_positions = {letter: [i for i, x in enumerate(self.guess) if x == letter] for letter in set(self.guess)}
            for letter, pos in guess_positions.items():
                for p in pos:
                    if msg[p] == 'o':
                        self.df = self.df[self.df[f'l{p}'] == letter]
                    elif msg[p] == '-':
                        self.df = self.df[(self.df[f'l{p}'] != self.guess[p])]
                        self.df = self.df[self.df["word"].str.contains(self.guess[p])]
                    else:
                        if len(pos) > 1:
                            flg = False
                            for i in pos:
                                if msg[i] == 'o' or msg[i] == '-':
                                    flg = True
                                    break
                            if flg:
                                self.df = self.df[(self.df[f'l{p}'] != self.guess[p])]
                            else:
                                self.df = self.df[self.df["word"].str.contains(self.guess[p]) == False]
                        else:
                            self.df = self.df[self.df["word"].str.contains(self.guess[p]) == False]
            
        self.guess = self.df.sort_values('prod', ascending=False).iloc[0]["word"]
        self.tries += 1
        return self.guess


if __name__ == "__main__":
    solver = Solver(("localhost", 5001))
    solver.start()