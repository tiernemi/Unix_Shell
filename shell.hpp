#ifndef SHELL_HPP_UFO0YKSH
#define SHELL_HPP_UFO0YKSH


/*
 * =====================================================================================
 *
 *       Filename:  shell.hpp
 *
 *    Description:  Header file for shell object.
 *
 *        Version:  1.0
 *        Created:  26/03/16 11:44:10
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  Michael Tierney (MT), tiernemi@tcd.ie
 *
 * =====================================================================================
 */

#include <string>
#include "parser.hpp"

/* 
 * ===  CLASS  =========================================================================
 *         Name:  Shell
 *       Fields: pid_t sessionID - The sid of the shell.
 *               pid_t shellPID - The pid of the shell.
 *               std::string homeDirectory - The directory of $HOME.
 *               std::string currdirectory - The current directory of the shell.
 *               std::string prevdirectory - The previous directory of the shell.
 *               std::vector<std::string> - backgroundCommands - The commands running
 *                  in the background due to this shell.
 *               std::vector<std::string> - backgroundCommandsPIDs - The command pids running
 *                  in the background due to this shell.
 *               std::vector<std::string> - backgroundCommandsIDs - The commands shell ids
 *                  running in the background due to this shell.
 *  Description:  Shell class that prompts for input, handles command execution and
 *                keeps track of spawned processes.
 *  =====================================================================================
 */

class Shell {
 public:
	Shell() ;
	std::string prompt() ;
	void execute(std::string) ;
	void checkBackgrounds() ;
	void displayShellName() ;
	virtual ~Shell() ;
 private:
	pid_t sessionID ;
	pid_t shellPID ;
	pid_t shellPGID ;
	int terminalFD ;
	std::string homeDirectory ;
	std::string currDirectory ;
	std::string prevDirectory ;
	std::vector<std::string> backgroundCommands ;
	std::vector<int> backgroundCommandsIDs ;
	std::vector<pid_t> backgroundCommandsPIDs ;
 private:
	void runCommand(std::vector<std::vector<std::string>> & cmds) ;
	void handlePipe(std::vector<std::vector<std::string>> & cmds) ;
	void handleOverwrite(std::vector<std::vector<std::string>> & cmds, int dest) ;
	void handleInput(std::vector<std::vector<std::string>> & cmds) ;
	void handleAppend(std::vector<std::vector<std::string>> & cmds, int dest) ;
	void handleBackground(std::vector<std::vector<std::string>> & cmds) ;
	void expandArgs(std::vector<std::vector<std::string>> & cmds) ;
	void changeDirectory(std::string arg) ;
} ;		/* -----  end of class Shell  ----- */

#endif /* end of include guard: SHELL_HPP_UFO0YKSH */
