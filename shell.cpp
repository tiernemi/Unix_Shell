/*
 * =====================================================================================
 *
 *       Filename:  shell.cpp
 *
 *    Description:  Source for Shell object.
 *
 *        Version:  1.0
 *        Created:  26/03/16 11:49:28
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  Michael Tierney (MT), tiernemi@tcd.ie
 *
 * =====================================================================================
 */

#include "shell.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h> 
#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <wordexp.h> 

/*
 * ===  MEMBER FUNCTION CLASS : Shell  ===============================================
 *         Name:  Shell
 *  Description:  Constructs a shell object that has a session id, process id
 *  a home directory and current directory.
 * =====================================================================================
 */

Shell::Shell() {
	char * dirBuf = new char[300] ;
	if (getcwd(dirBuf, 300) == NULL) {
		std::cerr << "Error getting current directory" << std::endl ;
	}
	currDirectory = dirBuf ;
	prevDirectory = dirBuf ;
	homeDirectory = getenv("HOME") ;
	delete [] dirBuf ;
	sessionID = setsid() ;
	shellPID = getpid() ;
	terminalFD = open(ctermid(NULL), O_WRONLY) ;
	shellPGID = getpgid(shellPID) ;
	signal(SIGTTOU, SIG_IGN) ;
}		/* -----  end of member function Shell  ----- */

/* 
 * ===  MEMBER FUNCTION CLASS : Shell  ================================================
 *         Name:  checkBackgrounds
 *  Description:  Function that examines background processes spawned by the shell and
 *                alerts the user if they have been completed.
 * =====================================================================================
 */

void Shell::checkBackgrounds() {
	for (unsigned int i = 0 ; i < backgroundCommandsPIDs.size() ; ++i) {
		int status ;
		int wpid ;
		wpid = waitpid(backgroundCommandsPIDs[i], &status, WNOHANG) ;
		if (wpid != 0) {
			std::cout << "[" << i+1 << "]   " << "Done           " << 
				backgroundCommands[i] << std::endl ;
			backgroundCommands.erase(backgroundCommands.begin()+i) ;
			backgroundCommandsPIDs.erase(backgroundCommandsPIDs.begin()+i) ;
			backgroundCommandsIDs.erase(backgroundCommandsIDs.begin()+i) ;
			--i ;
		}
	}
}		/* -----  end of member function checkBackgrounds  ----- */

/* 
 * ===  MEMBER FUNCTION CLASS : Shell  =================================================
 *         Name:  createPrompt
 *      Returns:  The command string taken from readline.
 *  Description:  Uses readline to read in input and returns the command string. 
 * =====================================================================================
 */

std::string Shell::prompt() {
	std::string promptStrng = currDirectory + ":$ " ;
	char * cmd = readline(promptStrng.c_str()) ;
	add_history(cmd) ;
	std::string cmdStr(cmd) ;
	return cmdStr ;
}		/* -----  end of member function createPrompt  ----- */

/* 
 * ===  MEMBER FUNCTION CLASS : Shell  ================================================
 *         Name:  execute
 *    Arguments:  std::string cmd - The raw command string to be executed.
 *  Description:  Parses the command string, and executes the process groups detected.
 *                If '&' is encountered the process is run in the background. 'CD' is
 *                treated as a special case. The shell forks a process group and waits
 *                until it's returned provided the process is run in the foreground.
 * =====================================================================================
 */

