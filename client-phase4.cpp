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
#include<algorithm> // lexicographical_compare
// #include<openssl/md5.h>
using namespace std;

//----------------------------------MACROS----------------------------------
#define MAX_QUEUE 20
#define MAXDATASIZE 1000

//---------------------------------GLOBAL----------------------------
char *MY_ID;
char *MY_PRIVATE_ID;
//("<<MY_ID<<")

//----------------------------------Client----------------------------------
void client(int NEIGHBOR_COUNT, vector< pair<char *, char *> > neighbors, int REQUIRED_FILES_COUNT, vector<char *> required_files, vector<string> files){

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
        while(connect(connect_fd[i], res[i]->ai_addr, res[i]->ai_addrlen) == -1){;}
        
        //Server's private id --phase 1
        char buf[MAXDATASIZE];
        memset(buf, '\0',sizeof(buf));
        recv(connect_fd[i], buf, MAXDATASIZE, 0);
        string server_uniqueid = buf;
        cout<<"Connected to "<<neighbors[i].first<<" with unique-ID "<<buf<<" on port "<<neighbors[i].second<<"\n";
        
        //Sharing the client file's names
        memset(buf, '\0',sizeof(buf));
        strcat(buf, MY_PRIVATE_ID);
        send(connect_fd[i], buf, MAXDATASIZE, 0);

        const char *x = to_string(files.size()).c_str();
        memset(buf,'\0',sizeof(buf));
        strcat(buf,x);
        send(connect_fd[i], buf, MAXDATASIZE, 0);

        for(int j=0; j<files.size(); j++){
            const char *x = files[j].c_str();
            memset(buf,'\0',sizeof(buf));
            strcat(buf,x);
            send(connect_fd[i], buf, MAXDATASIZE, 0);
        }
    }
        // for(int j=0; j<REQUIRED_FILES_COUNT; j++){
        //     char *filename = required_files[j];
        //     memset(buf,'\0',sizeof(buf));
        //     strcat(buf,filename);
        //     send(connect_fd[i], buf, MAXDATASIZE, 0);
        //     memset(buf,'\0',sizeof(buf));
        //     recv(connect_fd[i], buf, MAXDATASIZE, 0);
        //     bool found = (bool) atoi(buf);
        //     if(found){
        //         int y = stoi(server_uniqueid);
        //         if(filefound[j] == 0){
        //             filefound[j] = y;
        //         }
        //         else{
        //             filefound[j] = min(filefound[j], y);
        //         }
        //     }
        // }
    
    vector<pair<int, int> > filefound(REQUIRED_FILES_COUNT, {0, 0});

    for(int i=0; i<NEIGHBOR_COUNT; i++){
        char buf[MAXDATASIZE];
        const char *x = to_string(REQUIRED_FILES_COUNT).c_str();
        memset(buf,'\0',sizeof(buf));
        strcat(buf,x);
        send(connect_fd[i], buf, MAXDATASIZE, 0);

        for(int j=0; j<REQUIRED_FILES_COUNT; j++){
            memset(buf,'\0',sizeof(buf));
            strcat(buf, required_files[j]);
            send(connect_fd[i], buf, MAXDATASIZE, 0);

            memset(buf,'\0',sizeof(buf));
            recv(connect_fd[i], buf, MAXDATASIZE, 0);
            int prv_id = atoi(buf);

            memset(buf,'\0',sizeof(buf));
            recv(connect_fd[i], buf, MAXDATASIZE, 0);
            int depth = atoi(buf);

            if(prv_id != 0){
                if(filefound[j].first == 0){
                    filefound[j].first = prv_id;
                    filefound[j].second = depth;
                }
                else{
                    if(depth < filefound[j].second){
                        filefound[j].first = prv_id;
                        filefound[j].second = depth;
                    }
                    else if(depth == filefound[j].second){
                        filefound[j].first = min(filefound[j].first, prv_id);
                    }
                }
            }
        }

        close(connect_fd[i]);
    }

    // for(int j=0; j<REQUIRED_FILES_COUNT; j++){
    //     int depth;
    //     if(filefound[j] == 0) depth = 0;
    //     else depth = 1;
    //     cout<<"Found "<<required_files[j]<<" at "<<filefound[j]<<" with MD5 0 at depth "<<depth<<"\n";
    // }
    for(int i=0; i<REQUIRED_FILES_COUNT; i++){
        cout<<"Found "<<required_files[i]<<" at "<<filefound[i].first<<" with MD5 0 at depth "<<filefound[i].second<<endl;
    }
}

