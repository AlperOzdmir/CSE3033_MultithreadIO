#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_FILE_NAME_LENGTH 32
#define MAX_LINE_LENGTH 256

//Previous declarations of used functions
void check_arguments(int argc, char *argv[]);
void start_multi_threading(int argc, char *argv[]);
void get_arguments(int argc, char* argv[]);
int get_line_count();
int** calculate_thread_index_info(int read_thread_count);
void* read_thread_func(void* arg);
void* upper_thread_func(void* arg);
void* replace_thread_func(void* arg);
void* write_thread_func(void* arg);
void line_to_upper_case(int index, int thread_index);
void replace_chars(int line_index, int thread_index);
void write_line(int line_index, int thread_index);

//Declare global variables in order to access them via threads
int* thread_counts;
char** lines;
char* file_name;
int line_count;
char** modification_status;
pthread_mutex_t read_mutex;
pthread_mutex_t modify_mutex;
pthread_mutex_t write_mutex;

//Calls required functions in startup
int main(int argc, char* argv[]) {
    //If command line arguments are not as expected program will not continue further
    check_arguments(argc, argv);
    start_multi_threading(argc, argv);
}

//Checks if command line arguments are valid, if not calls exit function
void check_arguments(int argc, char *argv[]) {
    int i;
    if(argc != 8) {
        printf("Error: Incorrect number of arguments");
        exit(1);
    }
    if(strcmp(argv[1], "-d") != 0) {
        printf("Error: Incorrect argument. Expected -d");
        exit(1);
    }
    if(strcmp(argv[3], "-n") != 0) {
        printf("Error: Incorrect argument. Expected -n");
        exit(1);
    }
    for(i = 4; i < 8; i++) {
        int count = atoi(argv[i]);
        if (count <= 0) {
            printf("Error: Thread count must be greater than 0");
            exit(1);
        }
    }
}

//Initiliazing threads, joining them, creating their neccessary varialbes, mutexes etc. are done here
//Calls many functions in itself
void start_multi_threading(int argc, char *argv[]) {
    int i;
    thread_counts = malloc(4 * sizeof(int));
    file_name = malloc(MAX_FILE_NAME_LENGTH * sizeof(char));
    //Get command line arguments and store them to use later on
    get_arguments(argc, argv);
    //Get how many lines in the file given as command line argument, if the file does not exist program do not continue
    line_count = get_line_count();
    lines = malloc(line_count * sizeof(char*));
    for(i = 0; i < line_count; i++) {
        lines[i] = malloc(MAX_LINE_LENGTH * sizeof(char));
    }
    //Create an array in order to keep track of the threads process, (used to synchronize threads)
    modification_status = malloc(line_count * sizeof(char*));
    for(i = 0; i < line_count; i++) {
        modification_status[i] = malloc(4 * sizeof(char));
        modification_status[i][0] = '0';
        modification_status[i][1] = '0';
        modification_status[i][2] = '0';
        modification_status[i][3] = '0';
        modification_status[i][4] = '\0';
    }
    //Create pthread_t variables and mutex
    pthread_t read_threads[thread_counts[0]];
    pthread_t upper_threads[thread_counts[1]];
    pthread_t replace_threads[thread_counts[2]];
    pthread_t write_threads[thread_counts[3]];
    pthread_mutex_init(&read_mutex, NULL);
    //Share the work load among the threads, so each thread will know what indexes to modify,read,write.
    int** read_index_info = calculate_thread_index_info(thread_counts[0]);
    int** upper_index_info = calculate_thread_index_info(thread_counts[1]);
    int** replace_index_info = calculate_thread_index_info(thread_counts[2]);
    int** write_index_info = calculate_thread_index_info(thread_counts[3]);
    //Initiliaze threads
    for(i = 0; i < thread_counts[0]; i++) {
        if(pthread_create(&read_threads[i], NULL, &read_thread_func, read_index_info[i]) != 0) {
            perror("Error: Read thread creation failed.\n");
        }
    }
    free(read_index_info);
    pthread_mutex_init(&modify_mutex,NULL);
    for(i = 0; i < thread_counts[1]; i++) {
        if(pthread_create(&upper_threads[i], NULL, &upper_thread_func, upper_index_info[i]) != 0) {
            perror("Error: Upper thread creation failed.\n");
        }
    }
    free(upper_index_info);
    for(i = 0; i < thread_counts[2]; i++) {
        if(pthread_create(&replace_threads[i], NULL, &replace_thread_func, replace_index_info[i]) != 0) {
            perror("Error: Replace thread creation failed.\n");
        }
    }
    free(replace_index_info);
    pthread_mutex_init(&write_mutex, NULL);
    for(i = 0; i < thread_counts[3]; i++) {
        if(pthread_create(&write_threads[i], NULL, &write_thread_func, write_index_info[i]) != 0) {
            perror("Error: Write thread creation failed.\n");
        }
    }
    //Join threads
    free(write_index_info);
    for(i = 0; i < thread_counts[0]; i++) {
        if(pthread_join(read_threads[i], NULL) != 0) {
            perror("Error: Read thread join failed.\n");
        }
    }
    pthread_mutex_destroy(&read_mutex);
    for(i = 0; i < thread_counts[1]; i++) {
        if(pthread_join(upper_threads[i], NULL) != 0) {
            perror("Error: Upper thread join failed.\n");
        }
    }
    for(i = 0; i < thread_counts[2]; i++) {
        if(pthread_join(replace_threads[i], NULL) != 0) {
            perror("Error: Replace thread join failed.\n");
        }
    }
    for(i = 0; i < thread_counts[3]; i++) {
        if(pthread_join(write_threads[i], NULL) != 0) {
            perror("Error: Write thread join failed.\n");
        }
    }
    pthread_mutex_destroy(&write_mutex);
}