void Shell::execute(std::string cmd) {

	// Parse commands into seperate groups, then pipes + IO, seperate commands and then arguments. //
	std::vector<std::vector<std::vector<std::string>>> parsedCmd = Parser::parse(cmd) ;

	for (unsigned int i = 0 ; i < parsedCmd.size() ; ++i) {
		// Expand wildcards and ~ . //
		expandArgs(parsedCmd[i]) ;
		pid_t child_pid ;
		int status ;
		bool bg = false ;

		// Handle change directory. //
		if (parsedCmd[i][0][0].compare("cd") == 0 && parsedCmd[i].size() == 1) {
			changeDirectory(parsedCmd[i][0][1]) ;
		} else if (parsedCmd[i][0][0][0] == '/') {
			std::cout << parsedCmd[i][0][0] << " is a directory" << std::endl;
		} else if (parsedCmd[i][0][0].compare(".") == 0) {
			std::cout << ". is not a valid single command" << std::endl;
		} else if (parsedCmd[i][0][0].compare("..") == 0) {
			std::cout << ".. is not a valid single command" << std::endl;
		}
		// Handle regular command. //
		else {
			// Handle background processes. //
			if (parsedCmd[i][parsedCmd[i].size()-1][0].compare("&") == 0) {
				parsedCmd[i].pop_back() ;
				bg = true ;
				handleBackground(parsedCmd[i]) ;
			}
			// If foreground process then fork. //
			if (!bg) {
				if ((child_pid = fork()) < 0) {
					printf("*** ERROR: forking child process failed\n");
					exit(1);
				} else if (child_pid == 0) {
					runCommand(parsedCmd[i]) ;
				} else {
					setpgid(child_pid, 0) ;
					tcsetpgrp(terminalFD, getpgid(child_pid)) ;
					while (wait(&status) != child_pid) {
						;
					}
					tcsetpgrp(terminalFD, shellPGID) ;
				}
			}
		}
	}
}		/* -----  end of member function execute  ----- */

/* 
 * ===  MEMBER FUNCTION CLASS : Shell  =================================================
 *         Name:  runCommand
 *    Arguments:  std::vector<std::vector<std::string>> & cmds - The parsed process group.
 *  Description:  Runs the parsed command group backwards down the stack. Processes
 *                high on the stack spawn child processes and wait for them to
 *                finish. The command is run recursively in the case of multiple pipes.
 * =====================================================================================
 */

void Shell::runCommand(std::vector<std::vector<std::string>> & cmds) {
	std::vector<std::string> currentCmd = cmds[cmds.size()-1] ; 
	cmds.pop_back() ;
	if (currentCmd[0].compare("|") == 0) {
		handlePipe(cmds) ;
	} else if (currentCmd[0].compare(">") == 0) {
		int dest = atoi(currentCmd[1].c_str()) ;
		handleOverwrite(cmds, dest) ;
	} else if (currentCmd[0].compare(">>") == 0) {
		int dest = atoi(currentCmd[1].c_str()) ;
		handleAppend(cmds, dest) ;
	} else if (currentCmd[0].compare("<") == 0) {
		handleInput(cmds) ;
	} else if (currentCmd[0].compare("&") == 0) {
		handleBackground(cmds) ;
	} else if (currentCmd[0].compare("") == 0) {
		exit(EXIT_SUCCESS) ;
	} else {
		char ** args = new char*[currentCmd.size()+1] ;
		for (unsigned int j = 0 ; j < currentCmd.size() ; ++j) {
			args[j] = strdup(currentCmd[j].c_str()) ;
		}
		args[currentCmd.size()] = NULL ;
		execvpe(args[0], args, environ) ;
		if (errno == EACCES) {
			std::cerr << "Error cannot acces command" << std::endl ;
		} else if (errno == ENOENT) {
			std::cerr << "Error command " << args[0] <<  " does not exist" << std::endl ;
		} else if (errno == EIO) {
			std::cerr << "I/O Error" << std::endl ;
		}
		exit (EXIT_FAILURE) ;
	}
}		/* -----  end of member function runCommand  ----- */

/* 
 * ===  MEMBER FUNCTION CLASS : Shell  =================================================
 *         Name:  handlePipe
 *    Arguments:  std::vector<std::vector<std::string>> & cmds - The remaining commands.
 *  Description:  Spawns a child process, waits for input while the child executes.
 * =====================================================================================
 */

