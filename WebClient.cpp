#include "WebClient.h"
#include "ArgParser.h"
#include <vector>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

#define ARG_INPUT_URL  0x1
#define ARG_INFO   0x2
#define ARG_PRINT_STD_REQUEST 0x4
#define ARG_PRINT_STD_HEADER 0x8
#define ARG_SAVE_LOCATION 0x10

#define ERROR 1
#define REQUIRED_ARGC 3
#define PORT 80
#define PROTOCOL "tcp"
#define BUFLEN 1024

#define CODE_OK 200
#define CODE_MOVED_PERMANENTLY 301
#define CODE_BAD_REQUEST 400
#define CODE_NOT_FOUND 404
#define CODE_HTTP_VERSION_NOT_SUPPORTED 505

#define COUNTER_INITIAL_VALUE 0
#define OFF_BY_ONE_OFFSET 1
#define FUNCTION_ERROR_RETURN_VALUE -1

#define exitWithErr exit(-1)
#define exitWithNoErr exit(0)

WebClient::WebClient(int argLine, string url, string inputSavePath) : argLine(argLine), url(url), savePath(inputSavePath) {
    vector<string> hostPathVector = grabHostAndPath(url);
    host = hostPathVector.at(0);
    path = hostPathVector.at(1);
}

void WebClient::Exec(){
    /* When we reach here: 
        * host url is - at least in the sense of http:// formatting - valid
        * for now we can treat the savePath as valid (we will make the file using this)
        * path should be either / or some path to follow
        * host should be what follows the http://
    */

    /* HTTP GET request formatting (GET <path> HTTP/1.0
                                    HOST: <host>
                                    User-Agent: <name of user agent>
    */
    getRequest = "GET " + path + " HTTP/1.0\r\nHost: " + host + "\r\nUser-Agent: Case CSDS 325/425 WebClient 0.1\r\n\r\n";
    
    // actual socket work
    vector<string> splitResponse = httpGET(getRequest);

    // unpack the values
    string header = splitResponse.at(COUNTER_INITIAL_VALUE);
    string response = splitResponse.at(OFF_BY_ONE_OFFSET);

    // deal with 200, 301, 400, 404, 505 error codes
    handleHttpRSPStatus(header);

    // CLI work
    handleCLIArgs(header, getRequest);

    // save the file (we would have exit if non-200 (not OK) error codes came up)
    FILE * file = fopen(savePath.c_str(), "w");
    fprintf(file, "%s", response.c_str());
    fclose(file);
}

void WebClient::handleCLIArgs(string header, string request){
    int argLineOptional = parseArgLineOptionals();
    switch(argLineOptional){
        case ARG_INFO: // -i
            printf("INFO: host: %s\n", host.c_str());
            printf("INFO: web_file: %s\n", path.c_str());
            printf("INFO: output_file: %s\n", savePath.c_str());
            break;

        case ARG_PRINT_STD_HEADER: // -a
            printf("%s", appendAtNewline("RSP: ", header).c_str());
            break;

        case ARG_PRINT_STD_REQUEST: // -q
            printf("%s", appendAtNewline("REQ: ", request).c_str());
            break;
        
        default:
            break;
    }
}

int WebClient::parseArgLineOptionals(){
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

vector<string> WebClient::httpGET(string request){
    struct sockaddr_in sin;
    struct hostent *hinfo;
    struct protoent *protoinfo;
    char buffer [BUFLEN];
    int sd, ret;
    
    

    /* DNS lookup on host*/
    hinfo = gethostbyname(host.c_str());
    if (hinfo == NULL){
        printf("Error: DNS lookup failed, host '%s' does not exist\n", host.c_str());
        exitWithErr;
    }

    /* set endpoint information */
    memset ((char *)&sin, 0x0, sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);
    memcpy ((char *)&sin.sin_addr,hinfo->h_addr,hinfo->h_length);

    // grab protocol TCP information
    if ((protoinfo = getprotobyname (PROTOCOL)) == NULL){
        printf("Error: Invalid protocol: '%s'\n", PROTOCOL);
        exitWithErr;
    }

    /* allocate a socket*/
    sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if (sd < 0){
        printf("Error: Unable to allocate a socket using protocol '%s'\n", PROTOCOL);
        exitWithErr;
    }

    /* connect the socket */
    if (connect (sd, (struct sockaddr *)&sin, sizeof(sin)) < 0){
        printf("Error: Unable to create socket\n");
        exitWithErr;
    }

    /* Send HTTP GET request */
    if (write(sd, request.c_str(), strlen(request.c_str())) < 0){
        printf("Error: Web client unable to send request '%s'\n", request.c_str());
        exitWithErr;
    }
    
    /* Read and print the HTTP response */
    // We use a vector here to capture the whole response
    string wholeResponse = "";
    memset (buffer,0x0,BUFLEN);
    while ((ret = read(sd, buffer, BUFLEN -1))){
        buffer[ret] = '\0';
        wholeResponse.append(buffer);
    }
    if (ret < 0){
        printf("Error: No response to web client request");
        exitWithErr;
    }

    // split the wholeResponse into {header, response}
    vector<string> splitResponse = splitHttpHeaderAndResponse(wholeResponse);

    /* close & exit */
    close (sd);

    return splitResponse;
}

vector<string> WebClient::splitHttpHeaderAndResponse(string httpResponse){
    string header = "";
    string response = "";

    // locate the first '\r\n\r\n' --> this separates the header from the body
    int separatingPosition = httpResponse.find("\r\n\r\n");

    // grep for the positions of header or response
    if (separatingPosition != FUNCTION_ERROR_RETURN_VALUE){
        header = httpResponse.substr(COUNTER_INITIAL_VALUE, separatingPosition);
        response = httpResponse.substr(separatingPosition + strlen("\r\n\r\n")); 
    }
    else{
        header = httpResponse;
    }

    // pack the result into a vector and send over the vector
    vector<string> splitResult = {header, response};
    return splitResult;
}

void WebClient::handleHttpRSPStatus(string header){
    int headerPrecodeSize = strlen("HTTP/1.1 ");
    int headerCodeSize = strlen("###");
    string responseCode = header.substr(headerPrecodeSize, headerPrecodeSize + headerCodeSize + OFF_BY_ONE_OFFSET);
    int code = stoi(responseCode);

    switch(code){
        case CODE_OK:
            break;
        case CODE_MOVED_PERMANENTLY:
            printf("Error: Moved permanently, unable to access requested location\n");
            exitWithErr;
            break;
        case CODE_BAD_REQUEST:
            printf("Error: Bad request, please re-enter a valid request to the server\n");
            exitWithErr;
            break;
        case CODE_NOT_FOUND:
            printf("Error: Requested document not found on server\n");
            exitWithErr;
            break;
        case CODE_HTTP_VERSION_NOT_SUPPORTED:
            printf("Error: HTTP version is not supported\n");
            exitWithErr;
            break;
        default:
            printf("Bad error code, please try again");
            exitWithErr;
            break;
    }
}

string WebClient::appendAtNewline(string toAppend, string stringToConsider){
    string totalString = "";
    string remainingString = stringToConsider;
    string newlineSeparator = "\r\n";

    while (true){
        int newLinePosition = remainingString.find(newlineSeparator.c_str());
        if (newLinePosition <= 0){
            break;
        }
        string upToNewLine = remainingString.substr(COUNTER_INITIAL_VALUE, newLinePosition + strlen(newlineSeparator.c_str()));
        remainingString = remainingString.substr(newLinePosition + strlen(newlineSeparator.c_str()));
        
        totalString.append(toAppend);
        totalString.append(upToNewLine);
    }
    return totalString;
}