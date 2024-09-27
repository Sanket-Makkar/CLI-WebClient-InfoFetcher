/*  Name: Sanket Makkar
    CaseID:         sxm1626
    File Name:      ArgParser.h
    Date Created:   9/25/2024
    Description:    This file contains function declarations for the ArgParser module. These functions
                    include parseArgs - which is the main function used to orchestrate parsing CLI
                    arguments - and also helper functions other files may find useful such as 
                    "usage", "flagsContainBit".
*/

#ifndef ARGSPARSER_H
#define ARGSPARSER_H

#include <string>
using namespace std;

void usage(char *progname);
int parseArgs(int argc, char *argv[], void (*grabURL)(char URL[]), void (*grabSave)(char saveLocation[]));
bool checkUniqueArgs(int cmd_flags);
bool checkHasArg(int cmd_flags, int arg);
bool verifyValidCommandInput(int cmd_flags);
bool flagsContainBit(int cmd_flags, int bit);
void checkNonSetFlag(int cmd_flags, int bit, char arg);
string grabHostAndPath(string url, void (*grabHost)(string host), void (*grabPath)(string path));
bool validateURL(string* url, string defaultProtocol);

#endif // ARGSPARSER_H
