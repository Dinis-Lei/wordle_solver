#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>  
#include <libgen.h>

#define PATTERN_LENGTH 5
#define N_PATTERNS 243
#define N_THREADS 4
#define PORT 5000


struct Word {
    char word[PATTERN_LENGTH+1];
    bool valid;
    double entropy;
    double frequency;
    double probability;
};

struct Weights {
    double w1;
    double w2;
    double w3;
};

/**
 * @brief Connect to the server for batch word processing
 * @return 
 */
int network_connect();

/**
 * @brief Interactive mode
 * @return 
 */
int interactive_mode();

/**
 * @brief Launch the worker threads
 * @return 
 */
int launch_workers();

/**
 * @brief Entropy generate for each word using multiple threads
 * @param par 
 * @return 
 */
void *worker(void *par);

/**
 * @brief Count the number of occurrences of a character in a word
 * @param word Word to evaluate
 * @param c Character to count
 * @return count of the character
 */
int count_char_occurrences(char *word, char c);

/**
 * @brief Validate the words based on the pattern and the guess
 * @param pattern Current pattern
 * @param guess Current guess
 */
void filter_words(char *pattern, char *guess);

/**
 * @brief Generate the entropy for each word
 */
void generate_entropy();

/**
 * @brief Heuristic nº 1 based on the sum of the word's entropy and probability
 * @param word Word to calculate the heuristic
 * @param w_index Index of current weight
 * @return 
 */
double heuristic1(struct Word word, int w_index);

/**
 * @brief Heuristic nº 2 based on the word probability and expected score of the word
 * @param word Word to calculate the heuristic
 * @param w_index 
 * @return Value of the heuristic
 */
double heuristic2(struct Word word, int w_index);

/**
 * @brief Read and store the word matrix from a file
 * @param filename Name of the file
 * @return Success or failure
 */
int read_word_matrix(char *filename);

/**
 * @brief Read and store the words from a file
 * @param filename Name of the file
 * @return Success or failure
 */
int read_word_file(char *filename);

/**
 * @brief Read and store the weights from a file
 * @param filename Name of the file
 * @return Success or failure
 */
int read_weights(char *filename);

/**
 * @brief Read and store the word frequency from a file
 * @param filename Name of the file
 * @return Success or failure
 */
int read_word_frequency(char *filename);

/**
 * @brief Read and store the word probability from a file
 * @param filename Name of the file
 * @return Success or failure
 */
int read_word_probability(char *filename);

/**
 * @brief Print the top N words sorted by the heuristic
 * @param n Number of words to print
 * @param w_index Index of current weight
 */
void print_top_n_words(int n, int w_index);

/**
 * @brief Get the delta time object 
 * @return current time
 */
static double get_delta_time(void);

/**
 * @brief Print usage information
 * @param name 
 */
static void printUsage(char *name);

int **word_matrix;
int n_valid_words;
struct Word *words;
int n_words;
int *status_threads;
int n_tries;
char *outfile = NULL;
double (*heuristic)(struct Word word, int w_index);

struct Weights *weights;
bool has_weights = false;


int main(int argc, char *argv[]) {

    int opt;
    bool iflg = false;
    bool bflg = false;

    heuristic = &heuristic1;

    while ((opt = getopt(argc, argv, "hib:w:23")) != -1) {
        switch (opt) {
            case 'h':
                printUsage(basename(argv[0]));
                return EXIT_SUCCESS;
            case 'i':
                iflg = true;
                break;
            case 'b':
                bflg = true;
                outfile = optarg;
                break;
            case 'w':
                if (read_weights(optarg) != 0) {
                    printf("Error reading file\n");
                    return EXIT_FAILURE;
                }
                has_weights = true;
                break;
            case '2':
                heuristic = &heuristic2;
                break;
            case '?': /* invalid option */
                fprintf (stderr, "%s: invalid option\n", basename(argv[0]));
                exit(EXIT_FAILURE);
        }
    }


    if (!has_weights) {
        weights = (struct Weights *)malloc(sizeof(struct Weights));
        weights[0].w1 = 1.0;
        weights[0].w2 = 0.0;
        weights[0].w3 = 0.0;
    }

    if (bflg) {
        return network_connect();
    }

    else if (iflg) {
        return interactive_mode();
    }

    return EXIT_SUCCESS;
}

