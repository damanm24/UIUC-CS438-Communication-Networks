/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <vector>
#include <iostream>
#include <string>
#include <fstream>

#include <arpa/inet.h>

using namespace std;

#define PORT "80" // the port client will be connecting to 

#define MAXDATASIZE 1024 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// return ip, port number, and file path
vector<string> parse_url(string& text){
	string url = "";
	string port = "";
	string filepath = "";
	
	text = text.substr(7);
	int len = text.length();
	int i=0;
	while(i<len){
		if(text[i] == ':' || text[i] == '/'){
			if(text[i] == ':'){
				url = text.substr(0,i);
				text = text.substr(i+1);
				int j=0;
				while(text[j] != '/'){
					j++;
				}
				port = text.substr(0,j);
				// cout << text[j] << endl;
				// cout << port << endl;
				filepath = text.substr(j);
			}else{
				url = text.substr(0,i);
				port = "80";
				filepath = text.substr(i);
			}
			break;
		}
		i++;
	}
	vector<string> res{url, port, filepath};
	return res;
}

bool processHeader(string header){
	string status_code = "";
	for(int i=0; i<header.length(); i++){
		if(header[i] == ' '){
			header = header.substr(i+1);
			int j = 0;
			while(header[j] != ' '){
				j++;
			}
			status_code = header.substr(0,j);
			break;
		}
	}
	cout << status_code << endl;
	if(status_code != "200"){
		return false;
	}else{
		return true;
	}
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	string text = std::string(argv[1]);
	vector<string> parsed_url = parse_url(text);
	string request = "GET " + parsed_url[2] + " HTTP/1.1\r\n" \
	+ "User-Agent: Wget/1.15 (linux-gnu)\r\n" + \
	"Host: " + parsed_url[0] + ":" + parsed_url[1] + "\r\n" + \
	"Connection: Keep-Alive\r\n\r\n";
	cout << request << endl;
	// for(int i=0; i<parsed_url.size(); i++){
	// 	cout << parsed_url[i] << endl;
	// }

	const char *requestbuff = request.c_str();
	int len_req = strlen(requestbuff);
	// cout << len_req << endl;
	// cout << request.length() << endl;

	if ((rv = getaddrinfo(parsed_url[0].c_str(), parsed_url[1].c_str(), &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	// send http request get
	send(sockfd, requestbuff, len_req, 0);

	string received = "";
	int count = 0;
	char c;
	int line = 0;

	// filter response header
	while(true){
		recv(sockfd, &c, 1, 0);
		// cout << c;
		if(c == '\n' &&  received[count-1] == '\r'){
			received += c;
			if(line == 0){
				if(processHeader(received) == false){
					break;
				}
			}else if(received.length() == 2 && line > 0){
				// download file
				cout << "start downloading" << endl;
				string outputpath = "./output";
				FILE *file = fopen(outputpath.c_str(), "w");
				while(true){
					numbytes = recv(sockfd, buf, MAXDATASIZE, 0);
					if(numbytes <= 0){
						break;
					}
					fwrite(buf, numbytes, 1, file);
				}
				fclose(file);
				cout << "File received." << endl;
				break;
			}
			cout << received << endl;
			received = "";
			count = 0;
			line++;
		}else{
			received += c;
			count++;
		}
	}

	close(sockfd);

	return 0;
}

