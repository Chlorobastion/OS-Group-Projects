#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define FILE_NAME "wiki_dump.txt"
#define FILE_SIZE 2000000

double mean_values[FILE_SIZE]; // the size of this array should be the total lines in the file
char *buffers[32]; // at maximum, we have access to 32 cores for multiprocessing; change this number if that changes
double buffer_mean_values[32];
int number_of_cores = 1; // number of cores being used in our multiprocessing

void init_arrays()
{
    int i;
    //number_of_cores = ; //change the number of cores to the number of cores allocated

    for(i = 0; i < FILE_SIZE; i++) // i should represent the number of lines in the file
    {
        mean_values[i] = 0;
    }
    
    for(i = 0; i < number_of_cores; i++) // our array size will change based on how many cores we are using
    {
        buffers[i] = NULL;
    }
}

void read_file()
{
    int i, line_offset = 0;
    size_t line_buf_size = 0;
    ssize_t line_size = 0;
    FILE *fp = fopen(FILE_NAME, "r");
    if(!fp)
    {
        printf("Error reading file %s\n", FILE_NAME);
        return;
    }

    /* Read the lines in batches */
    while (line_size >= 0)
    {
        for(i = 0; i < number_of_cores; i++)
        {
            /* Grab a line in the file */
            line_size = getline(&buffers[i], &line_buf_size, fp);
            if(line_size < 0)
            {
                continue;
            }
        }
        count_array();
        for(i = 0; i < number_of_cores; i++)
        {
            mean_values[line_offset + j] = buffer_mean_values[j];
        }
        line_offset += number_of_cores; // make sure to shift the location we are writing to in the mean_values array
    }

    /* Free the allocated the line buffers */
    for(i = 0; i < number_of_cores; i++)
    {
        free(buffers[i]);
        buffers[i] = NULL;
    }

    /* Close the file when we are done */
    fclose(fp);
}

void count_array()
{
    char thisChar;

    int i, j;
    for(i = 0; i < number_of_cores; i++) // i should represent the total number of lines read right now
    {
        j = 0;
        char *this_line = buffers[i];
        thisChar = this_line[j];
        // Iterate over the entire line
        while(thisChar != '\0')
        {
            buffer_mean_values[i] += (int) thisChar;
            j++; // Move to the next character in the line
            thisChar = this_line[j]; // get the character at this i and j (line and symbol)
        }
        // Check if the line had content to calculate
        if(j > 0)
        {
            buffer_mean_values[i] = buffer_mean_values[i] / j;
        }
    }
}

void print_results()
{
    int i;

    for(i = 0; i < FILE_SIZE; i++) // should be the number of lines in the file
    {
        printf("%d: %d\n", i, mean_values[i]); // print the information to the console
    }
}

main()
{
    init_arrays();
    read_file();
    print_results();
}