int interactive_mode() {
    (void) get_delta_time();
    if (read_word_matrix("word_matrix.csv") != 0) {
        printf("Error reading word_matrix.csv file\n");
        return EXIT_FAILURE;
    }
    printf("Time to read word_matrix.csv file: %f\n", get_delta_time());

    (void) get_delta_time();
    if (read_word_file("valid_words.txt") != 0) {
        printf("Error reading file\n");
        return EXIT_FAILURE;
    }
    printf("Time to read file: %f\n", get_delta_time());

    (void) get_delta_time();
    if (read_word_frequency("word_freq.txt") != 0) {
        printf("Error reading file\n");
        return EXIT_FAILURE;
    }
    printf("Time to read file: %f\n", get_delta_time());

    (void) get_delta_time();
    if (read_word_probability("word_prob.txt") != 0) {
        printf("Error reading file\n");
        return EXIT_FAILURE;
    }
    printf("Time to read file: %f\n", get_delta_time());

    (void) get_delta_time();
    //generate_entropy();
    launch_workers();
    printf("Time to generate entropy: %f\n", get_delta_time());

    double max_entropy = -1.0;
    int max_entropy_index = 0;
    
    char guess[5];
    strcpy(guess, words[max_entropy_index].word);
    n_tries = 0;
    int weight_idx = 0;

    char input[6] = "INIT";

    while (true) {

        if (strcmp(input, "q") == 0) {
            break;
        }
        else if (strcmp(input, "INIT") == 0) {
            printf("INIT\n");
        }
        else if (strcmp(input, "ooooo") == 0){
            // Reset
            n_valid_words = n_words;
            printf("Number of guesses made: %d\n\n\n", n_tries);
            n_tries = 0;
            for (int i = 0; i < n_words; i++) {
                words[i].valid = true;
            }
            (void) get_delta_time();
            //generate_entropy();
            launch_workers();
            printf("Time to generate entropy: %f\n", get_delta_time());
        }
        else {
            // Filter words that don't match the input
            filter_words(input, guess);
            printf("Nº Valid words: %d\n", n_valid_words); 

            if (n_valid_words == 1) {
                for (int i = 0; i < n_words; i++) {
                    if (words[i].valid) {
                        printf("Guess: %s, %f\n", words[i].word, words[i].entropy);
                        strcpy(guess, words[i].word);
                        strcpy(input, "ooooo");
                        break;
                    }
                }
                continue;
            }

            
            // Generate entropy for the remaining words
            (void) get_delta_time();
            //generate_entropy();
            launch_workers();
            printf("Time to generate entropy: %f\n", get_delta_time());
        }

        struct Word max_word;
        max_word.entropy = -1;
        max_word.valid = false;
        max_word.frequency = -1;
        max_entropy = -1;
        max_entropy_index = 0;
        double max_frequency = -1;
        bool is_valid = false;
        double entropy_limit = -log2(1.0 / n_valid_words);
        weight_idx = n_tries-1;
        if (weight_idx > sizeof(weights)/sizeof(weights[0])) {
            weight_idx = sizeof(weights)/sizeof(weights[0]);
        }

        // Find the word with the highest entropy
        for (int i = 0; i < n_words; i++) {
            if ((*heuristic)(words[i], weight_idx) > (*heuristic)(max_word, weight_idx)) {
                max_word = words[i];
                max_entropy_index = i;
            }  
        }

        printf("Nº bits of information: %lf\n", -log2(1.0 / n_valid_words));
        printf("Guess: %s, E: %lf, H:%lf\n", 
                    words[max_entropy_index].word, 
                    words[max_entropy_index].entropy, 
                    (*heuristic)(words[max_entropy_index], weight_idx));
        strcpy(guess, words[max_entropy_index].word);

        // print_top_n_words(5, weight_idx);
        // if (n_valid_words < 11) {
        //     printf("--- Valid words ---\n");
        //     for (int i = 0; i < n_words; i++) {
        //         if (words[i].valid) {
        //             printf("Word: %s, E: %lf, P: %lf, H: %lf\n", 
        //                 words[i].word, words[i].entropy,
        //                 words[i].probability, 
        //                 (*heuristic)(words[i], weight_idx));
        //         }
        //     }
        //     printf("-------------------\n");
        // }

        n_tries++;
        printf("N tries: %d\n", n_tries);

        // read console input
        printf("Enter the pattern: ");
        scanf("%s", input);
    }


    return EXIT_SUCCESS;
}

