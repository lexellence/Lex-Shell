To change default shell for current user:

Edit /etc/shells file using mousepad or your favorite editor:
	$ sudo mousepad /etc/shells
Add /path/to/lex to end of that file, save, and close.
Verfiy that lex was added to shell list:
	$ cat /etc/shells
Change default shell to lex for current user: 
	$ chsh -s /path/to/lex
Log out and log in to apply changes. Opening a terminal will now use lex.

To switch back to bash:
	$ chsh -s /bin/bash
If lex is crashing or not working for some reason and you cannot run chsh command, you can use the first shell in /etc/shells by temporarily moving or renaming the lex executable and then opening a terminal. Now you can follow the previous instruction for switching back to bash. Then, if you wish, move lex executable back to its original location.
