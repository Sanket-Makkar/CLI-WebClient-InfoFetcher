#ifndef WEBCLIENT_H
#define WEBCLIENT_H

#include <string>
#include <vector>
using namespace std;

class WebClient{
    private:
        int argLine;
        string url;
        string savePath;    
        string host = "";
        string path = "";
        string getRequest = "";

        string headerIn = "";
        vector<unsigned char> responseIn;

        void handleCLIArgs(string header, string request);
        int parseArgLineOptionals();
        void httpGET(string request);
        vector<string> splitHttpHeaderAndResponse(string httpResponse);

        void handleHttpRSPStatus(string header);
        string appendAtNewline(string toAppend, string stringToConsider);

    public:
        WebClient(int argLine, string url, string inputSavePath);
        ~WebClient() = default;

        void Exec();
};

#endif // WEBCLIENT_H