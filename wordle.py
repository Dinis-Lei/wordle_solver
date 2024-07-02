from peer2peer import Peer2Peer


class Wordle:

    def __init__(self, address=("localhost", 5000)):
        self.p2p = Peer2Peer(address, callback=self.game)

        self.words = []
        with open("valid-wordle-words.txt", 'r') as file:
            for word in file:
                word = word.strip()
                self.words.append(word)
        self.n_words = len(self.words)
        self.set_word(self.words.pop(0))


    def start(self):
        self.p2p.start()
        # self.p2p.send(("localhost", 5001), "ACK")
        self.p2p.join()


    def set_word(self, word: str):
        #print(f"Word: {word}")
        self.word = word
        self.letters_in_word = set(word)
        self.letter_count = {letter: word.count(letter) for letter in self.letters_in_word}

    def game(self, guess):

        guess_count = {letter: 0 for letter in set(guess)}
        ans = ["" for _ in range(5)]

        for index, letter in enumerate(guess):
            if self.word[index] == letter:
                ans[index] = "o"
                guess_count[letter] += 1

        for index, letter in enumerate(guess):
            guess_count[letter] += 1
            if self.word[index] != letter:
                if letter in self.letters_in_word:
                    if guess_count[letter] > self.letter_count[letter]:
                        ans[index] = "x"
                    else:
                        ans[index] = "-"
                else:
                    ans[index] = "x"
            else:
                guess_count[letter] -= 1

        ans = str.join("", ans)
        #print(ans)

        if ans == "ooooo":
            if (self.n_words - len(self.words)) % 100 == 0:
                print(f"{len(self.words)} words left")

            if len(self.words) == 0:
                self.p2p.running = False
                return "q"
            self.set_word(self.words.pop(0))

        return ans


if __name__ == "__main__":
    wordle = Wordle()
    wordle.start()