//Get command line arguments
void get_arguments(int argc, char* argv[]) {
    int i;
    for(i = 0; i < 4; i++) {
        thread_counts[i] = -1;
    }
    strcpy(file_name, "");
    for(i = 0; i < argc; i++) {
        if(argv[i][0] == '-') {
            if(argv[i][1] == 'd') {
                strcpy(file_name, argv[i+1]);
            }
            else if(argv[i][1] == 'n') {
                thread_counts[0] = atoi(argv[i+1]);
                thread_counts[1] = atoi(argv[i+2]);
                thread_counts[2] = atoi(argv[i+3]);
                thread_counts[3] = atoi(argv[i+4]);
            }
        }
    }
    for(i = 0; i < 4; i++) {
        if(thread_counts[i] == -1) {
            perror("Error: Invalid arguments.\n");
            exit(1);
        }
    }
}

//Get how many lines in the file
int get_line_count() {
    FILE *file = fopen(file_name, "r");
    if(file == NULL) {
        perror("Error opening file\n");
	exit(1);
    }
    int line_count = 0;
    char line[MAX_LINE_LENGTH];
    while(fgets(line, sizeof(line), file) != NULL) {
        line_count++;
    }
    fclose(file);
    return line_count;
}

//Calculate and share work load among processes, return value is passed to threads
//Each process knows which index to start and which index to end their process
int** calculate_thread_index_info(int read_thread_count) {
    int i;
    int** line_index_info = malloc(read_thread_count * sizeof(int*));
    for(i = 0; i < read_thread_count; i++) {
        line_index_info[i] = malloc(3 * sizeof(int));
    }
    if(read_thread_count > line_count) {
        read_thread_count = line_count;
    }
    for(i = 0; i < read_thread_count; i++) {
        line_index_info[i] = malloc(2 * sizeof(int));
        if((line_count % read_thread_count) != 0) {
            if(i == 0) {
                line_index_info[i][1] = (line_count / read_thread_count) + (line_count % read_thread_count);
                line_index_info[i][0] = 0;
                line_index_info[i][2] = i;
            }
            else {
                line_index_info[i][1] = line_count / read_thread_count;
                line_index_info[i][0] = (i * line_index_info[i][1]) + (line_count % read_thread_count);
                line_index_info[i][2] = i;
            }
        }
        else {
            line_index_info[i][1] = line_count / read_thread_count;
            line_index_info[i][0] = i * (line_count / read_thread_count);
            line_index_info[i][2] = i;
        }
    } 
    return line_index_info;
}

//Do the reading as long as there are unread lines in the file
void* read_thread_func(void* arg) {
    int* line_index_info = (int*)(arg);
    FILE* file = fopen(file_name, "r");
    if(file == NULL) {
        perror("Error opening file\n");
    }
    else {
        int count = 0;
        char line[MAX_LINE_LENGTH];
        while(fgets(line, MAX_LINE_LENGTH, file) != NULL) {
            if ((count >= line_index_info[0]) && (count < line_index_info[0] + line_index_info[1])) {
                printf("Read Thread %d read the line : %s", line_index_info[2], line);
                pthread_mutex_lock(&read_mutex);
                strcpy(lines[count], line);
                modification_status[count][2] = '1';
                pthread_mutex_unlock(&read_mutex);
            }
            count++;
        }
        fclose(file);
    }
}

//Check the modification_status array and call 'line_to_upper_case' func when there is an available line to modify
void* upper_thread_func(void* arg) {
    int i;
    int j;
    int status_counter = 0;
    int* line_index_info = (int*)(arg);
    int start_index = line_index_info[0];
    int end_index = line_index_info[0] + line_index_info[1];
    int thread_index = line_index_info[2];
    for(i = start_index; i < end_index; i++) {
        if((modification_status[i][0] == '0') && (modification_status[i][2] == '1')) {
            modification_status[i][0] = '1';
            line_to_upper_case(i, thread_index);
        }
        else {
            for(j = start_index; j < end_index; j++) {
                if(modification_status[j][0] == '2') {
                    status_counter++;
                }
            }
            if(status_counter == line_index_info[1]) {
                return NULL;
            }
            else {
                status_counter = 0;
                if(i == end_index - 1) {
                    i = start_index - 1;
                }
            }
        }
    }
}

