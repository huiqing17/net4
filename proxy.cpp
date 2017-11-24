#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>
#include <limits.h>

#include "header.h"
using namespace std;



//fd_set to store the socket descriptors of the active connections with the server.
fd_set clientDescriptors;
fd_set allDescriptors;
int maxNumOfClients=3;
char* format = "%a, %d %b %Y %H:%M:%S %Z";
map<string, cache*> cacheEntry;

string getURL(char * recvbuffer){
	string URL;
	char* token = strtok(recvbuffer," ");
	char* tokens[3];
	int i=0;  
    while(token!=NULL)  
    {  
         tokens[i]=token; 
         token=strtok(NULL," "); 
         i++; 
    }
    return tokens[1];
}

cache* parseResponse(string response){
	header* header=new header;
	cache* cache=new cache;
	string delim="\r\n\r\n"; //split header and body
    vector<string> recv = parseString(response, delim); 

  if(recv.size() == 1 || recv.size() > 2)
  {
    cout<<"Reponse error. Exiting..."<<endl;
    exit(1);
  }
  //parse Header
  string head = recv.at(0);
  string delim2 = "\r\n";
  vector<string> headercontent = parseString(head, delim2);
  if(headercontent.size() == 0)
  {
    cout<<"Header error. Exiting..."<<endl;
    exit(1);
  }

  int size = headercontent.size();
  vector<string> contents;
  string delim3 = ": ";
  for(int i=1; i<size; i++) 
  {
    contents = parseString(headercontent.at(i), delim3);

    if(contents.at(0)=="Date") header->date=contents.at(1);
    else if(contents.at(0) == "Expires")
      header->expires=contents.at(1);
    else if(contents.at(0) == "Last-Modified")
      header->lastModified=contents.at(1);

  }

  //get current time
  
  time_t tt=time(NULL);
  struct tm* time;
  time=gmtime(&tt);
  time_t current = timegm(time); 
  header->lastAccessed=current;
  
  //add in the cache
  cache->header=header;
  cache->body=response;

  return cache;
}
vector <string> parseString(string str, string delim){
	vector<string> tokens;
	int i=0;
	int length=str.length();
	size_t pos=str.find(delim,i);
	while(i<length){
		if(pos==string::npos){//end of string
			tokens.push_back(str.substr(i));
			break;

		}
		tokens.push_back(str.substr(i,int(pos)-i));
		i=int(pos)+delim.length();
	}
	return tokens;
}
cache* stamp(cache* newpage){//stamp(cache is not full)
	header* header=newpage->header;
	time_t tt=time(NULL);
    struct tm* time;
    time=gmtime(&tt);
    time_t current = timegm(time); 
    header->lastAccessed=current;
	newpage->header=header;

	return newpage;
}
string receiveFromWebServer(string URL){
	string server,filename;
	char errorbuffer[256],recvbuffer[1000000];
	int point1,point2;
	point1=URL.find("/");
	//input http://or https://
	if(URL.at(point1)=='/' && (URL.at(point1+1)=='/')){
		point2=URL.substr(point1+2,URL.length()-1).find("/");
		server=URL.substr(point1,point2);
		filename=URL.substr(point1+point2+2,URL.length()-1);
	}
	else{//input no //
		server=URL.substr(0,point1);
		filename=URL.substr(point1,URL.length()-1);
	}
	//retrieve filename from webserver
	int httpsd;
	if((httpsd)=socket(AF_INET,SOCK_STREAM,0)==-1){
		fprintf(stderr, "%s\n", "HTTP socket creation error");
		char* errorMessage=strerror_r(errno,errorbuffer,256);
		exit(1);
	}

	struct addrinfo *httpinfo;
	struct addrinfo hints;
	memset(&hints,0,sizeof(hints));
	hints.ai_family=AF_INET;
	hints.ai_socktype=SOCK_STREAM;
	if(getaddrinfo(server.c_str(),"http",&hints,&httpinfo)!=0){
		fprintf(stderr,"getaddrinfo error\n");
		char* errorMessage=strerror_r(errno,errorbuffer,256);
		exit(1);
	}
	if(connect(httpsd,httpinfo->ai_addr,httpinfo->ai_addrlen)==-1){
		fprintf(stderr,"connect error\n");
		char* errorMessage=strerror_r(errno,errorbuffer,256);
		exit(1);
	}
	string message="GET"+URL+"HTTP/1.0\r\n\r\n";
	cout<<message<<endl;
	int bytes=0;
	if((bytes=send(httpsd,message.c_str(),message.length(),0))<=0){
		fprintf(stderr,"send error\n");
		char* errorMessage=strerror_r(errno,errorbuffer,256);
		exit(1);		
	}
	int bytes2=0;
	if((bytes2=recv(httpsd,recvbuffer,1000000,0))<=0){
		fprintf(stderr,"recv error\n");
		char* errorMessage=strerror_r(errno,errorbuffer,256);
		exit(1);		
	}
	string str;
	strncpy(str,recvbuffer,bytes2);
	return str;
}