void Shell::handlePipe(std::vector<std::vector<std::string>> & cmds) {
	int fd[2] ;
	pid_t child_pid ;
	int status ;
	pipe(fd) ;

	if ((child_pid = fork()) < 0) {
		printf("*** ERROR: forking child process failed\n");
		exit(1);
	} else if (child_pid == 0) {
		std::vector<std::string> currentCmd = cmds[cmds.size()-1] ; 
		cmds.pop_back() ;
		while ((dup2(fd[1], STDOUT_FILENO) == -1) && (errno == EINTR)) {} ;
		close(fd[1]); // close read
		close(fd[0]); // close read
		runCommand(cmds) ;
	} else {
		while ((dup2(fd[0], STDIN_FILENO) == -1) && (errno == EINTR)) {} ;
		close(fd[0]) ;
		close(fd[1]) ; // close write
		while(wait(&status) != child_pid) {
			;
		}
		runCommand(cmds) ;
	}
}		/* -----  end of member function handlePipe  ----- */


/* 
 * ===  MEMBER FUNCTION CLASS : Shell  =================================================
 *         Name:  handleOverwrite
 *    Arguments:  std::vector<std::vector<std::string>> & cmds - The remaining commands.
 *                int dest - Stream file descriptor.
 *  Description:  Switches stream dest with file and overwrites contents.
 * =====================================================================================
 */

void Shell::handleOverwrite(std::vector<std::vector<std::string>> & cmds, int dest) {
	std::vector<std::string> currentCmd = cmds[cmds.size()-1] ; 
	cmds.pop_back() ;
	int out = open(currentCmd[0].c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | 
			S_IRGRP | S_IWGRP | S_IWUSR) ;
	dup2(out, dest) ;
	runCommand(cmds) ;
}		/* -----  end of member function handleOverwrite  ----- */

/* 
 * ===  MEMBER FUNCTION CLASS : Shell  =================================================
 *         Name:  handleInput
 *    Arguments:  std::vector<std::vector<std::string>> & cmds - The remaining commands.
 *  Description:  Switches stdin to file.
 * =====================================================================================
 */

void Shell::handleInput(std::vector<std::vector<std::string>> & cmds) {
	std::vector<std::string> currentCmd = cmds[cmds.size()-1] ; 
	cmds.pop_back() ;
	int in = open(currentCmd[0].c_str(), O_RDONLY) ;
	dup2(in, STDIN_FILENO) ;
	runCommand(cmds) ;
}		/* -----  end of member function handleInput  ----- */

/* 
 * ===  MEMBER FUNCTION CLASS : Shell  =================================================
 *         Name:  handleAppend
 *    Arguments:  std::vector<std::vector<std::string>> & cmds - The remaining commands.
 *                int dest - Stream file descriptor.
 *  Description:  Switches stream dest with file and appends contents.
 * =====================================================================================
 */

void Shell::handleAppend(std::vector<std::vector<std::string>> & cmds, int dest) {	
	std::vector<std::string> currentCmd = cmds[cmds.size()-1] ; 
	cmds.pop_back() ;
	int out = open(currentCmd[0].c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | 
			S_IRGRP | S_IWGRP | S_IWUSR) ;
	dup2(out, dest) ;
	runCommand(cmds) ;
}		/* -----  end of member function handleAppend  ----- */

/* 
 * ===  MEMBER FUNCTION CLASS : Shell  =================================================
 *         Name:  handleBackground
 *    Arguments:  std::vector<std::vector<std::string>> & cmds - The remaining commands.
 *  Description:  Spawns a child process into the background. Alerts user to the
 *                created process and adds entries to the background process vectors.
 * =====================================================================================
 */

void Shell::handleBackground(std::vector<std::vector<std::string>> & cmds) {	
	pid_t child_pid ;
	if ((child_pid = fork()) < 0) {
		printf("*** ERROR: forking child process failed\\n");
		exit(1);
	} else if (child_pid == 0) {
		pid_t gid = setpgid(child_pid, 0) ;
		runCommand(cmds) ;
	} else {
		backgroundCommands.push_back(Parser::convertCmdsToString(cmds)) ;
		backgroundCommandsPIDs.push_back(child_pid) ;
		if (backgroundCommandsIDs.size() != 0) {
			backgroundCommandsIDs.push_back(backgroundCommandsIDs[backgroundCommandsIDs.size()-1]+1) ;
		} else {
			backgroundCommandsIDs.push_back(1) ;
		}
		std::cout << "[" << backgroundCommandsIDs[backgroundCommandsIDs.size()-1] << "] " << child_pid << std::endl ;
	}
}		/* -----  end of member function handleBackgroun  ----- */


