#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define FILE_NAME "/homes/dan/625/wiki_dump.txt"
#define FILE_SIZE 1000000

double mean_values[FILE_SIZE]; // the size of this array should be the total lines in the file
int line_num = 0; // global variable to keep track of the current line being dealt with
int number_of_cores = 1; // number of cores being used in our multiprocessing
int lines_to_read = FILE_SIZE;
int lines_per_thread = 25;
int end_of_file = 1; // This will act similar to a boolean
pthread_mutex_t lock;

void init_arrays()
{
    int i;

    for(i = 0; i < FILE_SIZE; i++) // i should represent the number of lines in the file
    {
        mean_values[i] = 0;
    }
    
    pthread_mutex_init(&lock, NULL);
}

void *read_lines(void *fp)
{
    int myFirstLine, i;
    char *line_buffer[lines_per_thread];
    size_t line_buf_size = 0;
    ssize_t line_size[lines_per_thread];

    for(i = 0; i < lines_per_thread; i++)
    {
        line_buffer[i] = NULL;
        line_size[i] = 0;
    }

    // Critical section here
    pthread_mutex_lock(&lock);
    for(i = 0; i < lines_per_thread; i++) // going to grab a bunch of lines at once
    {
        line_size[i] = getline(&line_buffer[i], &line_buf_size, fp); // Grab a line in the file
        if(line_size[i] < 0 || line_num + i > lines_to_read)
        {
            continue;
        }
    }
    myFirstLine = line_num; // Keep track of what line this thread is reading
    line_num += lines_per_thread; // Make sure the next thread knows it has the next line
    pthread_mutex_unlock(&lock);

    for(i = 0; i < lines_per_thread; i++)
    {
        if(line_size[i] >= 0 && myFirstLine + i < lines_to_read) // We are not at end of the file
        {
            int j = 0;
            char thisChar;
            double thisMean = 0;
            char *thisLine = line_buffer[i];
            thisChar = thisLine[j];
            // Iterate over the entire line, ending at the null terminal or a new line character
            while(thisChar != '\0' && thisChar != '\n')
            {
                thisMean += (int) thisChar;
                j++; // Move to the next character in the line
                thisChar = thisLine[j]; // get the character at this i and j (line and symbol)
            }
            // Check if the line had content to calculate
            if(j > 0)
            {
                thisMean = thisMean / j;
            }

            mean_values[myFirstLine + i] = thisMean;
        }
        else
        {
            end_of_file = 0; // We are at the end of the file, so we should break out of the loop after all current threads die
            break;
        }
    }
}

void read_file()
{
    FILE *fp = fopen(FILE_NAME, "r");
    if(!fp)
    {
        printf("Error reading file %s\n", FILE_NAME);
        return;
    }

    int nthreads, tid, i;
    pthread_t threads[number_of_cores];
    while(end_of_file > 0)
    {
       for(i = 0; i < number_of_cores; i++)
       {
           // Split up the threads to go in parallel here
           pthread_create(&threads[i], NULL, read_lines, (void *)fp);
       }
    }

    /* Close the file when we are done */
    fclose(fp);
}

void print_results()
{
    int i;

    for(i = 0; i < lines_to_read; i++) // we only need to report the first hundred lines
    {
        if(mean_values[i] != 0) // We won't care about lines that don't have content (make out files smaller)
        {
            printf("%d: %.1f\n", i, mean_values[i]); // print the information to the console
        }
    }
}

main(int argc, char *argv[])
{
    if(argc >= 2) // If we pass in more than just the name of the program, we must be changing the number of cores used
    {
        number_of_cores = atoi(argv[1]); // change the number of cores to the number of cores allocated
        if(argc >= 3 && atoi(argv[2]) < FILE_SIZE)
        {
            lines_to_read = atoi(argv[2]);
        }
    }
    init_arrays();
    time_t start; // time before parallel
    time_t end; // time after parallel
    time(&start);
    read_file();
    time(&end);
    print_results();
    double elapsed = difftime(end, start); // time taken to calculate mean
    pthread_mutex_destroy(&lock);
    printf("Cores used: %d\n", number_of_cores); // debugging info.
    printf("Time elapsed: %0.1f seconds\n", elapsed); // debugging info.
}