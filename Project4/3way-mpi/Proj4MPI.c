#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define FILE_NAME "/homes/dan/625/wiki_dump.txt"
#define FILE_SIZE 1000000

float mean_values[FILE_SIZE]; // the size of this array should be the total lines in the file
float local_mean_values[FILE_SIZE];
int number_of_cores = 1; // number of cores being used in our multiprocessing
int end_of_file = 1; // This will act similar to a boolean
int lines_to_read = FILE_SIZE;
char *local_line_buffer;
size_t local_line_buf_size = 0;
ssize_t local_line_size;

void init_arrays()
{
    int i;

    for(i = 0; i < FILE_SIZE; i++) // i should represent the number of lines in the file
    {
        mean_values[i] = 0;
    }
}

void *mean_lines(int rank, int iterations)
{
    int myLine = rank + iterations * number_of_cores; // Need to keep track of the line numbr this thread is dealing with

    if(local_line_size >= 0) // We are not at end of the file
    {
        int j = 0;
        char thisChar;
        float thisMean = 0;
        char *thisLine = local_line_buffer;
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

        local_mean_values[myLine] = thisMean;
    }
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
        number_of_cores = atoi(argv[1]); // I don't think MPI implementation makes this work like the others
        if(argc >= 3 && atoi(argv[2]) < FILE_SIZE)
        {
            lines_to_read = atoi(argv[2]);
        }
    }

    int i, rc, iterations = 0;
    int numtasks, rank;
    MPI_Status Status;
    char *pass_line_buffer;
    size_t pass_line_buf_size = 0;
    ssize_t pass_line_size;
    time_t start; // time before parallel
    time_t end; // time after parallel
    time(&start);

    FILE *fp = fopen(FILE_NAME, "r");
    if(!fp)
    {
        printf("Error reading file %s\n", FILE_NAME);
        return;
    }

    rc = MPI_Init(&argc, &argv);
    if(rc != MPI_SUCCESS)
    {
        printf("Error starting MPI program. Terminating.\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    number_of_cores = numtasks; // Numtasks is defined in a different setup call

    if(rank == 0)
    {
        init_arrays();
        pass_line_buffer = NULL;
        pass_line_size = 0;
    }

    for(i = 0; i < FILE_SIZE; i++)
    {
        local_mean_values[i] = 0; // Initialize all local mean value arrays
    }

    local_line_buffer = NULL;
    local_line_size = 0;

    while(end_of_file > 0)
    {
        for(i = 0; i < rank; i++)
        {
            local_line_size = getline(&local_line_buffer, &local_line_buf_size, fp); // Skip ahead lines in the file
            if(local_line_size < 0 || (i + iterations * number_of_cores) >= lines_to_read)
            {
                end_of_file = 0;
                continue;
            }
        }
        local_line_size = getline(&local_line_buffer, &local_line_buf_size, fp); // Grab the line in the file for this rank
        if(local_line_size < 0)
        {
            end_of_file = 0;
        }

        mean_lines(rank, iterations); // Do our work with the line

        for(i = 0; i < number_of_cores - rank - 1; i++)
        {
            local_line_size = getline(&local_line_buffer, &local_line_buf_size, fp); // Skip ahead lines in the file
            if(local_line_size < 0 || (i + rank + iterations * number_of_cores) > lines_to_read)
            {
                end_of_file = 0;
                continue;
            }
        }

        iterations++;
    }

    MPI_Reduce(local_mean_values, mean_values, FILE_SIZE, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD); // Combine all of the calculated values into the single array

    if(rank == 0)
    {
        /* Close the file when we are done */
        fclose(fp);
        time(&end);
        //print_results();
        double elapsed = difftime(end, start); // time taken to calculate mean
        printf("Cores used: %d\n", number_of_cores); // debugging info.
        printf("File Size (#of lines being read): %d\n", lines_to_read); // debugging info.
        printf("Time elapsed: %0.1f seconds\n", elapsed); // debugging info.
    }

    MPI_Finalize();
}