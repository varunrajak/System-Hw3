#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Flags uses simple bit positioning to check
//which flags were set/supplied by the user

// Reference : https://stackoverflow.com/questions/1631266/flags-enum-c
// https://stackoverflow.com/questions/37936552/flags-and-operation-on-enums-c-sharp
// https://stackoverflow.com/questions/8447/what-does-the-flags-enum-attribute-mean-in-c
// Also taken reference from my solution to this from last year.
// Can also use boolean expressions to check if the particular flag is set or not.

enum Flags{
    size_limit = 1 << 0,
    extra_details = 1 << 1,
    filter_string = 1 << 2,
    depth_control = 1 << 3,
    dirs_or_files_only = 1 << 4,
	execute_file = 1 << 5,
    execute_files = 1 << 6
};

//Aliasing
typedef enum Flags Flags;
Flags m_flags = 0;

// structure to keep track of all supplied arguments 
struct Args {
    char *start_dir;
    int size_limit;
    char *filter;
    int depth;
    int show_details;
    int dirs_or_files_only;
	char *cmd;
	char *all_files_cmd;
	char files[1024][1024];
	int num_of_files;
    int execute_all;
};

typedef struct Args Args;
Args cmd_args;


//this function uses getopt to parse the supplied
//commandline arguments
// using logical OR to get different attribute like size,edpth etc
int parse_args(int argc, char ** argv) {
    int c;
    while ((c = getopt(argc, argv, "s:t:f:S:e:E:")) != -1) {
        switch (c) {
        case 's':
            cmd_args.size_limit = atoi(optarg);
            m_flags |= size_limit;
            break;
        case 'f':
            cmd_args.filter = strdup(optarg);
            m_flags |= filter_string;
            if (optind < argc && argv[optind][0] != '-') {
                cmd_args.depth = atoi(argv[optind++]);
                m_flags |= depth_control;
            }
            break;
        case 'S':
            cmd_args.show_details = 1;
            m_flags |= extra_details;
            break;
        case 't':
            cmd_args.dirs_or_files_only = (optarg[0] == 'd' ? 1 : 2);
            m_flags |= dirs_or_files_only;
            break;
		case 'e':
            cmd_args.cmd = strdup(optarg);            
			m_flags |= execute_file;
            break;
        case 'E':
			cmd_args.all_files_cmd = strdup(optarg);
            m_flags |= execute_files;
            break;
        case '?':
            fprintf(stderr, "Usage: %s [-s file size in bytes] [-f <string pattern> <depth>] [-S]\n", argv[0]);
            return 0;
        default:
            abort();
        }
    }

    if (optind < argc) {
        cmd_args.start_dir = argv[optind];
    } else {
        return 0;
    }

    return 1;
}

// function to execute the command using the exec system call. It creates a child using fork and execute the command
void execute_cmd(char* cmd) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid < 0) {
        printf("Error: fork failed\n");
        exit(1);
    } else if (pid == 0) {  // child process
        char* argv[10];    // maximum 10 arguments
        int argc = 0;
        char* token = strtok(cmd, " ");  // split command string by space
        while (token != NULL && argc < 100) {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }
        argv[argc] = NULL;  // last element must be NULL for execvp()
        execvp(argv[0], argv);
        // execvp() only returns if there was an error
        printf("Error: execvp failed\n");
        exit(1);
    } else {  // parent process
        waitpid(pid, &status, 0);  // wait for child process to finish
    }
}





//returns the indentation depending upon depth/size
// https://man7.org/linux/man-pages/man3/bzero.3.html
char* get_indent(int sz) {
    char *id = malloc(sizeof (char) * sz+1);
    bzero(id, sz+1);
    for(int i=0; i<sz; ++i)
        id[i] = '\t';
    return id;
}