int network_connect() {
    int sockfd;
    int status;
    struct sockaddr_in address;
    char buffer[256];
    int n;
    int opt = 1;

    printf("Starting server\n");

    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "Error opening socket\n");
        return EXIT_FAILURE;
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address\n");
        return EXIT_FAILURE;
    }

    if ((status = connect(sockfd, (struct sockaddr *)&address, sizeof(address))) < 0) {
        fprintf(stderr, "Error connecting\n");
        return EXIT_FAILURE;
    }

    send(sockfd, "ACK", 3, 0);
    printf("ACK sent to %d\n", sockfd);

    n_tries = 0;
    char guess[6];
    char pattern[6] = "";
    FILE *file = fopen(outfile, "w");

    if (file == NULL) {
        printf("Error opening file %s\n", outfile);
        return EXIT_FAILURE;
    }

    (void) get_delta_time();
    if (read_word_matrix("word_matrix.csv") != 0) {
        printf("Error reading file\n");
        return EXIT_FAILURE;
    }
    printf("Time to read file: %f\n", get_delta_time());

    (void) get_delta_time();
    if (read_word_file("valid_words.txt") != 0) {
        printf("Error reading valid_words.txt\n");
        return EXIT_FAILURE;
    }
    printf("Time to read valid_words.txt file: %f\n", get_delta_time());

    (void) get_delta_time();
    if (read_word_frequency("word_freq.txt") != 0) {
        printf("Error reading word_freq.txt file\n");
        return EXIT_FAILURE;
    }
    printf("Time to read word_freq.txt file: %f\n", get_delta_time());

    (void) get_delta_time();
    if (read_word_probability("word_prob.txt") != 0) {
        printf("Error reading word_prob.txt file\n");
        return EXIT_FAILURE;
    }
    printf("Time to read word_prob.txt file: %f\n", get_delta_time());


    (void) get_delta_time();
    launch_workers();
    printf("Time to generate entropy: %f\n", get_delta_time());

    char *guesses[10];
    double *uncertainty = (double *)malloc(10 * sizeof(double));
    for (int i = 0; i < 10; i++) {
        guesses[i] = (char *)malloc(6 * sizeof(char));
    }


    while (true) {
        if (n_tries > 10) {
            // print_top_n_words(10);
            for (int i = 0; i < n_tries; i++) {
                printf("%s %lf,", guesses[i], uncertainty[i]);
            }
            printf("\n");
            break;
        }
        n = read(sockfd, pattern, 6);
        // printf("Pattern received: %s\n", pattern);

        if (strcmp(pattern, "INIT") != 0) {
            if (strcmp(pattern, "ooooo") == 0) {
                // RESET
                // printf("Reset\n");
                fprintf(file, "%d,", n_tries);
                for (int i = 0; i < n_tries-1; i++) {
                    fprintf(file, "%s, %lf,", guesses[i], uncertainty[i]);
                }
                fprintf(file, "%s, %lf\n", guesses[n_tries-1], uncertainty[n_tries-1]);
                n_valid_words = n_words;
                for (int i = 0; i < n_words; i++) {
                    words[i].valid = true;
                }
                (void) get_delta_time();
                launch_workers();
                // printf("Time to generate entropy: %f\n", get_delta_time());
                n_tries = 0;
            }
            else if (pattern[0] == 'q') {
                fprintf(file, "%d,", n_tries);
                for (int i = 0; i < n_tries-1; i++) {
                    fprintf(file, "%s, %lf,", guesses[i], uncertainty[i]);
                }
                fprintf(file, "%s, %lf\n", guesses[n_tries-1], uncertainty[n_tries-1]);
                printf("END\n");
                break;
            }
            else {
                int n_words_before = n_valid_words;
                filter_words(pattern, guess);
                // printf("Nº Valid words: %d\n", n_valid_words);
                // printf("Nº of words filtered: %d\n", n_words_before - n_valid_words);
                (void) get_delta_time();
                launch_workers();
                // printf("Time to generate entropy: %f\n", get_delta_time());
            }
        }

        if (n_valid_words == 1) {
            for (int i = 0; i < n_words; i++) {
                if (words[i].valid) {
                    // printf("FINAL Guess: %s\n", words[i].word);
                    strcpy(guess, words[i].word);
                    break;
                }
            }
        }
        else {
            int max_entropy_idx = 0;
            struct Word max_word;
            max_word.entropy = -1;
            max_word.frequency = -1;
            max_word.valid = false;


            int weight_idx = n_tries-1;
            if (weight_idx > sizeof(weights)/sizeof(weights[0])) {
                weight_idx = sizeof(weights)/sizeof(weights[0]);
            }

            for (int i = 0; i < n_words; i++) {
                if ((*heuristic)(words[i], weight_idx) > (*heuristic)(max_word, weight_idx)) {
                    max_word = words[i];
                    max_entropy_idx = i;
                }
                
            }
            // printf("Nº Valid words: %d\n", n_valid_words);
            // printf("Guess: %s, %f %d\n", words[max_entropy_idx].word, words[max_entropy_idx].entropy, words[max_entropy_idx].valid);
            strcpy(guess, words[max_entropy_idx].word);
        }
        
        send(sockfd, guess, sizeof(guess), 0);
        strcpy(guesses[n_tries], guess);
        uncertainty[n_tries] = -log2(1.0 / n_valid_words);
        n_tries++;
        // printf("Guess sent to server: %s, %d\n", guess, n_tries);

    }

    fclose(file);
    close(sockfd);
    return 0;

}

