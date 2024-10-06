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

        void handleCLIArgs();
        int parseArgLineOptionals();
        vector<string> httpGET();
        vector<string> splitHttpHeaderAndResponse(string httpResponse);

        void handleHttpRSPStatus(string header);

    public:
        WebClient(int argLine, string url, string inputSavePath);
        ~WebClient() = default;

        void Exec();
};

#endif // WEBCLIENT_H