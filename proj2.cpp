/*  Name: Sanket Makkar
    CaseID:         sxm1626
    File Name:      proj2.cpp
    Date Created:   9/25/2024
    Description:    This is the file that contains a main function for the simple CLI web-based client.
                    The point behind making this file is mostly to orchestrate functionality held within other
                    files of this project.
*/
#include <stdio.h>
#include "ArgParser.h"
#include <string>
#include <string.h>

using namespace std;

// we will store the file name we grab from parseArgs here
string URL;
string savePath;

string host;
string path;

// this callback will grab the url from parseArgs
void urlStorage(char url[]){
    URL = url;
}
// this callback will grab the savePath from parseArgs
void saveStorage(char path[]){
    savePath = path;
}

void hostStorage(string givenHost){
    host = givenHost;
}

void pathStorage(string givenPath){
    path = givenPath;
}

int main(int argc, char *argv[])
{
    // execute parse args, passing in a callback that will grab file name, and returning/storing the args flag indicator
    parseArgs(argc, argv, urlStorage, saveStorage);
    //int args = 
    // setup the verifier
    // Ipv4Verifier verifier = Ipv4Verifier(args, fileNameStorage);
    // verify
    // verifier.IPv4Verify();
    // printf("%s\n", URL.c_str());

    printf("%s\n", savePath.c_str());
    printf("%s\n", grabHostAndPath(URL, hostStorage, pathStorage).c_str());
    printf("%s + %s\n", host.c_str(), path.c_str());
    return 0;
}
