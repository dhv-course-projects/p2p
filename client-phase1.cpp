#include<bits/stdc++.h>
#include<dirent.h>
#include<netdb.h>
#include<arpa/inet.h>
#include <errno.h>
#include<netinet/in.h>
#include<unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include<thread>
using namespace std;

//----------------------------------MACROS----------------------------------
#define MAX_QUEUE 20
#define MAXDATASIZE 100

//----------------------------------Client----------------------------------
void client(int NEIGHBOR_COUNT, vector< pair<char *, char *> > neighbors){
    
    bool connectedwith[NEIGHBOR_COUNT] = {false};
    bool donewith[NEIGHBOR_COUNT] = {false};

    int connect_fd[NEIGHBOR_COUNT];

    struct addrinfo hints, *res[NEIGHBOR_COUNT];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; //IPV4 OR IPV6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; //MY IP

    for(int i=0; i<NEIGHBOR_COUNT; i++){
        getaddrinfo(NULL, neighbors[i].second, &hints, &(res[i]));
        connect_fd[i] = socket(res[i]->ai_family, res[i]->ai_socktype, res[i]->ai_protocol);
    }

    for(int i=0; i<NEIGHBOR_COUNT; i++){        
        connect(connect_fd[i], res[i]->ai_addr, res[i]->ai_addrlen);
        char buf[MAXDATASIZE];
        int numbytes = recv(connect_fd[i], buf, MAXDATASIZE-1, 0);
            buf[numbytes] = '\0';
            cout<<"Connected to "<<neighbors[i].first<<" with unique-ID "<<buf<<" on port "<<neighbors[i].second<<"\n";
            close(connect_fd[i]);
    }
}

//----------------------------------Server----------------------------------

void server(char *MY_PORT, char *MY_PRIVATE_ID, int NEIGHBOR_COUNT){

    int listen_fd;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; //IPV4 OR IPV6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; //MY IP
    getaddrinfo(NULL, MY_PORT, &hints, &res);
    listen_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    int yes=1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    bind(listen_fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    listen(listen_fd, MAX_QUEUE);
    for(int i = 0; i<NEIGHBOR_COUNT; i++){
        struct sockaddr_storage their_addr;
        socklen_t addr_size = sizeof their_addr;
        int new_fd;

        new_fd = accept(listen_fd, (struct sockaddr *)&their_addr, &addr_size);
        send(new_fd, MY_PRIVATE_ID, strlen(MY_PRIVATE_ID), 0);
        close(new_fd);
    }
}

//--------------------------------------------------------------------
int main(int argc, char *argv[]){

    char *config_file_name = argv[1];
    char *path = argv[2]; //Last character should be '/'

    fstream configfile;
    configfile.open(config_file_name, ios::in);
    
    string x, y, z;
    configfile>>x>>y>>z;
    char *MY_ID = new char[x.length()+1];
    char *MY_PORT = new char[y.length()+1];
    char *MY_PRIVATE_ID = new char[z.length()+1];
    strcpy(MY_ID, x.c_str());
    strcpy(MY_PORT, y.c_str());
    strcpy(MY_PRIVATE_ID, z.c_str());

    int NEIGHBOR_COUNT;
    configfile>>NEIGHBOR_COUNT;
    vector< pair<char *, char *> > neighbors(NEIGHBOR_COUNT);
    for(int i=0; i<NEIGHBOR_COUNT; i++){
        string id, port;
        configfile>>id>>port;
        neighbors[i].first = new char[id.length()+1];
        neighbors[i].second = new char[port.length()+1];
        strcpy(neighbors[i].first, id.c_str());
        strcpy(neighbors[i].second, port.c_str());
    }

    int REQUIRED_FILES_COUNT;
    configfile>>REQUIRED_FILES_COUNT;
    string required_files[REQUIRED_FILES_COUNT];
    for(int i=0; i<REQUIRED_FILES_COUNT; i++){
        configfile>>required_files[i];
    }

    //Step 1: Printing the files
    vector<string> files;
    DIR *dir;
    struct dirent *file;
    if((dir = opendir(path)) != nullptr){
        while((file = readdir(dir)) != nullptr){
            if(file->d_type == DT_REG){
            //    cout<<file->d_name<<"\n";
                // string x
                files.push_back((string)file->d_name);
            }
        }
    }
    else{
        perror("opendir failed\n");
        exit(1);
    }
    // sort(files.begin(), files.end(), compare_string);
    sort(files.begin(), files.end());
    for(string x: files) cout<<x<<"\n";

    //----------------------------------Exit if there are no neighbors----------------------------------
    if(NEIGHBOR_COUNT == 0){
        return 0;
    }

    thread server_thread(server, MY_PORT, MY_PRIVATE_ID, NEIGHBOR_COUNT);
    sleep(1);
    thread client_thread(client, NEIGHBOR_COUNT, neighbors);

    client_thread.join();
    server_thread.join();

    return 0;
}