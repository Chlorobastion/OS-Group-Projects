This is my file describing how I edited the tests to include the
alarm-mega test.

To get the test to run, I used the grep command to find
all of the locations of alarm-multiple and tried to mimic how it
was implemented to implement alarm-mega. This included adding an
alarm-mega to tests.c similar to alarm-multiple, adding an
alarm-mega to test names in Make.tests similar to alarm-multiple,
adding an external test_func test_alarm_mega to tests.h similar
to test_alarm_multiple, adding a test_alarm_mega function to
alarm_wait.c identical to test_alarm_multiple but replacing the
7 in the function with 70 as instructed, and finally creating an
alarm-mega.ck file (also made identically to the alarm-multiple.ck
file, except with 7 becoming 70) in the tests/threads folder.

Finally, I included the logs for the multiple and mega tests in
both the src/threads folder and a src/threads/IsLog* folder
because the documentation was unclear on which was preferred.
This README file is also contained in both locations, for similar
reasons.
