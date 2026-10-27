# Simple Shell
A simple Linux shell implemented in C that reads user input, tokenizes commands into an array of arguments, and executes system commands using `execvp()`.

The shell parses user input by separating the command from its arguments. It uses the first token as the command and passes the rest of the tokens as arguments to `execvp()`, allowing users to execute system commands directly from the shell

## Key Features
* Reads and parsing user input.
* Tokenizes input into a command and its arguments.
* Executes system commands using `execvp()`.
* Passes command-line arguments to target program


## How to Run the Project
After compiling with 'make build', run the executable with './shell'
There should be a prompt for user input. The shell loops to allow
the user to enter different commands/prompts. Use 'exit' to quit.
I also build a short test file inside the 'test' directory. 

To run the test script in tests/test_file.sh:
1. 'make build'
2. 'chmod +x tests/test_file.sh;
3. './tests/test_file.sh'

The script feeds a set sequence of commands into the shell then
prints '.history_file'.

## Project Issues

1. If the user passed extra arguments when using history (ex. 'history 1'),
it was treated as a normal 'history' command. I added an extra check for any
extra arguments to pass an error.

## Sources Used

1. The Open Group Base Specifications Issue 8
2. cppreference.com, C Standard Library headers
