letter_statistics = [
    dict(), dict(), dict(), dict(), dict(),
]


with open("valid-wordle-words.txt", 'r') as file:
    count = 0
    for word in file:
        count += 1
        word = word.strip()
        for index, letter in enumerate(word):
            if letter not in letter_statistics[index]:
                letter_statistics[index][letter] = 0
            letter_statistics[index][letter] += 1
        
    letter_statistics = [
        {k: v/count for k, v in stat.items()}
        for stat in letter_statistics
    ]
with open("valid-wordle-words.txt", 'r') as file:    
    with open("word_stats.csv", 'w') as out:
        out.write("word,l0,l1,l2,l3,l4,p0,p1,p2,p3,p4,sum,prod,info\n")
        for word in file:
            word = word.strip()
            out.write(f"{word},")
            for letter in word:
                out.write(letter + ",")
            lst = []
            prod = 1
            for index, letter in enumerate(word):
                lst.append(letter_statistics[index][letter])
                prod *= letter_statistics[index][letter]
                out.write(f"{letter_statistics[index][letter]},")
            out.write(f"{sum(lst)},")
            out.write(f"{prod},")
            i = 1
            letter_appeared = set()
            for index, letter in enumerate(word):
                if letter in letter_appeared:
                    letter_info = letter_statistics[index][letter]
                else:
                    letter_info = sum([letter_statistics[i][letter] for i in range(5)])
                i *= letter_info
                letter_appeared.add(letter)
            out.write(f"{i}")
            out.write("\n")