int count_char_occurrences(char *word, char c) {
    int count = 0;
    int j;
    for (j = 0; j < PATTERN_LENGTH; j++) {
        if (word[j] == c) {
            count++;
        }
    }
    return count;
}

void filter_words(char *pattern, char *guess) {

    int i;
    int p;

    int letter_counts[26];
    int correct_letters[26];
    int vallidate_pattern[26]; 
    
    for (i = 0; i < 26; i++) {
        letter_counts[i] = 0;
        correct_letters[i] = 0;
        vallidate_pattern[i] = 0;
    } 

    for (p = PATTERN_LENGTH-1; p >= 0; p--) {
        if (pattern[p] == '-') {
            vallidate_pattern[guess[p] - 'a'] = 1;
        }
        else if (pattern[p] == 'x' && vallidate_pattern[guess[p] - 'a'] == 1) {
            // Invalid Pattern
            n_valid_words = 0;
            printf("Invalid Pattern\n");
            return;
        }
    }

    for (i = 0; i < PATTERN_LENGTH ; i++) {
        letter_counts[guess[i] - 'a']++;
        if (pattern[i] == 'o' || pattern[i] == '-') {
            correct_letters[guess[i] - 'a']++;
        }
    }


    for (i = 0; i < n_words; i++) {
        words[i].entropy = 0.0;

        if (!words[i].valid) {
            continue;
        }
        for (p = 0; p < PATTERN_LENGTH; p++) {
            if (pattern[p] == 'o' && guess[p] != words[i].word[p]) {
                words[i].valid = false;
                n_valid_words--;
                break;
            }
            else if (pattern[p] == '-') {
                if (guess[p] == words[i].word[p]) {
                    words[i].valid = false;
                    n_valid_words--;
                    break;
                }
                else if (count_char_occurrences(words[i].word, guess[p]) < correct_letters[guess[p] - 'a']) {
                    words[i].valid = false;
                    n_valid_words--;
                    break;
                }
            }
            else if (pattern[p] == 'x') {
                if (correct_letters[guess[p] - 'a'] > 0) {
                    if (words[i].word[p] == guess[p]) {
                        words[i].valid = false;
                        n_valid_words--;
                        break;
                    }
                    else if (correct_letters[guess[p] - 'a'] != count_char_occurrences(words[i].word, guess[p])) {
                        words[i].valid = false;
                        n_valid_words--;
                        break;
                    }
                }
                else if (strchr(words[i].word, guess[p]) != NULL) {
                    words[i].valid = false;
                    n_valid_words--;
                    break;
                }
            }
        }
    }
}

int launch_workers() {
    pthread_t *threads;
    unsigned int *thread_id;
    
    if ((threads = malloc(N_THREADS * sizeof(pthread_t))) == NULL 
        || (thread_id = malloc(N_THREADS * sizeof(unsigned int))) == NULL
        || (status_threads = malloc(N_THREADS * sizeof(int))) == NULL) {
        fprintf(stderr, "Error allocating memory for threads\n");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < N_THREADS; i++)
        thread_id[i] = i;

    for (int i = 0; i < N_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, worker, &thread_id[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < N_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Error joining thread %d\n", i);
            return EXIT_FAILURE;
        }
    }

    free(threads);
    free(thread_id);
    free(status_threads);

    return EXIT_SUCCESS;
}

