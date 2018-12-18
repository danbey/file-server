#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <string.h>
#include <algorithm>
#include <arpa/inet.h>
#include "json/json.h"

int create_socket(int,char *);

#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

#define MAXLINE 4096 /*max text line length*/

int check_recv_status(int sockfd)
{
    struct sockaddr_in servaddr;
    char sendline[MAXLINE], recvline[MAXLINE];

	char buffer[MAXLINE];
	int bytes;
	Json::StreamWriterBuilder builder_writer;

	bytes = recv(sockfd, buffer, sizeof(buffer), 0);
	std::string recv_str = std::string(buffer, bytes);
	const auto rawJsonLength = static_cast<int>(recv_str.length());
	constexpr bool shouldUseOldWay = false;
	JSONCPP_STRING err;
	Json::Value root;
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	std::cout << recv_str << std::endl;
	reader->parse(recv_str.c_str(), recv_str.c_str() + rawJsonLength, &root, &err);
	const std::string status = root["status"].asString();
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
	Json::StreamWriterBuilder builder_writer;

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

    std::cout<<"ftp>";

    while (fgets(sendline, MAXLINE, stdin) != NULL) {
        Json::Value root;
        root["ID"] = "";
        root["data"] = "";
        root["status"] = "";
        root["command"] = "";	

        char *token,*dummy;
        dummy=sendline;
        token=strtok(dummy," ");
        std::cout << "*****DBY DEBUG func:" << __func__ << " line:" << __LINE__ <<
            " token:" <<  token << std::endl;

        //send(sockfd, sendline, MAXLINE, 0);
        std::string command(token);
        if (strcmp("quit\n",sendline)==0)  {
            //close(sockfd);
            return 0;
        }
        else if (strcmp("put",token)==0 || strcmp("update",token)==0)  {
            char port[MAXLINE], buffer[MAXLINE],char_num_blks[MAXLINE],char_num_last_blk[MAXLINE];
            int data_port,datasock,lSize,num_blks,num_last_blk,i;
            FILE *fp;
            std::locale loc;
            std::transform(command.begin(), command.end(), command.begin(), ::toupper);
            root["command"] = command.c_str();
            Json::StreamWriterBuilder builder;

            token=strtok(NULL," \n");
            root["ID"] = token;

            if ((fp=fopen(token,"r"))!=NULL)
            {
                //std::cout << "DBY DEBUG func:" << __func__ << " line:" << __LINE__ << std::endl;

                //size of file
                //send(sockfd,"1",MAXLINE,0);
                fseek (fp , 0 , SEEK_END);
                lSize = ftell (fp);
                rewind (fp);

                std::cout << "DBY DEBUG func:" << __FUNCTION__ << "line:" << __LINE__ <<
                    " lsize:" << lSize << std::endl;

                num_blks = lSize/MAXLINE;
                num_last_blk = lSize%MAXLINE; 
                //sprintf(char_num_blks,"%d",num_blks);
                std::cout << " *****DBY DEBUG func:" << __FUNCTION__ << "line:" << __LINE__ <<
                    " num_blks:" << num_blks << std::endl;
                //send(sockfd, char_num_blks, MAXLINE, 0);
                //std::cout<<num_blks<<"	"<<num_last_blk<<std::endl;
                for(i= 0; i < num_blks; i++) { 
                    fread (buffer,sizeof(char),MAXLINE,fp);
                    root["data"] = buffer;
                    const std::string jline = Json::writeString(builder, root);
                    std::cout << jline << std::endl;

                    send(sockfd, jline.c_str(), MAXLINE, 0);
					check_recv_status(sockfd);

                }
                //sprintf(char_num_last_blk,"%d",num_last_blk);
                //send(sockfd, char_num_last_blk, MAXLINE, 0);
                if (num_last_blk > 0) {
                    fread (buffer,sizeof(char),num_last_blk,fp);
                    root["data"] = buffer;
                    const std::string jline = Json::writeString(builder, root);
                    std::cout << jline << std::endl;

                    send(sockfd, jline.c_str(), MAXLINE, 0);
					check_recv_status(sockfd);
                //std::cout<<buffer<<std::endl;
                }
                fclose(fp);
                std::cout<<"File upload done.\n";
				//sending empty data means the upload is done
				root["data"] = "";
                const std::string jline = Json::writeString(builder, root);
                send(sockfd, jline.c_str(), MAXLINE, 0);
            }
            else{
                //send(sockfd,"0",MAXLINE,0);
                std::cerr<<"Error in opening file. Check filename\nUsage: put filename"<<std::endl;
            }
        }

        else if (strcmp("get",token)==0)  {
            char port[MAXLINE], buffer[MAXLINE],char_num_blks[MAXLINE],char_num_last_blk[MAXLINE],message[MAXLINE];
            int data_port,datasock,lSize,num_blks,num_last_blk,i;
            FILE *fp;
            recv(sockfd, port, MAXLINE,0);
            data_port=atoi(port);
            datasock=create_socket(data_port,argv[1]);
            token=strtok(NULL," \n");
            recv(sockfd,message,MAXLINE,0);
            if(strcmp("1",message)==0){
                if((fp=fopen(token,"w"))==NULL)
                    std::cout<<"Error in creating file\n";
                else
                {
                    recv(sockfd, char_num_blks, MAXLINE,0);
                    num_blks=atoi(char_num_blks);
                    for(i= 0; i < num_blks; i++) { 
                        recv(datasock, buffer, MAXLINE,0);
                        fwrite(buffer,sizeof(char),MAXLINE,fp);
                        //std::cout<<buffer<<std::endl;
                    }
                    recv(sockfd, char_num_last_blk, MAXLINE,0);
                    num_last_blk=atoi(char_num_last_blk);
                    if (num_last_blk > 0) { 
                        recv(datasock, buffer, MAXLINE,0);
                        fwrite(buffer,sizeof(char),num_last_blk,fp);
                        //std::cout<<buffer<<std::endl;
                    }
                    fclose(fp);
                    std::cout<<"File download done."<<std::endl;
                }
            }
            else{
                std::cerr<<"Error in opening file. Check filename\nUsage: put filename"<<std::endl;
            }
        }
        else{
            std::cerr<<"Error in command. Check Command"<<std::endl;
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
