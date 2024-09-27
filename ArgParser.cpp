/*  Name: Sanket Makkar
    CaseID:         sxm1626
    File Name:      ArgParser.cpp
    Date Created:   9/5/2024
    Description:    This file contains implementations for functions described within ArgParser.h.
                    In general, this class is intended to provide a set of methods helpful to any
                    other file that may want to parse, or perform some parsing-related action, on
                    user input.
*/

#include "ArgParser.h"  
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <list>
#include <string>
#include <string.h>

using namespace std;

// These are flags we will check for and set
#define ARG_INPUT_URL  0x1
#define ARG_INFO   0x2
#define ARG_PRINT_STD_REQUEST 0x4
#define ARG_PRINT_STD_HEADER 0x8
#define ARG_SAVE_LOCATION 0x10

// Basic counter related constants
#define COUNTER_INITIALIZER 0
#define OFF_BY_ONE_OFFSET 1

// this is a local copy of the file name and a CLI flags holder (init to 0 - no flags)
char *INPUT_URL = NULL;
char *saveLocation = NULL;
unsigned int cmd_line_flags = 0x00000;
list<int> requiredArgsList = {ARG_INPUT_URL, ARG_SAVE_LOCATION};
list<int> uniqueArgsList = {ARG_INFO, ARG_PRINT_STD_REQUEST, ARG_PRINT_STD_HEADER};

// rather than writing exit(-1) every time I want to exit with an error, I wrote up this macro to make it easier
#define exitWithErr exit(-1)

// We take an input of argc, argv from the caller - arguments and number of arguments, as well as two callbacks to execute at the end of the function call
int parseArgs(int argc, char *argv[], void (*grabINPUT_URL)(char INPUT_URL[]), void (*grabSave)(char saveLocation[])){
    int opt;
    while ((opt = getopt(argc, argv, "iqau:w:")) != -1)
    {
        // check for cases of iqauw
        switch (opt)
        {
            case 'i': // if valid -i given, set ARG_INFO flag
                checkNonSetFlag(cmd_line_flags, ARG_INFO, 'i'); // don't let the user enter a flag more than one time
                cmd_line_flags |= ARG_INFO;
                break;
            case 'q': // if valid -q given, set ARG_PRINT_STD_REQUEST flag
                checkNonSetFlag(cmd_line_flags, ARG_PRINT_STD_REQUEST, 'q'); // don't let the user enter a flag more than one time
                cmd_line_flags |= ARG_PRINT_STD_REQUEST;
                break;
            case 'a': // if valid -a given, set ARG_PRINT_STD_HEADER flag
                checkNonSetFlag(cmd_line_flags, ARG_PRINT_STD_HEADER, 'a'); // don't let the user enter a flag more than one time
                cmd_line_flags |= ARG_PRINT_STD_HEADER;
                break;
            case 'u': // if valid -u given, set ARG_INPUT_URL flag
                checkNonSetFlag(cmd_line_flags, ARG_INPUT_URL, 'u'); // don't let the user enter a flag more than one time
                cmd_line_flags |= ARG_INPUT_URL;
                INPUT_URL = optarg;
                break;
            case 'w': // if valid -w given, set ARG_SAVE_LOCATION flag
                checkNonSetFlag(cmd_line_flags, ARG_SAVE_LOCATION, 'w'); // don't let the user enter a flag more than one time
                cmd_line_flags |= ARG_SAVE_LOCATION;
                saveLocation = optarg;
                break;
            case '?': // if invalid option provided, react with an error message and exit
                usage(argv[0]);
                exitWithErr;
            default: // and if nothing else gets caught then just react with an error message and exit
                usage(argv[0]);
        }
    }

    if (cmd_line_flags == 0){ // no command-line flags set implies no CLI option was given up until now (the loop above would have caught it)
        fprintf(stderr, "Error: no command line option given\n");
        exitWithErr;
    }

    if (!verifyValidCommandInput(cmd_line_flags)){ // ensure that we have a -f, and that we have either a -l or a -s
        exitWithErr;
    }

    grabINPUT_URL(INPUT_URL); // execute the callback passed in, this will just pass the INPUT_URL
    grabSave(saveLocation); // execute the callback passed in, this will just pass the Save location for HTTP contents
    return cmd_line_flags;
}