string receiveFromWebServerModified(string URL,string time){
	string server,filename;
	char errorbuffer[256],recvbuffer[1000000];
	int point1,point2;
	point1=URL.find("/");
	//input http://or https://
	if(URL.at(point1)=='/'&&(URL.at(point1+1)=='/')){
		point2=URL.substr(point1+2,URL.length()-1).find("/");
		server=URL.substr(point1,point2);
		filename=URL.substr(point1+point2+2,URL.length()-1);
	}
	else{//input no //
		server=URL.substr(0,point1);
		filename=URL.substr(point1,URL.length()-1);
	}
	//retrieve filename from webserver
	int httpsd;
	if((httpsd)=socket(AF_INET,SOCK_STREAM,0)==-1){
		fprintf(stderr, "%s\n", "HTTP socket creation error");
		char* errorMessage=strerror_r(errno,errorbuffer,256);
		exit(1);
	}

	struct addrinfo *httpinfo;
	struct addrinfo hints;
	memset(&hints,0,sizeof(hints));
	hints.ai_family=AF_INET;
	hints.ai_socktype=SOCK_STREAM;
	if(getaddrinfo(server.c_str(),"http",&hints,&httpinfo)!=0){
		fprintf(stderr,"getaddrinfo error\n");
		char* errorMessage=strerror_r(errno,errorbuffer,256);
		exit(1);
	}
	if(connect(httpsd,httpinfo->ai_addr,httpinfo->ai_addrlen)==-1){
		fprintf(stderr,"connect error\n");
		char* errorMessage=strerror_r(errno,errorbuffer,256);
		exit(1);
	}
	string message="GET"+URL+"HTTP/1.0\r\n\r\n";
	cout<<message<<endl;
	int bytes=0;
	if((bytes=send(httpsd,message.c_str(),message.length(),0))<=0){
		fprintf(stderr,"send error\n");
		char* errorMessage=strerror_r(errno,errorbuffer,256);
		exit(1);		
	}
	int bytes2=0;
	if((bytes2=recv(httpsd,recvbuffer,1000000,0))<=0){
		fprintf(stderr,"recv error\n");
		char* errorMessage=strerror_r(errno,errorbuffer,256);
		exit(1);		
	}
	char* str;
	strncpy(str,recvbuffer,bytes2);
	char* temp=str;
	vector<string> contents=parseString(temp,"\r\n");
	string status=contents.at(0);
	vector<string> modify=parseString(status," ");
	if(modify.at(1)!="304")return str;
	else return 0;

}
void cacheInsert(string URL,cache* newpage){
	if(cacheEntry.size()<maxCacheSize){
		newpage=stamp(newpage);//find invalid cache entry
		cacheEntry.insert(make_pair(URL,newpage));
	}
	else{//cache is full, find a victim
		//int i=0;
		time_t tt=time(NULL);
    	struct tm* time;
    	time=gmtime(&tt);
    	time_t current = timegm(time);
    	map<string,cache*>::iterator victim;
    	double timediff,maxTime=INT_MIN;

    	map<string,cache*>::iterator vic=cacheEntry.begin();
    	for(;vic!=cacheEntry.end();vic++){
    		cache* newpage=vic->second;
    		time_t lastaccessed=newpage->header->lastAccessed;
    		if(timediff>maxTime){
    			maxTime=timediff;
    			victim=vic;
    		}

    	}

    	cacheEntry.erase(victim);
    	cacheEntry.insert(make_pair(URL,newpage));

	}

}
void cacheUpdate(string URL, cache* newpage){

}
cache* cacheMiss(string URL){
	cout<<"cache miss..."<<endl;
	string page=receiveFromWebServer(URL);
	cache* newpage=parseResponse(URL);
	cacheInsert(URL,newpage);

	return newpage;
}
cache* cacheHit(string URL){
	map<string,cache*>::iterator hit=cacheEntry.find(URL);
	cache* entry=hit->second;

	string expire=entry->header->expires;
	if(expire.empty()){
		expire=entry->header->lastModified;
		if(expire.empty()){
			expire=entry->header->date;
		}
	}
   struct tm tm1;
   char* ret = strptime(expire.c_str(), format, &tm1);
  if(ret == NULL)
  {
    cout<<"The string "<<expire<<" was not recognized as a date format"<<endl;
    exit(1);
  }
  time_t t1 = timegm(&tm1);
  int ex;
  if(t1<=0)ex=-1;
  else{
  	ex=int(t1);
  }
	time_t tt=time(NULL);
    struct tm* time;
    time=gmtime(&tt);
    time_t current = timegm(time);
    int curr=int(current);
    if(current>ex){
    	string modifypage=receiveFromWebServerModified(URL,expire);
    	if(modifypage.empty()){
    		entry=stamp(entry);
    		return entry;
    	}else{
    		cache* newpage=parseResponse(modifypage);
    		cacheEntry.insert(make_pair(URL,newpage));
    		return newpage;

    	}
    }else{
    	entry=stamp(entry);
    	return entry;
    }

}

