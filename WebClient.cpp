/*  Name: Sanket Makkar
    CaseID:         sxm1626
    File Name:      proj2.cpp
    Date Created:   9/27/2024
    Description:    The purpose of this file is to implement the core functionality - i.e. the socket work and
                    arg-responses based on resulting socket work - as required by this assignment. This file in 
                    particular implements the methods intended to do this work as defined within the WebClient.h
                    header file.
*/
#include "WebClient.h"
#include "ArgParser.h"
#include <vector>
#include <string>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Arguments that are parsed in that we have to know about (for the sake of understanding the cmdLine int passed here)
#define ARG_INFO   0x2
#define ARG_PRINT_STD_REQUEST 0x4
#define ARG_PRINT_STD_HEADER 0x8

// Useful information for the socket work we will be doing
#define PORT 80
#define PROTOCOL "tcp"

// Data regarding buffer management (used when we fread from the socket)
#define BUFLEN 1024
#define BYTES_TO_WRITE_AT_A_TIME 1

// Error codes we need to be informed of
#define CODE_OK 200
#define CODE_MOVED_PERMANENTLY 301
#define CODE_BAD_REQUEST 400
#define CODE_NOT_FOUND 404
#define CODE_HTTP_VERSION_NOT_SUPPORTED 505

// Helpful variables for initialization of counters, handling off-by-one errors, and comparisons
#define COUNTER_INITIAL_VALUE 0
#define OFF_BY_ONE_OFFSET 1
#define FUNCTION_ERROR_RETURN_VALUE -1

// how we can exit the program
#define exitWithErr exit(FUNCTION_ERROR_RETURN_VALUE)
#define exitWithNoErr exit(0)

using namespace std;

WebClient::WebClient(int argLine, string url, string inputSavePath) : argLine(argLine), url(url), savePath(inputSavePath) {
    /* Basic constructor for the web client - stores passed argline, url, save path, and grabs the host and path from the url*/
    vector<string> hostPathVector = grabHostAndPath(url);
    host = hostPathVector.at(COUNTER_INITIAL_VALUE);
    path = hostPathVector.at(COUNTER_INITIAL_VALUE + OFF_BY_ONE_OFFSET);
}

void WebClient::GrabFromNetwork(){
    /* This is the function that actually allows the web-client to do its core work (going to url, providing output, saving file appropriately) 
     * When we reach here: 
        * host url is - at least in the sense of http:// formatting - valid
        * for now we can treat the savePath as valid (we will make the file using this)
        * path should be either / or some path to follow
        * host should be what follows the http://
    */

    /* HTTP GET request formatting (GET <path> HTTP/1.0
                                    HOST: <host>
                                    User-Agent: <name of user agent>
    */
    string getRequest = "GET " + path + " HTTP/1.0\r\nHost: " + host + "\r\nUser-Agent: Case CSDS 325/425 WebClient 0.1\r\n\r\n";
    
    // actual socket work
    httpGET(getRequest);

    // deal with 200, 301, 400, 404, 505 error codes
    handleHttpRSPStatus(headerIn);

    // CLI work
    handleCLIArgs(headerIn, getRequest);

    // save the file (we would have exit if non-200 (not OK) error codes came up)
    // note that we use fwrite to handle the case of non text data (instead of more straightforward fprintf for text)
    FILE * file = fopen(savePath.c_str(), "wb");
    if (file == NULL){
        fprintf(stderr, "Error: Unable to open file specified by %s\n", savePath.c_str());
        exitWithErr;   
    }
    fwrite(responseIn.data(), BYTES_TO_WRITE_AT_A_TIME, responseIn.size(), file);
    fclose(file);
    
}

void WebClient::handleCLIArgs(string header, string request){
    int argLineOptional = parseArgLineOptionals(); // figure out which cli args were made
    // we request using the double newline ending format, and appendAtNewline doesn't tend to like that format very much
    string reducedRequest = request.substr(COUNTER_INITIAL_VALUE, request.find("\r\n\r\n"));
    switch(argLineOptional){ // for each arg case
        case ARG_INFO: // -i
            printf("INFO: host: %s\n", host.c_str());
            printf("INFO: web_file: %s\n", path.c_str());
            printf("INFO: output_file: %s\n", savePath.c_str());
            break;

        case ARG_PRINT_STD_HEADER: // -a
            printf("%s", appendAtNewline("RSP: ", header).c_str());
            break;

        case ARG_PRINT_STD_REQUEST: // -q
            printf("%s", appendAtNewline("REQ: ", reducedRequest).c_str());
            break;
        
        default: // no arg provided - that is okay here (note that case of too many args provided is handled in parseArgs)
            break;
    }
}

int WebClient::parseArgLineOptionals(){
    // Just a simple helper to figure out which arg was passed here
    // Note: We know only one of ARG_INFO, ARG_PRINT_STD_HEADER, ARG_PRINT_STD_REQUEST was passed at this point as parseArgs ensures this
    if (flagsContainBit(argLine, ARG_INFO)){
        return ARG_INFO;
    }
    else if (flagsContainBit(argLine, ARG_PRINT_STD_HEADER)){
        return ARG_PRINT_STD_HEADER;
    }
    else if (flagsContainBit(argLine, ARG_PRINT_STD_REQUEST)){
        return ARG_PRINT_STD_REQUEST;
    }
    return -1;
}

