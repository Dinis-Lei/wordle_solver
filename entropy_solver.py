import itertools
from solver import SolverBase
import pandas as pd
from math import log2
import time
import sys

info_comb = list(itertools.product(['o', '-', 'x'], repeat=5))

class EntropySolver(SolverBase):

    def __init__(self, address, start=0, length=10, id=0) -> None:
        #super().__init__(address, self.solve)
        self.df = pd.read_csv('word_stats.csv')

        self.df : pd.DataFrame = self.df[["word", "l0", "l1", "l2", "l3", "l4"]]
        self.df["letters"] = self.df["word"].apply(lambda x: set(x))
        self.df["entropy"] = 0.0

        self.begining = start
        self.length = length
        self.id = id

    def load_data(self):
        pass

    def initialize(self):
        self.df = pd.read_csv('word_stats.csv')
        self.df : pd.DataFrame = self.df[["word", "l0", "l1", "l2", "l3", "l4"]]
        self.df["letters"] = self.df["word"].apply(lambda x: set(x))
        self.df["entropy"] = 0.0

        self.construct_entropy()
        self.df.to_csv(f"entropy{self.id}.csv", index=False)

    def get_nmatches(self, word, pattern):
        # Vectorized condition checks
        o_conditions = [self.df[f'l{i}'] == word[i] for i in range(5) if pattern[i] == 'o']
        x_conditions = [self.df['letters'].apply(lambda x: word[i] not in x) for i in range(5) if pattern[i] == 'x']
        other_conditions = [(self.df['letters'].apply(lambda x: word[i] in x)) & (self.df[f'l{i}'] != word[i]) for i in range(5) if pattern[i] not in ['o', 'x']]

        # Combine conditions using & operator
        query = True  # Start with True for a neutral starting condition
        for condition in o_conditions + x_conditions + other_conditions:
            query &= condition

        return self.df.loc[query].shape[0]


    def construct_entropy(self):
        self.df["entropy"] = 0.0
        for i, comb in enumerate(info_comb):
            if i < self.begining:
                continue
            if i >= self.begining + self.length:
                break

            print(f"Combination {i} out of {len(info_comb)}")
            print(comb)
            c = 0
            for index, row in self.df.iterrows():
                word = row["word"]
                tic = time.time()
                nmatches = self.get_nmatches(word, comb)
                toc = time.time()
                #print(f"Row n {c}, Time taken: {toc-tic}", end="\r")
                prob = nmatches / len(self.df)
                self.df.loc[index, "entropy"] += prob * log2(1/prob) if prob != 0 else 0
                c += 1

            



    def solve(self):
        pass

    def __str__(self) -> str:
        return self.df.sort_values("entropy", ascending=False).head()



if __name__ == "__main__":
    solver = EntropySolver(("localhost", 5001), int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]))
    solver.initialize()
    #print(solver)