int main(int argc, char *argv[]) {

	int sockId, newSocketId, port, maxDescriptors;
	char *proxyIP;
	int lenOfSockaddr_in;
	struct hostent *host;
	int selectVal;
	int numOfClients = 0;
	char errorbuffer[256];


	//check arguments
	if (argc != 3) {
        fprintf(stderr,"missing arguments, please use the form ./proxy <ip to bind> <port to bind>\n");
        exit(1);
    }

	//Creating a TCP socket for the server.
	sockId = socket(AF_INET, SOCK_STREAM, 0);
	if(sockId == -1) {
		fprintf(stderr,"Fail to create socket\n");
   		char* errorMessage = strerror_r(errno, errorbuffer, 256);
		exit(1);
	}
	printf("INFO: Socket created successfully\n");

	proxyIP = argv[1];
	port = atoi(argv[2]);

	//client lists
	//struct clientDetails clientList[maxNumOfClients];

	//Initializing the serverSockAddr structure
	struct sockaddr_in serverSockAddr,clientSockAddr;;
	bzero(&serverSockAddr, sizeof(serverSockAddr));
	serverSockAddr.sin_family = AF_INET;
	serverSockAddr.sin_port = htons(port);
	host = gethostbyname(proxyIP);
	memcpy(&serverSockAddr.sin_addr.s_addr, host->h_addr, host->h_length);
	lenOfSockaddr_in = sizeof(clientSockAddr);

	//Binding the created socket to the IP address and port number provided
	if (bind(sockId, (struct sockaddr *) &serverSockAddr, sizeof(serverSockAddr)) == -1) {
		printf("ERROR: Unable to bind the socket to IPv4 = %s and Port = %d\n", proxyIP, port);
		return -1;
	}

	printf("INFO: Binding successfully\n");

	//Listening on the socket with the maximum clients provided
	if(listen(sockId, maxNumOfClients) == -1) {
		printf("ERROR: Fail to listen because the queue is full\n");
		return -1;
	}

	printf("INFO: Listenning at IP: %s, Port: %d...\n", proxyIP, port);

	//clear the sets
	FD_ZERO(&clientDescriptors);
	FD_ZERO(&allDescriptors);

	FD_SET(sockId, &allDescriptors);
	maxDescriptors = sockId;
	printf("the # of descriptors are: %d\n", maxDescriptors);
	char copybuffer[1024],recvbuffer[1024];
	int i;
	while(1){
		clientDescriptors=allDescriptors;
		memset(copybuffer,0,sizeof(copybuffer));
		memset(recvbuffer,0,sizeof(recvbuffer));
		if(select(sockId+1,&clientDescriptors,NULL,NULL,NULL)==-1){
			fprintf(stderr, "Select error\n");
      		char* errorMessage = strerror_r(errno, errorbuffer, 256);
      		//printf(errorMessage);
      		exit(1);
		}
		//for every 
		for(i=3;i<maxDescriptors;i++){
			if(FD_SET(i,&clientDescriptors)){
				if(i==sockId){
					newSocketId=accept(sockId,(struct sockaddr*)&clientSockAddr,&lenOfSockaddr_in);
					if(newSocketId<0){
						fprintf(stderr, "Select error\n");
      					char* errorMessage = strerror_r(errno, errorbuffer, 256);
      					printf(errorMessage);

					}else{
						FD_SET(newSocketId,&allDescriptors);
						if(newSocketId>maxDescriptors) maxDescriptors=newSocketId;
					}


				}else{
					int re;

					if(re=recv(i,recvbuffer,sizeof(recvbuffer),0)<=0){
						FD_CLR(i,&allDescriptors);
						continue;
					}
					//parse the response
					string URL=getURL(recvbuffer);
					cout<<"You are visiting "<<URL<<endl;
					if(cacheEntry.count(URL) == 0){//cache miss
					cache* page=cacheMiss(URL);
					string body=page->body;
					send(i,body.c_str(),body.length(),0);

					}
					else{//cache hit
					cache* page=cacheHit(URL);
					string body=page->body;
					send(i,body.c_str(),body.length(),0);
                    //print cache

					}

				}
			}
		}
	}

	
	close(sockId);
	return 0;
}