//Check the modification_status array and call 'replace_chars' func when there is an available line to modify
void* replace_thread_func(void* arg) {
    int i;
    int j;
    int status_counter = 0;
    int* line_index_info = (int*)(arg);
    int start_index = line_index_info[0];
    int end_index = line_index_info[0] + line_index_info[1];
    int thread_index = line_index_info[2];
    for(i = start_index; i < end_index; i++) {
        if((modification_status[i][1] == '0') && (modification_status[i][2] == '1')) {
            modification_status[i][1] = '1';
            replace_chars(i, thread_index);
        }
        else {
            for(j = start_index; j < end_index; j++) {
                if(modification_status[j][1] == '2') {
                    status_counter++;
                }
            }
            if(status_counter == line_index_info[1]) {
                return NULL;
            }
            else {
                status_counter = 0;
                if(i == end_index - 1) {
                    i = start_index - 1;
                }
            }
        }
    }
}

//Check the modification_status array and call 'write_line' func when there is an available line to overwrite the file
void* write_thread_func(void* arg) {
    int i;
    int j;
    int status_counter = 0;
    int* line_index_info = (int*)(arg);
    int start_index = line_index_info[0];
    int end_index = line_index_info[0] + line_index_info[1];
    int thread_index = line_index_info[2];
    for(i = start_index; i < end_index; i++) {
        if((modification_status[i][0] == '2') && (modification_status[i][1] == '2') && (modification_status[i][2] == '1') && (modification_status[i][3] == '0')) {
            modification_status[i][3] = '1';
            write_line(i, thread_index);
        }
        else {
            for(j = start_index; j < end_index; j++) {
                if(modification_status[j][0] == '2') {
                    status_counter++;
                }
            }
            if(status_counter == line_index_info[1]) {
                return NULL;
            }
            else {
                status_counter = 0;
                if(i == end_index - 1) {
                    i = start_index - 1;
                }
            }
        }
    }
}

//Converts the given indexed line in the global lines array to upper case and update the modification_status array so that other threads will know that line is modified by an upper thread
void line_to_upper_case(int line_index, int thread_index) {
    int i;
    pthread_mutex_lock(&modify_mutex);
    int line_length = strlen(lines[line_index]);
    for(i = 0; i < line_length; i++) {
        lines[line_index][i] = toupper(lines[line_index][i]);
    }
    pthread_mutex_unlock(&modify_mutex);
    modification_status[line_index][0] = '2';
    printf("Upper thread %d changed the chars to upper case as - %s", thread_index, lines[line_index]);
}

//Replace the chars ' ' with '_' in the given indexed line in the global lines array and update the modification_status array so that other threads will know that line is modified by an replace thread
void replace_chars(int line_index, int thread_index) {
    int i;
    pthread_mutex_lock(&modify_mutex);
    int line_length = strlen(lines[line_index]);
    for(i = 0; i < line_length; i++) {
        if(lines[line_index][i] == ' ') {
            lines[line_index][i] = '_';
        }
    }
    pthread_mutex_unlock(&modify_mutex);
    modification_status[line_index][1] = '2';
    printf("Replace thread %d replaced the whitespaces as - %s", thread_index, lines[line_index]);
}

//Writes the line in lines array to the files given indexed line. Overwrites it.
void write_line(int line_index, int thread_index) {
    pthread_mutex_lock(&write_mutex);
    FILE* file = fopen(file_name, "r");
    if(file == NULL) {
        perror("Error opening file\n");
    }
    FILE* temp_file = tmpfile();
    if(temp_file == NULL) {
        perror("Error opening temp file\n");
        fclose(file);
        return;
    }
    char line[MAX_LINE_LENGTH];
    int count = 0;
    while(fgets(line, MAX_LINE_LENGTH, file) != NULL) {
        if(count == line_index) {
            fputs(lines[line_index], temp_file);
        }
        else {
            fputs(line, temp_file);
        }
        count++;
    }
    fclose(file);
    file = fopen(file_name, "w");
    if(file == NULL) {
        perror("Error opening file\n");
        return;
    }
    fseek(temp_file, 0, SEEK_SET);
    while(fgets(line, MAX_LINE_LENGTH, temp_file) != NULL) {
        fputs(line, file);
    }
    fclose(file);
    fclose(temp_file);
    pthread_mutex_unlock(&write_mutex);
    modification_status[line_index][3] = '2';
    printf("Write thread %d wrote the line - %s", thread_index, lines[line_index]);
}