void *worker(void *par) {
    unsigned int id = *(unsigned int *)par;
    int length = ceil((double)n_words / N_THREADS);

    int start = id * length;
    int end = (id + 1) * length;

    if (end > n_words) {
        end = n_words;
    }
    
    // printf("Starting thread %d, start: %d, end: %d\n", id, start, end);
    double *pattern_count = (double *)malloc(N_PATTERNS * sizeof(double));

    for (int i = start; i < end; i++) {
        for (int p = 0; p < N_PATTERNS; p++) {
            pattern_count[p] = 0.0;
        }
        double entropy = 0.0;

        //printf("Thread %d, word %d\n", id, i);
        for (int j = 0; j < n_words; j++) {
            if (!words[j].valid) {
                continue;
            }
            pattern_count[word_matrix[i][j]] += words[j].probability;
        }

        for (int p = 0; p < N_PATTERNS; p++) {
            if (pattern_count[p] == 0) {
                continue;
            }
            double p_i = (double)pattern_count[p] / (double)n_valid_words;
            entropy += p_i * -log2(p_i);
        }

        // Round to 6 decimal places
        double tmp = (int)(entropy * 1000000 + 0.5);
        words[i].entropy = (double) tmp / 1000000;
    }
    free(pattern_count);
}

void generate_entropy() {

    for (int i = 0; i < n_words; i++) {
        int *pattern_count = (int *)calloc(N_PATTERNS, sizeof(int));
        double entropy = 0.0;

        for (int j = 0; j < n_words; j++) {
            if (!words[j].valid) {
                continue;
            }
            pattern_count[word_matrix[i][j]]++;
        }

        for (int p = 0; p < N_PATTERNS; p++) {
            if (pattern_count[p] == 0) {
                continue;
            }
            double p_i = (double)pattern_count[p] / (double)n_valid_words;
            entropy += p_i * -log2(p_i);
        }

        words[i].entropy = entropy;
    }

}

double heuristic1(struct Word word, int w_index) {
    if (n_valid_words == 1) {
        return 0.0;
    }
    
    return weights[w_index].w1 * word.entropy / -log2(1.0 / n_valid_words); + 
            weights[w_index].w2 * word.frequency + 
            weights[w_index].w3 * word.valid;
}

double expected_score(double x) {
    // a * log2(x + 1) + b
    // 0.74532867 0.82761198
    // a**-x + b*(1 - c**-x)
    // 0.93110814 1.48816043 1.50511912
    // a**-x + b*(c - d**-x)
    // 0.93514211 1.42977659 1.13184927 1.36789824
    double a = 0.93514211;
    double b = 1.42977659;
    double c = 1.13184927;
    double d = 1.36789824;
    return pow(a, -x) + b * (c - pow(d, -x));
}

double heuristic2(struct Word word, int w_index) {
    double prob = word.valid * (1.0 / n_valid_words) * word.probability;
    
    return -(prob * (n_tries + 1) + 
            (1 - prob) * ((n_tries + 1) + 
                expected_score(-log2(1.0 / n_valid_words) - word.entropy)));
}

int read_word_file(char *filename) {
    printf("Opening file %s\n", filename);
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file %s\n", filename);
        return 1;
    }
    printf("File opened\n");
    int size = 0;

    if (fscanf(file, "%d", &size) != 1) {
        printf("Failed to read the size from the file\n");
        fclose(file);
        return 1;
    }
    printf("Size: %d\n", size);
    words = (struct Word *)malloc(size * sizeof(struct Word));
    n_words = size;
    n_valid_words = size;
    // Consumes the newline
    int c;
    while ((c = fgetc(file)) != '\n' && c != EOF);

    char line[7];
    int i = 0;
    while (i < size && fgets(line, sizeof(line), file)) {
        // printf("Line %d:%d %s", i, sizeof(line), line);
        // printf("---\n");
        // printf("Line %d: %s", i, line);
        for (int j = 0; j < 5; j++) {
            words[i].word[j] = line[j];
        }
        words[i].word[PATTERN_LENGTH] = '\0';
        // printf("Line %d: %s, %s\n", i, words[i].word, line);
        words[i].entropy = 0.0;
        words[i].valid = true;
        // printf("Line %d: %s, %f, %d\n", i, words[i].word, words[i].entropy, words[i].valid);
        i++;
    }

    fclose(file);
    return 0;
}

