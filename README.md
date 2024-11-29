<h1>Custom Shell</h1>

I implement a command line interpreter (CLI) or, as it is more commonly known, a shell. 
The shell should operate in this basic way: when you type in a command (in response to its prompt), the shell creates a child 
process that executes the command you entered and then prompts for more user input when it has finished.
The shells you implement will be similar to, but simpler than, the one you run every day in Unix. 

<h3>Structures</h3>
<h4>1. Basic Shell</h4>
The shell is very simple (conceptually): it runs in a while loop, repeatedly asking for input to tell it what command to execute. It then executes that command. 
The loop continues indefinitely, until the user types the built-in command exit, at which point it exits.
<h4>2. Paths</h4>
The path variable contains the list of all directories to search, in order, when the user types a command, like ls.
<h4>3. Built-in Commands</h4> exit, cd, jobs, fg, bg
<h4>4. Controlling Terminal</h4> Ctrl-C, Ctrl-Z, kill, pkill
<h4>5. Pipes</h4> | and & (run in background)
