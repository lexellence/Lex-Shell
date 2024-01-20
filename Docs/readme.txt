//+---------------------\-------------------------------------
//|	  Install 	|
//\---------------------/-------------------------------------
To install lex for current user:
	$ sudo cp /path/to/lex /usr/local/bin
From any working directory, run lex:
	$ lex
Or create shortcut on desktop in Debian:
	Right-click desktop->Create Launcher
	Name=lex
	Command=/usr/local/bin/lex
	Run in terminal=enabled

//+-----------------------------\-----------------------------
//| 	Change default shell 	|
//\-----------------------------/-----------------------------
To change default shell for current user:
Edit /etc/shells file using mousepad or your favorite editor:
	$ sudo mousepad /etc/shells
Add /path/to/lex to end of that file, save, and close.
Verfiy that lex was added to shell list:
	$ cat /etc/shells
Change default shell to lex for current user: 
	$ chsh -s /path/to/lex
Log out and log in to apply changes. Opening a terminal will now use lex.

//+-----------------------------\-----------------------------
//| 	  Revert to Bash 	|
//\-----------------------------/-----------------------------
To switch back to bash:
	$ chsh -s /bin/bash
(If lex is crashing or not working for some reason and you cannot run chsh command, 
	you can use the first shell in /etc/shells by temporarily moving or renaming 
	the lex executable and then opening a terminal. Now you can follow the previous 
	instruction for switching back to bash. Then, if you wish, move lex executable 
	back to its original location.)

//+---------------------\-------------------------------------
//|	    Usage 	|
//\---------------------/-------------------------------------
&& or ; between commands executes a list of commands
	Example: command1 arg1 arg2 && command2 ; command3 arg3
cd <dir>: Change working directory. <..> = go up one level; <~> as first character of dir = home directory
	</> or no dir = root directory	
	Examples: $ cd ~/Desktop/Projects
		  $ cd ../../Desktop/Projects
		  $ cd /
cdl: Goes to working directory before last cd command
quit or exit: exits lex
history: prints the last ten commands
! <1-10>: Type the ! symbol, a space, then a number between 1 and 10 for the command you want to execute.
cat <file> : Prints the named file to the terminal.
help: displays this menu.
appName > <outputFile>: Execute a program (no arguments supported) from the current working directory 
	and redirect its output to outputFile (use >> for append version)
