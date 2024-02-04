#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/*Constants and Global Declarations:*/ 
#define MAX_KEYWORDS 100
#define BUFFER_SIZE 2048
FILE *inputFile;
FILE *tokenFile;
FILE *errorFile;

char* keywords[30]; // Define keywords globally
int table[20][100] = {0}; // initalize transition table w/ 0
char buffer1[BUFFER_SIZE];
char buffer2[BUFFER_SIZE];
char* currentBuffer = buffer1;

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

    // generate keywords and transition table
    char **keywordsArray = generateKeywords();
    generateTable();

    // 

    return 0;
}