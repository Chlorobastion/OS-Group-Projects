#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//#define FILE_NAME "Test.txt"
#define FILE_NAME "/homes/dan/625/wiki_dump.txt"
#define FILE_SIZE 1000000

double mean_values[FILE_SIZE]; // the size of this array should be the total lines in the file
int line_num = 0; // global variable to keep track of the current line being dealt with
int number_of_cores = 1; // number of cores being used in our multiprocessing

void init_arrays()
{
    int i;

    for(i = 0; i < FILE_SIZE; i++) // i should represent the number of lines in the file
    {
        mean_values[i] = 0;
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

    int end_of_file = 1; // This will act similar to a boolean
    int nthreads, tid;
    while(end_of_file > 0)
    {
        // Split up the threads to go in parallel here
        #pragma omp parallel num_threads(number_of_cores) private(tid)
        {
            tid = omp_get_thread_num();

            /* Only master thread will do this (Debugging) */
            /*
            if(tid == 0)
            {
                nthreads = omp_get_num_threads();
                printf("Number of threads = %d\n", nthreads);
            }
            */

            int myLine;
            char *line_buffer = NULL;
            size_t line_buf_size = 0;
            ssize_t line_size = 0;

            // Critical section here
            #pragma omp critical
            {
                line_size = getline(&line_buffer, &line_buf_size, fp); // Grab a line in the file
                myLine = line_num; // Keep track of what line this thread is reading
                line_num++; // Make sure the next thread knows it has the next line
            }

            if(line_size >= 0) // We are not at end of the file
            {
                int i = 0;
                char thisChar;
                double thisMean = 0;
                thisChar = line_buffer[i];
                // Iterate over the entire line, ending at the null terminal or a new line character
                while(thisChar != '\0' && thisChar != '\n')
                {
                    thisMean += (int) thisChar;
                    i++; // Move to the next character in the line
                    thisChar = line_buffer[i]; // get the character at this i and j (line and symbol)
                }
                // Check if the line had content to calculate
                if(i > 0)
                {
                    thisMean = thisMean / i;
                }

                mean_values[myLine] = thisMean;
            }
            else
            {
                end_of_file = 0; // We are at the end of the file, so we should break out of the loop after all current threads die
            }
        }
        // Rejoin the parallel threads and reloop unless we are done with the file
    }

    /* Close the file when we are done */
    fclose(fp);
}

void print_results()
{
    int i;

    for(i = 0; i < FILE_SIZE; i++) // should be the number of lines in the file
    {
        if(mean_values[i] != 0) // We won't care about lines that don't have content (make out files smaller)
        {
            printf("%d: %.1f\n", i, mean_values[i]); // print the information to the console
            
        }
    }
}

main(int argc, char *argv[])
{
    if(argc = 2) // If we pass in more than just the name of the program, we must be changing the number of cores used
    {
        number_of_cores = atoi(argv[1]); // change the number of cores to the number of cores allocated
    }
    init_arrays();
    time_t start; // time before parallel
    time_t end; // time after parallel
    time(&start);
    read_file();
    time(&end);
    print_results();
    double elapsed = difftime(end, start); // time taken to calculate mean
    printf("Cores used: %d\n", number_of_cores); // debugging info.
    printf("Time elapsed: %0.1f seconds\n", elapsed); // debugging info.
}