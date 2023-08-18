#include<bits/stdc++.h>
#include<dirent.h>
#include<netdb.h>
#include<arpa/inet.h>
#include <errno.h>
#include<netinet/in.h>
#include<unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include<thread>
#include<algorithm> // lexicographical_compare
#include <openssl/md5.h>
// #include<openssl/md5.h>
using namespace std;

//----------------------------------MACROS----------------------------------
#define MAX_QUEUE 20
#define MAXDATASIZE 1000

//---------------------------------GLOBAL----------------------------
char *MY_ID;
//("<<MY_ID<<")

//----------------------------------Client----------------------------------
void client(int NEIGHBOR_COUNT, vector< pair<char *, char *> > neighbors, int REQUIRED_FILES_COUNT, vector<char *> required_files, char *path){

    int connect_fd[NEIGHBOR_COUNT];

    struct addrinfo hints, *res[NEIGHBOR_COUNT];

    vector<int> filefound(REQUIRED_FILES_COUNT, 0);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; //IPV4 OR IPV6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; //MY IP

    for(int i=0; i<NEIGHBOR_COUNT; i++){
        getaddrinfo(NULL, neighbors[i].second, &hints, &(res[i]));
        connect_fd[i] = socket(res[i]->ai_family, res[i]->ai_socktype, res[i]->ai_protocol);
    }

    vector<int> private_ids(NEIGHBOR_COUNT);

    for(int i=0; i<NEIGHBOR_COUNT; i++){
        while(connect(connect_fd[i], res[i]->ai_addr, res[i]->ai_addrlen) == -1){;}

        char buf[MAXDATASIZE];
        
        memset(buf, '\0',sizeof(buf));
        recv(connect_fd[i], buf, MAXDATASIZE, 0);
        
        string server_uniqueid = buf;
        private_ids[i] = stoi(server_uniqueid);

        cout<<"Connected to "<<neighbors[i].first<<" with unique-ID "<<buf<<" on port "<<neighbors[i].second<<endl;
        
        memset(buf, '\0', sizeof(buf));
        strcat(buf,MY_ID);
        send(connect_fd[i], buf, MAXDATASIZE, 0);
        
        const char *x = to_string(REQUIRED_FILES_COUNT).c_str();

        memset(buf,'\0',sizeof(buf));
        strcat(buf,x);
        send(connect_fd[i], buf, MAXDATASIZE, 0);

        for(int j=0; j<REQUIRED_FILES_COUNT; j++){
            char *filename = required_files[j];
        
            memset(buf,'\0',sizeof(buf));
            strcat(buf,filename);
            send(connect_fd[i], buf, MAXDATASIZE, 0);
        
            memset(buf,'\0',sizeof(buf));
            recv(connect_fd[i], buf, MAXDATASIZE, 0);
        
            bool found = (bool) atoi(buf);
            if(found){
                int y = stoi(server_uniqueid);
                if(filefound[j] == 0){
                    filefound[j] = y;
                }
                else{
                    filefound[j] = min(filefound[j], y);
                }
            }
        }
        //close(connect_fd[i]);
    }
    
    // for(int j=0; j<REQUIRED_FILES_COUNT; j++){
    //     int depth;
    //     if(filefound[j] == 0) depth = 0;
    //     else depth = 1;
    //     cout<<"Found "<<required_files[j]<<" at "<<filefound[j]<<" with MD5 0 at depth "<<depth<<endl;
    // }
    
    char buf[MAXDATASIZE];
    memset(buf,'\0', sizeof(buf));
    strcat(buf, path);
    strcat(buf, "Downloaded");
    mkdir(buf,0777);

    // finding the files we want from each neighbour
    for(int i=0; i<NEIGHBOR_COUNT; i++){
        vector<string> files_needed;    // from i-th neighbour
        for(int j=0; j<REQUIRED_FILES_COUNT; j++){
            if(filefound[j] == private_ids[i]){
                string filename = required_files[j];
                files_needed.push_back(filename);
            }
        }
        int numfiles = files_needed.size();
        
        memset(buf, '\0', sizeof(buf));
        strcat(buf, to_string(numfiles).c_str());
        send(connect_fd[i], buf, MAXDATASIZE, 0);   // sending numfiles

        for (int j=0; j<numfiles; j++){
            memset(buf,'\0',sizeof(buf));
            strcat(buf, files_needed[j].c_str());
            send(connect_fd[i], buf, MAXDATASIZE, 0);   // send filename

            memset(buf,'\0',sizeof(buf));
            recv(connect_fd[i], buf, MAXDATASIZE, 0);   //receive filesize

            int filesize = atoi(buf);
            
            memset(buf, '\0', sizeof(buf));
            strcat(buf,path);
            strcat(buf,"Downloaded/");
            strcat(buf,files_needed[j].c_str());
            FILE *fileptr = fopen(buf,"wb+");
            if (fileptr == NULL) {
                printf("File open error client");
                return;
            }

            int received_bytes = 0;
            while(received_bytes < filesize){
                memset(buf, '\0', sizeof(buf));
                int received = recv(connect_fd[i], buf, MAXDATASIZE, 0);
                received_bytes += received;
                fwrite(buf,sizeof(char),received,fileptr);
            }
            fclose(fileptr);
        }
    }

    for(int j=0; j<REQUIRED_FILES_COUNT; j++){
        int depth;
        if(filefound[j] == 0) {
            depth = 0;
            cout <<"Found "<<required_files[j]<<" at "<<filefound[j]<<" with MD5 0 at depth "<<depth<<endl;
        }
        else {
            depth = 1;
        
            memset(buf, '\0', sizeof(buf));
            strcat(buf, path);
            strcat(buf, "Downloaded/");
            strcat(buf, required_files[j]);
            FILE *fileptr = fopen(buf, "rb");
            if (fileptr == NULL) {
                std::cout <<"File open error\n";
            }

            fseek(fileptr, 0L, SEEK_END);
            long int filesize = ftell(fileptr);
            fseek(fileptr, 0L, SEEK_SET);

            unsigned char file_read_buffer[filesize];
            unsigned char md5_string[MD5_DIGEST_LENGTH];
            memset(file_read_buffer, '\0', sizeof(file_read_buffer));
            filesize = fread(file_read_buffer, sizeof(char), filesize, fileptr);
            MD5(file_read_buffer, filesize, md5_string);

            cout <<"Found "<<required_files[j]<<" at "<<filefound[j]<<" with MD5 ";
            for(int i=0; i<MD5_DIGEST_LENGTH; i++){
                cout << hex << setw(2) << setfill('0') << (int)md5_string[i] << dec;
            }
            cout <<" at depth "<<depth<<endl;
        }
    }
}

