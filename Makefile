sshell: sshell.c
	gcc -g -Wall -Werror -o sshell sshell.c

clean:
	$(RM) sshell