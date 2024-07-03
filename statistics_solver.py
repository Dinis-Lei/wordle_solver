from solver import SolverBase
import sys
import numpy as np

class StatsSolver(SolverBase):
    def __init__(self, address, stat="info") -> None:
        super().__init__(address, self.solve)
        self.stat=stat


    def solve(self, msg):
        msg = super().solve(msg)
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
            
        self.guess = self.df.sort_values(self.stat, ascending=False).iloc[0]["word"]
        self.tries += 1
        return self.guess


if __name__ == "__main__":
    stat = "info"
    if len(sys.argv) > 1:
        stat = sys.argv[1]

    solver = StatsSolver(("localhost", 5001), stat=stat)
    solver.start()