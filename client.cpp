#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <string.h>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <istream>
#include <iterator>
#include <string>
#include <vector>
#include <streambuf>
#include <arpa/inet.h>
#include <nlohmann/json.hpp>
//#include "json/json.h"
using json = nlohmann::json;

int create_socket(int,char *);

#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

#define MAXLINE 1048576 /*max text line length*/

int check_recv_status(int sockfd)
{
    struct sockaddr_in servaddr;
    char sendline[MAXLINE], recvline[MAXLINE];

	char buffer[MAXLINE];
	int bytes;
//	Json::StreamWriterBuilder builder_writer;

	bytes = recv(sockfd, buffer, sizeof(buffer), 0);
	std::string recv_str = std::string(buffer, bytes);
	const auto rawJsonLength = static_cast<int>(recv_str.length());
	constexpr bool shouldUseOldWay = false;
//	JSONCPP_STRING err;
//	Json::Value root;

//	Json::CharReaderBuilder builder;
//	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	std::cout << recv_str << std::endl;
    json root = json::parse(recv_str);
//	reader->parse(recv_str.c_str(), recv_str.c_str() + rawJsonLength, &root, &err);

	const std::string status = root["status"];
	std::cout << "status :" << status << std::endl;
	return 0;
}

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr;
    char sendline[MAXLINE], recvline[MAXLINE];

	char buffer[MAXLINE];
	int bytes;
//	Json::StreamWriterBuilder builder_writer;

    //basic check of the arguments
    //additional checks can be inserted
    if (argc !=3) {
        std::cerr<<"Usage: ./a.out <IP address of the server> <port number>"<<std::endl;
        exit(1);
    }

    //Create a socket for the client
    //If sockfd<0 there was an error in the creation of the socket
    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
        std::cerr<<"Problem in creating the socket"<<std::endl;
        exit(2);
    }

    //Creation of the socket
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr= inet_addr(argv[1]);
    servaddr.sin_port =  htons(atoi(argv[2])); //convert to big-endian order

    //Connection of the client to the socket
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
        std::cerr<<"Problem in connecting to the server"<<std::endl;
        exit(3);
    }

    std::string command;
    std::string filename;

    std::cout<<"ftp>";
    std::string line;
    
    while (std::getline(std::cin, line)) {
    
        json root;
        root["ID"] = "";
        root["data"] = "";
        root["status"] = "";
        root["command"] = "";	

        std::istringstream iss(line);
        std::vector<std::string> parsed ((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

        std::string command(parsed[0]);

        std::cout << command.c_str() << std::endl;

        if (!command.compare("quit"))  {
            break;;
        }
        else {
                char buffer[MAXLINE];
                std::transform(command.begin(), command.end(), command.begin(), ::toupper);
                root["command"] = command;

                std::string filename = parsed[1];

                // only for c++17 and up 
                // std::string filename = std::filesystem::path(filePath).filename().string();

                size_t pos = filename.rfind("/");
                if (pos != std::string::npos)
                     root["ID"] = filename.substr(++pos);

            if (!command.compare("PUT") || !command.compare("UPDATE"))  {
                
                std::ifstream f(filename.c_str());
                root["data"] = "";

                if (f.good())
                {
                    std::string  str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                    root["data"] = str;
                }
                else {
                    std::cout << "No such file: " << filename.c_str() << std::endl;
                    continue;
                }
                
                send(sockfd, root.dump().c_str(), root.dump().size()+1 ,0);
                f.close();

                //recieving response
                recv(sockfd, buffer, MAXLINE,0);

                //sending empty data to close the connection
                json root = json::parse(buffer);
                const std::string status = root["status"].get<std::string>();
                std::cout << status.c_str() << std::endl;
                root["data"] = "";
                send(sockfd, root.dump().c_str(), root.dump().size()+1 ,0);

            } else if (!command.compare("GET"))  {

                send(sockfd, root.dump().c_str(), root.dump().size()+1 ,0);

                //recieving response
                recv(sockfd, buffer, MAXLINE,0);

                json root = json::parse(buffer);
                std::string status = root["status"].get<std::string>();
                std::cout << status.c_str() << std::endl;

                std::ofstream f(filename.c_str());
                if (f.good())
                {
                    std::ostreambuf_iterator<char> osbuf(f) ;
                    osbuf._M_put(root["data"].get<std::string>().c_str(), root["data"].get<std::string>().size());
                }

                //std::cout << root.dump().c_str() << std::endl;
                f.close();

                //sending empty data to close the connection
                root = json::parse(buffer);
                status = root["status"].get<std::string>();
                std::cout << status.c_str() << std::endl;
                root["data"] = "";
                send(sockfd, root.dump().c_str(), root.dump().size()+1 ,0);

                std::cout<<"File download done."<<std::endl;
            }
            else {
                std::cerr<<"Error in command. Check Command"<<std::endl;
            }
        }
        std::cout<<"ftp>";
    }

    exit(0);
}


int create_socket(int port,char *addr)
{
    int sockfd;
    struct sockaddr_in servaddr;


    //Create a socket for the client
    //If sockfd<0 there was an error in the creation of the socket
    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
        std::cerr<<"Problem in creating the socket"<<std::endl;
        exit(2);
    }

    //Creation of the socket
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr= inet_addr(addr);
    servaddr.sin_port =  htons(port); //convert to big-endian order

    //Connection of the client to the socket
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
        std::cerr<<"Problem in creating data channel"<<std::endl;
        exit(3);
    }

    return(sockfd);
}
