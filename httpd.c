#include "httpd.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#define CONNMAX 10000

static int listenfd, conn;
static void error(char *);
static void startServer(const char *);
static void respond(int);
static void *test_data_logger(void *data);

typedef struct { char *name, *value; } header_t;

static header_t reqhdr[17] = { {"\0", "\0"} };
static int clientfd;
static FILE *log_file;

static char *buf;
pthread_mutex_t count_mutex;
long long count;



void serve_forever(const char *PORT)
{
    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    char c;

    void *in_q = NULL;
    int slot=0;
    pthread_t thread_id;

    printf(
            "Server started %shttp://127.0.0.1:%s%s\n",
            "\033[92m",PORT,"\033[0m"
    );

    /**>startIPS()*/
    
    // Setting all elements to -1: signifies there is no client connected
    int i;
    startServer(PORT);

    /**> getIPSQueue(&in_q)*/

    // Ignore SIGCHLD to avoid zombie threads
    signal(SIGCHLD,SIG_IGN);

    // ACCEPT connections
    while (1)
    {
        addrlen = sizeof(clientaddr);
    	conn = accept (listenfd, (struct sockaddr *) &clientaddr, &addrlen);
    	if (conn < 0 )
	    {
            perror("accept() error");
        }
        else
        {
            if ( fork()==0 )
            {
                respond(conn);
                exit(0);
            }
        }
    }
}

//start server
void startServer(const char *port)
{
    struct addrinfo hints, *res, *p;

    // getaddrinfo for host
    memset (&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo( NULL, port, &hints, &res) != 0)
    {
        perror ("getaddrinfo() error");
        exit(1);
    }
    // socket and bind
    for (p = res; p!=NULL; p=p->ai_next)
    {
        int option = 1;
        listenfd = socket (p->ai_family, p->ai_socktype, 0);
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
        if (listenfd == -1) continue;
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
    }
    if (p==NULL)
    {
        perror ("socket() or bind()");
        exit(1);
    }

    freeaddrinfo(res);

    // listen for incoming connections
    if ( listen (listenfd, 1000000) != 0 )
    {
        perror("listen() error");
        exit(1);
    }
}


// get request header
char *request_header(const char* name)
{
    header_t *h = reqhdr;
    while(h->name) {
        if (strcmp(h->name, name) == 0) return h->value;
        h++;
    }
    return NULL;
}



//client connection
void respond(int conn)
{
    int rcvd, fd, bytes_read;
    char *ptr;

    buf = malloc(65535);
    rcvd=recv(conn, buf, 65535, 0);

    if (rcvd<0)    // receive error
        fprintf(stderr,("recv() error\n"));
    else if (rcvd==0)    // receive socket closed
        fprintf(stderr,"Client disconnected upexpectedly.\n");
    else    // message received
    {
        buf[rcvd] = '\0';

        method = strtok(buf,  " \t\r\n");
        uri    = strtok(NULL, " \t");
        prot   = strtok(NULL, " \t\r\n");

        fprintf(stderr, "\x1b[32m + [%s] %s\x1b[0m\n", method, uri);

        if (qs = strchr(uri, '?'))
        {
            *qs++ = '\0'; //split URI
        } else {
            qs = uri - 1; //use an empty string
        }

        header_t *h = reqhdr;
        char *t, *t2;
        while(h < reqhdr+16) {
            char *k,*v;
            k = strtok(NULL, "\r\n: \t"); if (!k) break;
            v = strtok(NULL, "\r\n");     while(*v && *v==' ') v++;
            h->name  = k;
            h->value = v;
            h++;
            fprintf(stderr, "[H] %s: %s\n", k, v);
            t = v + 1 + strlen(v);
            if (t[1] == '\r' && t[2] == '\n'){
            //    t = strtok(NULL, "\r\n: \t\r\n: \t");
                break;
            }
        }
        t++; // now the *t shall be the beginning of user payload
        t2 = request_header("Content-Length"); // and the related header if there is  
        payload = t;
        payload_size = t2 ? atol(t2) : (rcvd-(t-buf));
	
        fprintf(stderr, "%s\n\n\n", payload);

	    dup2(conn, STDOUT_FILENO);
        close(conn);

        // call router
        route();

        fflush(stdout);
        shutdown(STDOUT_FILENO, SHUT_WR);
        close(STDOUT_FILENO);

        pthread_mutex_lock(&count_mutex);
        test_data_logger(payload);
        pthread_mutex_unlock(&count_mutex);
    }

    //Closing SOCKET
    shutdown(conn, SHUT_RDWR);         //All further send and recieve operations are DISABLED...
    close(conn);
   
}


char* file_binary_loader(){
    FILE *file;
    char *buffer;
    char *reply;
    int fileLen;

    file = fopen("logfile.txt", "rb");
    if (!file)
    {
        return;
    }

    fseek(file, 0, SEEK_END);
    fileLen=ftell(file);
    fseek(file, 0, SEEK_SET);

    buffer=(char *)malloc(fileLen+1);
    if (!buffer)
    {
        fclose(file);
        return;
    }

    fread(buffer, fileLen, 1, file);
    fclose(file);

    reply = (char*)malloc(fileLen);
    memcpy(reply, buffer, fileLen);

    return reply;
}

void *test_data_logger(void *data){
    log_file = fopen("logfile.txt","a");
    fputs((char*)data, log_file);
    fclose(log_file);
}
