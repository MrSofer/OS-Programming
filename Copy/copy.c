#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char* argv[]){

    // Checking if there are enough arguments
    if (argc < 4){
        fprintf(stderr, "usage: %s <input_file>, <output_file>, <n>\n" ,argv[0]);
        exit(1);
    }

    // Parsing buffer size
    int n;
    int scan_result = sscanf(argv[3], "%d", &n);

    if (scan_result != 1){
        fprintf(stderr, "Buffer size 'n' must be a valid integer.\n");
        exit(1);
    }


    if (n<=0){
        fprintf(stderr, "Buffer size 'n' must be positive.\n");
        exit(1);
    }

    // Opening the input file
    int input_file = open(argv[1], O_RDONLY);

    if (input_file == -1){
        perror("open (input file)");
        exit(1);
    }

    // Opening the output file

    int output_file = open(argv[2], O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR | S_IXUSR);
    
    if (output_file == -1){
        perror("open (output file)");
        close(input_file);
        exit(1);
    }

    // initiating buffer
    char* buffer = (char *)malloc(n * sizeof(char));
    
    if (buffer == NULL){
        perror("buffer");
        close(input_file);
        close(output_file);
        exit(1);
    }

    //loop
    ssize_t bytes_read, bytes_written;
    while ((bytes_read = read(input_file, buffer, n)) > 0) {

        ssize_t total_written = 0;
    
        while (total_written < bytes_read) {
        
            bytes_written = write(output_file, buffer + total_written, bytes_read - total_written);

            if (bytes_written == -1) {
                perror("write");
                close(input_file);
                close(output_file);
                free(buffer);
                exit(1);
            }

            if (bytes_written == 0) {
                fprintf(stderr, "Error: Write made no progress (returned 0 bytes).\n");
                close(input_file);
                close(output_file);
                free(buffer);
                exit(1);
            }
            
            total_written += bytes_written;

        }

    }

    if (bytes_read == -1){
        perror("read");
        close(input_file);
        close(output_file);
        free(buffer);
        exit(1);
    }

    // ending a successful run
    close(input_file);
    close(output_file);
    free(buffer);
    return 0;
}