int read_word_matrix(char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return EXIT_FAILURE;
    }
    int file_size = 0;
    int c = fscanf(file, "%d", &file_size);
    n_valid_words = file_size;

    // Remove newline
    c = fgetc(file);

    word_matrix = (int **)malloc(file_size * sizeof(int *));
    for (int i = 0; i < file_size; i++) {
        word_matrix[i] = (int *)malloc(file_size * sizeof(int));
    }

    for (int i = 0; i < file_size; i++) {
        for (int j = 0; j < file_size; j++) {
            c = fscanf(file, "%d", &word_matrix[j][i]);
            // Remove the comma
            fgetc(file);
        }
        // Remove the newline
        fgetc(file);
    }

    
    fclose(file);
    return EXIT_SUCCESS;
}

int read_weights(char *filename) {
    FILE *file = fopen(filename, "r");

    int file_size = 0;
    int c = fscanf(file, "%d", &file_size);

    // Remove newline
    c = fgetc(file);

    weights = (struct Weights *)malloc(file_size * sizeof(struct Weights));

    for (int i = 0; i < file_size; i++) {
        c = fscanf(file, "%lf %lf %lf", &weights[i].w1, &weights[i].w2, &weights[i].w3);
        // Remove the newline
        fgetc(file);
    }

    fclose(file);
    return EXIT_SUCCESS;

}

int read_word_frequency(char *filename) {
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        return EXIT_FAILURE;
    }

    int size = 0;
    int c = fscanf(file, "%d", &size);

    if (c != 1) {
        return EXIT_FAILURE;
    }

    // Remove newline
    c = fgetc(file);

    double freq;
    for (int i = 0; i < size; i++) {
        c = fscanf(file, "%lf", &freq);
        
        if (c != 1) {
            return EXIT_FAILURE;
        }

        words[i].frequency = freq;

        // Remove newline
        c = fgetc(file);
    }

    fclose(file);
    return EXIT_SUCCESS;
}

int read_word_probability(char *filename) {
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        return EXIT_FAILURE;
    }

    int size = 0;
    int c = fscanf(file, "%d", &size);

    if (c != 1) {
        return EXIT_FAILURE;
    }

    // Remove newline
    c = fgetc(file);

    double prob;
    for (int i = 0; i < size; i++) {
        c = fscanf(file, "%lf", &prob);
        
        if (c != 1) {
            return EXIT_FAILURE;
        }

        words[i].probability = prob;

        // Remove newline
        c = fgetc(file);
    }

    fclose(file);
    return EXIT_SUCCESS;
}

void print_top_n_words(int n, int w_index) {
    struct Word *sorted_words = (struct Word *)malloc(n_words * sizeof(struct Word));
    for (int i = 0; i < n_words; i++) {
        sorted_words[i] = words[i];
    }

    // Bubble sort the words by entropy
    for (int i = 0; i < n_words; i++) {
        for (int j = 0; j < n_words - i - 1; j++) {
            if ((*heuristic)(sorted_words[j], w_index) < (*heuristic)(sorted_words[j+1], w_index)) { 
                struct Word temp = sorted_words[j];
                sorted_words[j] = sorted_words[j+1];
                sorted_words[j+1] = temp;
            }
        }
    }
    
    for (int i = 0; i < n; i++) {
        printf("W: %s, E: %lf, F: %lf, P: %lf, V: %d, H: %lf\n", 
            sorted_words[i].word, sorted_words[i].entropy, 
            sorted_words[i].frequency, 
            sorted_words[i].probability,
            sorted_words[i].valid,
            (*heuristic)(sorted_words[i], w_index)
        );
        
    }
}


static double get_delta_time(void) {
    static struct timespec t0, t1;

    t0 = t1;
    if(clock_gettime(CLOCK_MONOTONIC, &t1) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    return (double) (t1.tv_sec - t0.tv_sec) + 1.0e-9 * (double) (t1.tv_nsec - t0.tv_nsec);
}


static void printUsage (char *cmdName) {
    fprintf(stderr, "\nSynopsis: %s [OPTIONS] ...\n"
           "  OPTIONS:\n"
           "  -h            --- Print this help screen\n"
           "  -i            --- Iteractive mode\n"
           "  -b [FNAME]    --- Batch mode\n"
           "  -1            --- Heuristic 1\n"
           "  -2            --- Heuristic 2 (best)\n", cmdName);
}