//----------------------------------Server----------------------------------
void filequery(int fd, vector<string> myfiles, vector< pair<int, string> > d2files){
    char buf[MAXDATASIZE];
    memset(buf,'\0',sizeof(buf));
    recv(fd, buf, MAXDATASIZE, 0);
    int num_queries = atoi(buf);

    for(int i=0; i<num_queries; i++){
        char buf[MAXDATASIZE];
        memset(buf,'\0',sizeof(buf));
        recv(fd, buf, MAXDATASIZE, 0);
        string filename = buf;

        int send_prv_id = 0;
        int send_depth = 0;

        for(string y: myfiles){
            if(y == filename){
                send_prv_id = atoi(MY_PRIVATE_ID);
                send_depth = 1;
                break;
            }
        }
        if(send_prv_id == 0){
            for(auto z: d2files){
                if(z.second == filename){
                    send_prv_id = z.first;
                    send_depth = 2;
                    break;
                }
            }
        }

        memset(buf,'\0',sizeof(buf));
        strcat(buf, to_string(send_prv_id).c_str());
        send(fd, buf, MAXDATASIZE, 0);

        memset(buf,'\0',sizeof(buf));
        strcat(buf, to_string(send_depth).c_str());
        send(fd, buf, MAXDATASIZE, 0);
    }

    close(fd);
}

void server(char *MY_PORT, char *MY_PRIVATE_ID, int NEIGHBOR_COUNT, vector<string> myfiles){

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

    int new_fd[NEIGHBOR_COUNT];
    vector< pair<int, string> > d2files;

    for(int i = 0; i<NEIGHBOR_COUNT; i++){
        struct sockaddr_storage their_addr;
        socklen_t addr_size = sizeof their_addr;
        new_fd[i] = accept(listen_fd, (struct sockaddr *)&their_addr, &addr_size);

        char buf[MAXDATASIZE];
        memset(buf,'\0',sizeof(buf));
        strcat(buf, MY_PRIVATE_ID);
        send(new_fd[i], buf, MAXDATASIZE, 0);

        memset(buf,'\0',sizeof(buf));
        recv(new_fd[i], buf, MAXDATASIZE, 0);
        int CLIENT_PRIVATE_ID = atoi(buf);

        memset(buf,'\0',sizeof(buf));
        recv(new_fd[i], buf, MAXDATASIZE, 0); // the received string in buffer is '\0' appended
        int sender_files_count = atoi(buf);

        for(int j=0; j<sender_files_count; j++){
            int depth = 2;
            memset(buf,'\0',sizeof(buf));
            recv(new_fd[i], buf, MAXDATASIZE, 0);
            string filename = buf;
            d2files.push_back({CLIENT_PRIVATE_ID, filename});
        }

        // close(new_fd[i]);

            // int REQUIRED_FILES_COUNT = atoi(buf);
            // for(int j=0; j<REQUIRED_FILES_COUNT; j++){
            //     memset(buf,'\0',sizeof(buf));
            //     recv(new_fd, buf, MAXDATASIZE, 0);
            //     string filename = buf;
            //     bool found = false;
            //     for(string f: files){
            //         if(filename == f){
            //             found = true;
            //             break;
            //         }
            //     }
            //     const char *x = to_string(found).c_str();
            //     memset(buf,'\0',sizeof(buf));
            //     strcat(buf,x);
            //     send(new_fd, buf, MAXDATASIZE, 0);            
            // }
    }

    sort(d2files.begin(), d2files.end());

    //Threading, close the fd at the end of the thread function
    vector< thread > file_query_threads;
    //file_transfer_threads.reserve(NEIGHBOR_COUNT);
    for (int i=0; i<NEIGHBOR_COUNT; i++){
        file_query_threads.push_back(thread(filequery,new_fd[i], myfiles, d2files));
    }
    for (auto& t : file_query_threads) t.join();
}

bool compare_char (char c1, char c2)
{
    return tolower(c1)<tolower(c2);
}

bool compare_string(string s1,string s2){
    return lexicographical_compare(s1.begin(),s1.end(),s2.begin(),s2.end(),compare_char);
}

bool comparisonFunc(const char *c1, const char *c2)
{
    return strcmp(c1, c2) < 0;
}


//--------------------------------------------------------------------
int main(int argc, char *argv[]){

    char *config_file_name = argv[1];
    char *path = argv[2]; //Last character should be '/'

    fstream configfile;
    configfile.open(config_file_name, ios::in);
    
    string x, y, z;
    configfile>>x>>y>>z;
    MY_ID = new char[x.length()+1];
    char *MY_PORT = new char[y.length()+1];
    MY_PRIVATE_ID = new char[z.length()+1];
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
    vector<char *> required_files(REQUIRED_FILES_COUNT);
    for(int i=0; i<REQUIRED_FILES_COUNT; i++){
        string filename;
        configfile>>filename;
        required_files[i] = new char[filename.length()+1];
        strcpy(required_files[i], filename.c_str());
    }
    sort(required_files.begin(), required_files.end(), comparisonFunc);

    //Step 1: Printing the files
    vector<string> files;
    DIR *dir;
    struct dirent *file;
    if((dir = opendir(path)) != nullptr){
        while((file = readdir(dir)) != nullptr){
            if(file->d_type == DT_REG){
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


    thread server_thread(server, MY_PORT, MY_PRIVATE_ID, NEIGHBOR_COUNT, files);
    thread client_thread(client, NEIGHBOR_COUNT, neighbors, REQUIRED_FILES_COUNT, required_files, files);
    client_thread.join();
    server_thread.join();

    return 0;
}
