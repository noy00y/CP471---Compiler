// Libraries:
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/*Constants and Global Declarations:*/ 
#define BUFFER_SIZE 2048
FILE *inputFile;
FILE *tokenFile;
FILE *errorFile;

char buffer1[BUFFER_SIZE]; // Buffers for reading file
char buffer2[BUFFER_SIZE];
char *currentBuffer; // current buffer being read

char *keywords[30]; // store keywords
int** table; // store transition table

/*Functions*/
void generateKeywords() {
    FILE *file = fopen("keywords.txt", "r");
    if (file == NULL) {
        printf("Error opening file");
        return 1;
    }
    
    // Read from file and populate keywords
    currentBuffer = buffer1;
    char* val;
    int i = 0;
    while (fgets(currentBuffer, BUFFER_SIZE, file) != NULL) {
        val = currentBuffer;
        keywords[i] = val;
        // printf("%s", keywords[i]);
        i++;

        // Swap Buffers:
        if (currentBuffer == buffer1) { currentBuffer = buffer2;}
        else {currentBuffer=buffer1;}
    }
    fclose(file);
}

void generateKeywords() {
    FILE *file = fopen("table.txt", "r");
    if (file == NULL) {
        printf("Error opening file");
        return 1;
    }    
    fclose(file);
}

// Driver Code:
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
    
    // Generate Keywords and Table:
    generateKeywords();
    generateTTable();

    return 0;
}