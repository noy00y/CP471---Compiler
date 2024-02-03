#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

int main() {
    FILE *file;
    char buffer1[BUFFER_SIZE];
    char buffer2[BUFFER_SIZE];
    char *currentBuffer;
    size_t bytesRead;

    // Open the file for reading
    file = fopen("test cases/Test9.cp", "rb");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Initialize double buffer
    currentBuffer = buffer1;

    while ((bytesRead = fread(currentBuffer, 1, BUFFER_SIZE, file)) > 0) {
        // Process data in the current buffer
        // Example: Print the content of the buffer
        fwrite(currentBuffer, 1, bytesRead, stdout);

        // Swap buffers for the next iteration
        if (currentBuffer == buffer1) {
            currentBuffer = buffer2;
        } else {
            currentBuffer = buffer1;
        }
    }

    // Close the file
    fclose(file);

    return 0;
}
