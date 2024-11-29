# Custom-Shell

I implement a command line interpreter (CLI) or, as it is more commonly known, a shell. 
The shell should operate in this basic way: when you type in a command (in response to its prompt), the shell creates a child 
process that executes the command you entered and then prompts for more user input when it has finished.
The shells you implement will be similar to, but simpler than, the one you run every day in Unix. 

<h4>Structures</h4>
1. Basic Shell
The shell is very simple (conceptually): it runs in a while loop, repeatedly asking for input to tell it what command to execute. It then executes that command. 
The loop continues indefinitely, until the user types the built-in command exit, at which point it exits.
2. Paths
The path variable contains the list of all directories to search, in order, when the user types a command, like ls.
3. Built-in Commands: exit, cd, jobs, fg, bg
4. Controlling Terminal: Ctrl-C, Ctrl-Z, kill, pkill
5. Pipes: | and & (run in background)
