/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  Main function for running shell.
 *
 *        Version:  1.0
 *        Created:  26/03/16 12:11:49
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  Michael Tierney (MT), tiernemi@tcd.ie
 *
 * =====================================================================================
 */

#include "shell.hpp"

int main(int argc, char *argv[]) {
	// Create new shell. //
	Shell newShell ;
	newShell.displayShellName() ;
	while (1) {
		// Check background processes. //
		newShell.checkBackgrounds() ;
		// Get input. //
		std::string cmd = newShell.prompt() ;
		// Execute input. //
		newShell.execute(cmd) ;
	}
	return EXIT_SUCCESS ;
}