string grabHostAndPath(string url, void (*grabHost)(string host), void (*grabPath)(string path)){
    string defaultProtocol = "http://";
    if (!validateURL(&url, defaultProtocol)){ // has a side effect of lower-casing any http:// given
        printf("URL protocol not specified correctly\n");
        exitWithErr;
    }

    string hostName = "";
    string pathName = "/";

    int hostLength = COUNTER_INITIALIZER;
    int pathStart = COUNTER_INITIALIZER;
    string postProtocolURL = url.substr(defaultProtocol.length(), url.length());
    for (char& urlChar : postProtocolURL){
        if (urlChar == '/'){
            pathStart++;
            break;
        }
        hostLength++;
        pathStart++;
    }

    hostName += postProtocolURL.substr(COUNTER_INITIALIZER, hostLength);
    pathName += postProtocolURL.substr(pathStart, postProtocolURL.length());

    grabHost(hostName);
    grabPath(pathName);

    return url;
}

bool validateURL(string* url, string defaultProtocol){
    if (url->length() < defaultProtocol.length()){
        return false;
    }

    string protocol = url->substr(COUNTER_INITIALIZER, defaultProtocol.length());
    for(char& protoChar : protocol){
        protoChar = tolower(protoChar);
    }
    url->replace(COUNTER_INITIALIZER, defaultProtocol.length(), protocol); // we just want to lowercase the url here

    if (protocol != defaultProtocol){
        return false;
    }

    return true;
}

// This is a simple helper to ensure we have all required and appropriately used flags
bool verifyValidCommandInput(int cmd_flags){
    if (!checkUniqueArgs(cmd_flags)) // do we have -s or -l but not both?
    {
        fprintf(stderr, "Error: only one may be allowed -i, -q, or -a\n");
        return false;
    }
    else if (!checkHasArg(cmd_flags, ARG_INPUT_URL)) // do we have a -f with a fileName specified?
    {
        fprintf(stderr, "Error: please include a INPUT_URL\n");
        return false;
    }
    else if (!checkHasArg(cmd_flags, ARG_SAVE_LOCATION)) // do we have a -f with a fileName specified?
    {
        fprintf(stderr, "Error: please include a save location\n");
        return false;
    }
    return true;
}

// the way we verify we have either -l or -s but not both
bool checkUniqueArgs(int cmd_flags){
    int bitsContained = 0;
    for (int arg : uniqueArgsList){
        if (flagsContainBit(cmd_flags, arg)){
            bitsContained++;
        }
    }
    return bool(bitsContained <= 1);
}

// the way we verify we have a -f flag
bool checkHasArg(int cmd_flags, int arg){
    return flagsContainBit(cmd_flags, arg);
}

// more universal helper to ensure the flags contain a specified bit
bool flagsContainBit(int cmd_flags, int bit){ 
    if (cmd_flags & bit){
        return true;
    }
    return false;
}

// verify a flag was not set
void checkNonSetFlag(int cmd_flags, int bit, char arg){
    if (flagsContainBit(cmd_flags, bit)){
        fprintf(stderr, "Error: failed user entry (2 instances of -%c)\n", arg);
        exitWithErr;
    }
}

// inform the user of the flags available to use this program
void usage(char *progname){
    fprintf(stderr, "%s [-i] [-q] [-a] -u INPUT_URL -w filename", progname);
    fprintf(stderr, "   -i    print debugging information about command line parameters to stdout (only one of -i, -q, -a may be given)\n");
    fprintf(stderr, "   -q    print the sent HTTP request to stdout (only one of -i, -q, -a may be given)\n");
    fprintf(stderr, "   -a    print the sent HTTP response header (only one of -i, -q, -a may be given)\n");
    fprintf(stderr, "   -w X  specifies location to save file is at path X (MUST BE GIVEN)");
    fprintf(stderr, "   -u Y  specifies a INPUT_URL the client will access (MUST BE GIVEN)");
    exitWithErr;
}