typedef char* (*Indent) (int);
//display/list all the file starting from the path
//max_depth defines the maximum directory depth to penitrate/scan
//depth is another argument which is used to display tabs depending upon current
//directory relative to the starting path
void list_files(char *path, int max_depth, int depth, Indent id) {
    if(max_depth < 0)
        return;

    DIR *dir = opendir(path);   //opens the target directory
    if (dir == NULL) {
        fprintf(stderr, "opendir %s: %s\n", path, strerror(errno));
        return;
    }


// reference : https://stackoverflow.com/questions/18776559/new-at-c-and-working-with-directories-trouble-opening-files-within-subdirectori
// Reference : https://stackoverflow.com/questions/42840480/c-get-filenames-sizes-and-permissions-using-stat
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char file_path[PATH_MAX];
        snprintf(file_path, sizeof(file_path), "%s/%s", path, entry->d_name);

        struct stat st;
		
        if (stat(file_path, &st) != 0) {
            fprintf(stderr, "stat %s: %s\n", file_path, strerror(errno));
            continue;
        }

        if (S_ISDIR(st.st_mode) && !((m_flags & dirs_or_files_only) && (cmd_args.dirs_or_files_only == 2))) {
            printf("%s (0 bytes)\r\n", entry->d_name);
            list_files(file_path, max_depth-1, depth+1, id);
            continue;
        }

        if(m_flags & dirs_or_files_only && cmd_args.dirs_or_files_only == 1)
            continue;

        if((m_flags & size_limit) && st.st_size > cmd_args.size_limit)
            continue;

        if((m_flags & filter_string) && (strstr(entry->d_name, cmd_args.filter) == NULL))
            continue;

        if(m_flags & extra_details) {
            printf("%s%s (%ld bytes, ", (depth?id(depth):""), entry->d_name, st.st_size);

            //print permissions
            printf("%c%c%c%c%c%c%c%c%c, ",
                   (st.st_mode & S_IRUSR) ? 'r' : '-',
                   (st.st_mode & S_IWUSR) ? 'w' : '-',
                   (st.st_mode & S_IXUSR) ? 'x' : '-',
                   (st.st_mode & S_IRGRP) ? 'r' : '-',
                   (st.st_mode & S_IWGRP) ? 'w' : '-',
                   (st.st_mode & S_IXGRP) ? 'x' : '-',
                   (st.st_mode & S_IROTH) ? 'r' : '-',
                   (st.st_mode & S_IWOTH) ? 'w' : '-',
                   (st.st_mode & S_IXOTH) ? 'x' : '-');

            //print last access time
            char time_buf[32];
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&st.st_atime));
            printf("%s)\n", time_buf);
        } else {
            if(S_ISDIR(st.st_mode) && ((m_flags & dirs_or_files_only) && (cmd_args.dirs_or_files_only == 2)))
                continue;
            else if(S_ISLNK(st.st_mode) && ((m_flags & dirs_or_files_only) && (cmd_args.dirs_or_files_only == 2))) {
                char target[1024];
                int len = readlink(entry->d_name, target, sizeof(target) - 1);
                if(len == -1) {
                    printf("error reading link\r\n");
                    continue;
                }
                target[len] = '\0';

                printf("%s (%s)\n", entry->d_name, target);
            } else {
                printf("%s%s (%ld bytes)\n", (depth?id(depth):""), entry->d_name, (S_ISDIR(st.st_mode)) ? 0 : st.st_size);
				
				// if there is -e flag in the command it will handle this, by executing command on each file				
				if ((m_flags & execute_file) && !(S_ISDIR(st.st_mode))){
					char temp[1024];
					strcpy(temp,cmd_args.cmd);
					strcat(temp," ");
					strcat(temp,file_path);
					execute_cmd(temp);
				}
				else if (m_flags & execute_files){  //this will handle -E flag it will store the path of file, which will be executed
													// at the end		
					
					strcpy(cmd_args.files[cmd_args.num_of_files], file_path);
					cmd_args.num_of_files++;
					
				}
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if(!parse_args(argc, argv)) {
        cmd_args.start_dir = malloc(sizeof (char) * 100);
        bzero(cmd_args.start_dir, 100);
        getcwd(cmd_args.start_dir, 100);
    }
	cmd_args.num_of_files = 0;
    Indent func = get_indent;
    list_files(cmd_args.start_dir, (m_flags & depth_control) ? cmd_args.depth : 1, 0, func);
	
	//handling the -E flag here, concatenate all the paths and execute the command
	if (m_flags && execute_files){
		for (int i=0;i<cmd_args.num_of_files;i++){
			strcat(cmd_args.all_files_cmd, " ");
			strcat(cmd_args.all_files_cmd, cmd_args.files[i]);
		}
		execute_cmd (cmd_args.all_files_cmd);
	}


    return 0;
}