//--------------------------Server spawns this thread to serve each client-----
void file_transfer(int fd, int id, vector<string> files, char *path){ // id of the client it serves
    char buf[MAXDATASIZE];
    memset(buf,'\0', sizeof(buf));
    recv(fd,buf, MAXDATASIZE,0);    // number of files client wants
    int numfiles = atoi(buf);

    for(int i=0; i<numfiles; i++){
        memset(buf,'\0',sizeof(buf));
        recv(fd,buf,MAXDATASIZE,0); // receive name of file client wants
        string filename = buf;

        memset(buf, '\0', sizeof(buf));
        strcat(buf,path);
        strcat(buf,filename.c_str());

        FILE *fileptr = fopen(buf,"rb");
        if (fileptr == NULL) {
		    cerr <<"File open error server\n";
	    }
        

        fseek(fileptr, 0L, SEEK_END);
        long int filesize = ftell(fileptr);
        fseek(fileptr, 0L, SEEK_SET);
        
        const char* x = to_string(filesize).c_str();
        memset(buf, '\0', sizeof(buf));
        strcat(buf,x);
        send(fd,buf,MAXDATASIZE,0); //send filesize

        int numbytes;
        memset(buf,'\0',sizeof(buf));
        while((numbytes = fread(buf, sizeof(char), MAXDATASIZE, fileptr))){
            send(fd, buf, numbytes, 0);
            memset(buf, '\0', sizeof(buf));
        }
        fclose(fileptr);
    }
    return;
}


//----------------------------------Server----------------------------------
void server(char *MY_PORT, char *MY_PRIVATE_ID, int NEIGHBOR_COUNT, vector<string> files, char *path){

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

    // cout<<"BINDING"<<endl;
    bind(listen_fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    listen(listen_fd, MAX_QUEUE);

    vector<int> new_fd(NEIGHBOR_COUNT);
    vector<int> client_id(NEIGHBOR_COUNT);

    for(int i = 0; i<NEIGHBOR_COUNT; i++){
        
        struct sockaddr_storage their_addr;
        socklen_t addr_size = sizeof their_addr;
        new_fd[i] = accept(listen_fd, (struct sockaddr *)&their_addr, &addr_size);
        char buf[MAXDATASIZE];
        
        memset(buf,'\0',sizeof(buf));
        strcat(buf,MY_PRIVATE_ID);
        send(new_fd[i], buf, MAXDATASIZE, 0);

        memset(buf, '\0', sizeof(buf));
        recv(new_fd[i],buf, MAXDATASIZE, 0);
        
        client_id[i] = atoi(buf);
        
        memset(buf,'\0',sizeof(buf));
        recv(new_fd[i], buf, MAXDATASIZE, 0); // the received string in buffer is '\0' appended
        
        int REQUIRED_FILES_COUNT = atoi(buf);
        for(int j=0; j<REQUIRED_FILES_COUNT; j++){
            memset(buf,'\0',sizeof(buf));
            recv(new_fd[i], buf, MAXDATASIZE, 0);
        
            string filename = buf;
            bool found = false;
            for(string f: files){
                if(filename == f){
                    found = true;
                    break;
                }
            }
            const char *x = to_string(found).c_str();
        
            memset(buf,'\0',sizeof(buf));
            strcat(buf,x);
            send(new_fd[i], buf, MAXDATASIZE, 0);            
        }
        //close(new_fd[i]);
    }

    vector< thread > file_transfer_threads;
    //file_transfer_threads.reserve(NEIGHBOR_COUNT);
    for (int i=0; i<NEIGHBOR_COUNT; i++){
        file_transfer_threads.push_back(thread(file_transfer,new_fd[i],client_id[i],files,path));
    }
    for (auto& t : file_transfer_threads) t.join();
    

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


    thread server_thread(server, MY_PORT, MY_PRIVATE_ID, NEIGHBOR_COUNT, files, path);
    thread client_thread(client, NEIGHBOR_COUNT, neighbors, REQUIRED_FILES_COUNT, required_files, path);
    client_thread.join();
    server_thread.join();
    
    //cout<<"RETURNING"<<endl;
    return 0;
}