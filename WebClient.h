/*  Name: Sanket Makkar
    CaseID:         sxm1626
    File Name:      WebClient.h
    Date Created:   9/27/2024
    Description:    The purpose of this file is to define a header for the core functionality - i.e. the socket
                    work and arg-responses based on resulting socket work - as required by this assignment. In
                    particular this file just defines the WebClient class, private data members used by this class,
                    some internal and helper methods, and the main method used to orchestrate this classes behavior.
*/
#ifndef WEBCLIENT_H
#define WEBCLIENT_H

#include <string>
#include <vector>
using namespace std;

class WebClient{
    private:
        // passed to constructor
        int argLine;
        string url;
        string savePath;    
        // grabbed from url
        string host = "";
        string path = "";

        // the result from searching the network (header, response)
        string headerIn = "";
        vector<unsigned char> responseIn;

        // core methods used behind the scenes
        void httpGET(string request); // core socket work
        void handleCLIArgs(string header, string request); // Actually deals with the CLI arg reactions
        void handleHttpRSPStatus(string header, string getRequest); // deal with the response of the network (error codes)

        // small helpers
        int parseArgLineOptionals(); // figure out which CLI optional was requested
        string appendAtNewline(string toAppend, string stringToConsider); // append something to every newline of a string
        string searchForUrl(string header); // given a returned header, continue to search for the url

    public:
        WebClient(int argLine, string url, string inputSavePath); // basic constructor
        ~WebClient() = default; // nothing special for destructor

        void grabFromNetwork(); // method used to orchestrate core functionality (grabbing something from a url, reacting to CLI args, and saving it to a file)
};

#endif // WEBCLIENT_H