void WebClient::httpGET(string request){
    struct sockaddr_in sin;
    struct hostent *hinfo;
    struct protoent *protoinfo;
    int sd;

    /* DNS lookup on host*/
    hinfo = gethostbyname(host.c_str());
    if (hinfo == NULL){
        fprintf(stderr, "Error: DNS lookup failed, host '%s' does not exist\n", host.c_str());
        exitWithErr;
    }

    /* set endpoint information */
    memset ((char *)&sin, 0x0, sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);
    memcpy ((char *)&sin.sin_addr,hinfo->h_addr,hinfo->h_length);

    // grab protocol TCP information
    if ((protoinfo = getprotobyname (PROTOCOL)) == NULL){
        fprintf(stderr, "Error: Invalid protocol: '%s'\n", PROTOCOL);
        exitWithErr;
    }

    /* allocate a socket*/
    sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if (sd < 0){
        fprintf(stderr, "Error: Unable to allocate a socket using protocol '%s'\n", PROTOCOL);
        exitWithErr;
    }

    /* connect the socket */
    if (connect (sd, (struct sockaddr *)&sin, sizeof(sin)) < COUNTER_INITIAL_VALUE){
        fprintf(stderr, "Error: Unable to create socket\n");
        exitWithErr;
    }
    
    // make a socket pointer
    FILE *sp = fdopen(sd, "r");
    if (sp == NULL){
        fprintf(stderr, "Error: Unable to make socket pointer\n");
        exitWithErr;
    }

    /* Send HTTP GET request */
    if (write(sd, request.c_str(), strlen(request.c_str())) < COUNTER_INITIAL_VALUE){
        printf("Error: Web client unable to send request '%s'\n", request.c_str());
        exitWithErr;
    }
    
    /* Read and print the HTTP response */
    string responseRead = ""; // will be a string to store the header
    char tempChar; // used by fgetc
    int separatorPos = 0; // used to check for header end
    
    // first look for header - get chars from socket until we hit EOF or we find the double newline separator
    while ((tempChar = fgetc(sp)) != EOF){
        responseRead += tempChar;
        separatorPos = responseRead.find("\r\n\r\n");
        if (separatorPos >= 0){
            break;
        }
    }
    // now that we have the header we get rid of the double newline separator
    string headerRead = responseRead.substr(COUNTER_INITIAL_VALUE, separatorPos);

    // now look through remainder of data using fread
    vector<unsigned char> buffer; // this holds the data (unsigned char works for any UTF - all types of data not just ascii)
    char tempBuffer[BUFLEN]; // we fill up this tempBuffer for the sake of using fread, but this is just a bucket for the vector above
    int bytesRead = 0; // helps us append to buffer (appropriate quantity from tempBuffer)
    while ((bytesRead = fread(tempBuffer, 1, sizeof(tempBuffer), sp)) > 0) {
        buffer.insert(buffer.end(), tempBuffer, tempBuffer + bytesRead); 
    }

    // now store the resulting headerIn and responseIn w/in local vars
    headerIn = headerRead;
    responseIn = buffer;

    /* close & exit */
    close(sd);
    fclose(sp);

    return;
}

void WebClient::handleHttpRSPStatus(string header){
    /* Responds to the error code of a returned HTTP response*/
    // predefine some useful vars that help define the position of the HTTP response error code
    int headerPrecodeSize = strlen("HTTP/1.1 ");
    int headerCodeSize = strlen("###");
    // find the HTTP response error code
    string responseCode = header.substr(headerPrecodeSize, headerPrecodeSize + headerCodeSize + OFF_BY_ONE_OFFSET);
    int code = stoi(responseCode); // we want it as an int

    // for each code situation... (print err if not 200)
    switch(code){
        case CODE_OK:
            break;
        case CODE_MOVED_PERMANENTLY:
            printf("Error (%d): Moved permanently, unable to access requested location\n", CODE_MOVED_PERMANENTLY);
            exitWithErr;
            break;
        case CODE_BAD_REQUEST:
            printf("Error (%d): Bad request, please re-enter a valid request to the server\n", CODE_BAD_REQUEST);
            exitWithErr;
            break;
        case CODE_NOT_FOUND:
            printf("Error (%d): Requested document not found on server\n", CODE_NOT_FOUND);
            exitWithErr;
            break;
        case CODE_HTTP_VERSION_NOT_SUPPORTED:
            printf("Error (%d): HTTP version is not supported\n", CODE_HTTP_VERSION_NOT_SUPPORTED);
            exitWithErr;
            break;
        default: // if we don't get a 200, but we don't know the explicit error - just mention the non-200 error code
            printf("Non-%d error code, please try again", CODE_OK);
            exitWithErr;
            break;
    }
}

string WebClient::appendAtNewline(string toAppend, string stringToConsider){
    /* Helps us append something to a string (good for RSP: ... and REQ: ... parts of this assignment)*/
    string totalString = ""; // what we use to hold the appended result in total
    string remainingString = stringToConsider; // we slowly shrink this 
    string newlineSeparator = "\n"; // this is the newline separator we will use to iterate line by line
    bool doNotBreakBool = true; // tells us when to break the loop
    while (doNotBreakBool == true){
        string upToNewLine = ""; // use this to get part up to separator
        // find the newline separator
        int newLinePosition = remainingString.find(newlineSeparator.c_str());
        if (newLinePosition <= COUNTER_INITIAL_VALUE){ // if no newline separator left then exit after this
            doNotBreakBool = false;
            upToNewLine = remainingString;
        }
        else{
            // grab the part up to the separator and reduce the part we look at (remainingString)
            upToNewLine = remainingString.substr(COUNTER_INITIAL_VALUE, newLinePosition + strlen(newlineSeparator.c_str()));
            remainingString = remainingString.substr(newLinePosition + strlen(newlineSeparator.c_str()));    
        }
        
        // put the prefix
        totalString.append(toAppend);
        // put the part of the line
        totalString.append(upToNewLine);
    }
    return totalString + "\r\n";
}