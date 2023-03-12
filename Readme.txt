Description:
	For this assignment, I made changes to our previous project. I added two more flags -e and -E to our project.
To accomplish the functionality of these two flags, I added two flags "execute_file" for -e and "execute_files"
for -E to our m_flags structure. A function execute_cmd(char *) is added which takes a command as parameter, extract
the arguments, create a child using the fork and execute the command in child using the exec() system call.
For -e flag, we simply add the file_path while printing to the command and execute it using our execute_command function.
So, execute_command runs everytime for each file that meets the criteria.
For -E flag, we simply store the file_path in array of strings. And after the filenames are printed, we concatenate all
the file_paths and make a single command. Then we run that command using the execute_command function. 
