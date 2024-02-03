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
int** table; // store transition table
char buffer1[BUFFER_SIZE];
char buffer2[BUFFER_SIZE];
char* currentBuffer = buffer1;

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
    scanf("%s", inputFName);
    inputFile = fopen(inputFName, "r");

    tokenFile = fopen("tokens.txt", "w");
    errorFile = fopen("errors.txt", "w");

    if (inputFile == NULL || tokenFile == NULL || errorFile == NULL) {
        printf("Error opening files\n");
        return 1;
    }

    char **keywordsArray = generateKeywords();

    return 0;
}