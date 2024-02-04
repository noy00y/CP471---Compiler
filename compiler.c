/* Imports */ 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* Constants and Global Declarations: */ 
#define MAX_KEYWORDS 100
#define BUFFER_SIZE 2048

// Define Token Types --> expand upon in future iterations
typedef enum {
    TOKEN_INT,
    TOKEN_IDENTIFIER,
    TOKEN_OPERATOR,
    TOKEN_FLOAT,
    TOKEN_KEYWORD,
    TOKEN_LITERAL,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

// Define struct to represent token
typedef struct {
    TokenType type;
    char value[256];
    int line;
    int character;
} Token;

// Lexical Analysis:
FILE *inputFile;
FILE *tokenFile;
FILE *errorFile;

char* keywords[30]; // eg. for, do, while, etc...
int table[20][100] = {0}; // transition table for automaton machine
char buffer1[BUFFER_SIZE]; // Dual buffers for reading 
char buffer2[BUFFER_SIZE];
char* currentBuffer = buffer1;

/* Functions: */
void generateTable() {
    FILE *file = fopen("table.txt", "r"); // Open the file for reading
    if (file == NULL) {
        printf("Error opening file.\n");
        return NULL;
    }

    int state, input, next_state;
    while (fscanf(file, "%d, %d, %d\n", &state, &input, &next_state) == 3) {
        table[state][input] = next_state; // Assign next_state to index
    }
    fclose(file);
}

char** generateKeywords() {
    FILE *file = fopen("keywords.txt", "r");
    if (file == NULL) {
        printf("Error opening file");
        return NULL;
    }

    char* val;
    int i = 0;
    while (fgets(currentBuffer, BUFFER_SIZE, file) != NULL && i < MAX_KEYWORDS) {
        val = malloc(BUFFER_SIZE * sizeof(char)); // Allocate memory for each keyword
        if (val == NULL) {
            printf("Memory allocation failed\n");
            return NULL;
        }
        strcpy(val, currentBuffer); // Copy keyword into allocated memory
        keywords[i] = val;
        i++;

        // Swap Buffers:
        if (currentBuffer == buffer1) {
            currentBuffer = buffer2;
        }
        else {
            currentBuffer = buffer1;
        }
    }
    fclose(file);
    return keywords;
}

// Phases:
void lexicalAnalysis() {
    
}

int main() {

    // Open files --> input file (to compile), tokenFile (symbol table), errorFile
    char inputFName[100];
    printf("Enter path of file to compile: ");
    // test cases/Test1.cp
    scanf("%s", inputFName);
    inputFile = fopen(inputFName, "r");

    tokenFile = fopen("tokens.txt", "w");
    errorFile = fopen("errors.txt", "w");

    if (inputFile == NULL || tokenFile == NULL || errorFile == NULL) {
        printf("Error opening files\n");
        return NULL;
    }

    // Generate keywords and transition table
    char **keywordsArray = generateKeywords();
    generateTable();

    lexicalAnalysis(); // run parsing

    return 0;
}