/* 
 * ===  MEMBER FUNCTION CLASS : Shell  =================================================
 *         Name:  expandArgs
 *    Arguments:  std::vector<std::vector<std::string>> & cmds) - The remaining commands.
 *  Description:  Expands terminal arguments such as ~.
 * =====================================================================================
 */

void Shell::expandArgs(std::vector<std::vector<std::string>> & cmds) {
	wordexp_t expansion ;
	for (unsigned int i = 0 ; i < cmds.size() ; ++i) {
		for (unsigned int j = 0 ; j < cmds[i].size() ; ++j) {
			int res = wordexp(cmds[i][j].c_str(), &expansion, WRDE_SHOWERR) ;
			if (res == 0 && expansion.we_wordc > 0) {
				if (cmds[i][j].compare(expansion.we_wordv[0]) != 0) {
					cmds[i].erase(cmds[i].begin()+j) ;
					cmds[i].insert(cmds[i].begin()+j, expansion.we_wordv, expansion.we_wordv + expansion.we_wordc) ;
				}
				j += expansion.we_wordc-1 ;
			}
		}
	}
	wordfree(&expansion) ;
}		/* -----  end of member function expandArgs  ----- */

/* 
 * ===  MEMBER FUNCTION CLASS : Shell  =================================================
 *         Name:  changeDirectory
 *    Arguments:  std::string path - New directory.
 *  Description:  Changes current directory to new path. '-' reverts to the old directory.
 * =====================================================================================
 */

void Shell::changeDirectory(std::string path) {
	if (path.compare("-") == 0) {
		chdir(prevDirectory.c_str()) ;
	} else {
		chdir(path.c_str()) ;
	}
	prevDirectory = currDirectory ;
	char * currDirectoryStr = new char[300] ;
	if (getcwd(currDirectoryStr, 300) == NULL) {
		std::cerr << "Error getting current directory" << std::endl; 
	}
	currDirectory = currDirectoryStr ; 
	delete [] currDirectoryStr ;
}		/* -----  end of member function changeDirectory  ----- */

/* 
 * ===  MEMBER FUNCTION CLASS : Shell  =================================================
 *         Name:  displayShellName
 *  Description:  Prints out name of shell (Ghost in the shell was too big)
 * =====================================================================================
 */

void Shell::displayShellName() {
	printf("\n");
	printf("\n");
	printf("\n");
	printf("            ('-. .-.              .-')   .-') _     .-')    ('-. .-.   ('-.   \n") ;
	printf("           ( OO )  /             ( OO ).(  OO) )   ( OO ). ( OO )  / _(  OO)           \n") ;          
	printf("  ,----.   ,--. ,--..-'),-----. (_)---\\_)     '._ (_)---\\_),--. ,--.(,------.,--.      ,--.    \n") ;  
	printf(" '  .-./-')|  | |  ( OO'  .-.  '/    _ ||'--...__)/    _ | |  | |  | |  .---'|  |.-')  |  |.-')  \n") ;
	printf(" |  |_( O- )   .|  /   |  | |  |\\  :` `.'--.  .--'\\  :` `. |   .|  | |  |    |  | OO ) |  | OO ) \n") ;
	printf(" |  | .--, \\       \\_) |  |\\|  | '..`''.)  |  |    '..`''.)|       |(|  '--. |  |`-' | |  |`-' | \n") ;
	printf("(|  | '. (_/  .-.  | \\ |  | |  |.-._)   \\  |  |   .-._)   \\|  .-.  | |  .--'(|  '---.'(|  '---.' \n") ;
	printf(" |  '--'  ||  | |  |  `'  '-'  '\\       /  |  |   \\       /|  | |  | |  `---.|      |  |      |  \n") ;
	printf("  `------' `--' `--'    `-----'  `-----'   `--'    `-----' `--' `--' `------'`------'  `------' \n") ;
	printf("\n");
	printf("\n");
	printf("\n");
}		/* -----  end of member function displayShellName  ----- */


Shell::~Shell() {
	;
}		/* -----  end of member function ~Shell  ----- */
