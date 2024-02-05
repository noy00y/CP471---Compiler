/* Imports */ 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* Constants and Global Declarations: */ 
#define MAX_KEYWORDS 100
#define BUFFER_SIZE 2048

// Lexical Analysis:
FILE *inputFile;
FILE *tokenFile;
FILE *errorFile;

char* keywords[30]; // eg. for, do, while, etc...
int table[30][127] = {0}; // transition table for automaton machine (30 states, 127 inputs)
char buffer1[BUFFER_SIZE]; // Dual buffers for reading 
char buffer2[BUFFER_SIZE];
char* currentBuffer = buffer1;

// Define Token Types --> expand upon in future iterations
typedef enum {
    TOKEN_INT,
    TOKEN_IDENTIFIER,
    TOKEN_OPERATOR,
    TOKEN_FLOAT,
    TOKEN_KEYWORD,
    TOKEN_LITERAL,
    TOKEN_EOF, // end of file
    TOKEN_ERROR // error
} TokenType;

/* Token
    - struct to represent token
    - getNextToken() - parse inputFile char by char and build tokens for lexical analysis 
 */
typedef struct {
    TokenType type;
    char buffer_val1[BUFFER_SIZE]; // dual buffer for storing large chars
    char buffer_val2[BUFFER_SIZE];
    int line; // line and char where token starts
    int character;
} Token;

// Parse file stream for chars
Token getNextToken() {
    // Declarations
    Token token; 
    int currentState = 0;
    char currentChar;
    int ascii = 0; // ascii conversion of char --> used to index transition table
    int bufferIndex = 0;

    while (1) {
        currentChar = fgetc(inputFile); // Read char by char until token is complete or EOF
        if (currentChar == EOF) {
            token.type = TOKEN_EOF;
            break;
        } 
        
        // Not EOF --> parse char
        else {
            ascii = (int)currentChar; // get ascii
            // printf("Token: %c --> ascii: %d\n", currentChar, ascii);

            /* Get current state */
            // If a-z --> set state = 10
            if (ascii >= 97 && ascii <= 122) {currentState = table[currentState][97];}

            // If ws or \n --> set state 100?, 
            if (ascii == 32 || ascii == 10) {currentState = table[currentState][ascii];}

            // If symbols: (,)
            

            /* Automaton Decisions*/
            // State 10 --> add char to buffer
            if (currentState == 10) {
                // Use buffer 2 if index surpases max size
                if (bufferIndex < BUFFER_SIZE) {
                    token.buffer_val1[bufferIndex] = currentChar;
                    printf("Adding %c to buffer 1\n", currentChar);
                }
                else {
                    token.buffer_val2[bufferIndex - BUFFER_SIZE] = currentChar;
                    printf("Adding %c to buffer2", currentChar);
                }
                bufferIndex += 1;                
            }

            // State 100 --> accept token
            else if (currentState == 100) {
                token.type = TOKEN_IDENTIFIER;
                break;
            }
        }
    }
    printf("returning token: %s\n", token.buffer_val1);
    return token;
}

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

/* Phases: */
void lexicalAnalysis() {
    // Initialize: temp token for storing, line and character for tracking position
    Token token;
    
    while(1) {
        token = getNextToken();
        if (token.type == TOKEN_EOF) {
            break;
        } 
    }
}

/* Driver Code: */
int main() {

    // Open files --> input file (to compile), tokenFile (symbol table), errorFile
    // char inputFName[100];
    // printf("Enter path of file to compile: ");
    // test cases/Test1.cp
    // scanf("%s", inputFName);
    inputFile = fopen("test cases/Test6.cp", "r");
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