#include "httpd.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define CONNMAX 10000

static int listenfd, conn;
static void error(char *);
static void startServer(const char *);
static void respond(int);
static void logger(void *data);
static void pipe_writer(void *data);
static int pipe_reader();
static const char * timelog();

typedef struct { char *name, *value; } header_t;

static header_t reqhdr[17] = { {"\0", "\0"} };
static int clientfd;
static FILE *log_file;

static char *buf;
pthread_mutex_t count_mutex;
pthread_mutex_t packet_in_queue_mutex;
long long count;

static char *httplog;
static int packet_in_queue_fd;
static const char *pakcet_in_queue = "/tmp/packet_in_queue";
static char *pid;

void serve_forever(const char *PORT)
{
    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    char c;
    char temp[0];

    void *in_q = NULL;
    int slot=0;
    pthread_t thread_id;

    printf(
            "Server started %shttp://127.0.0.1:%s%s\n",
            "\033[92m",PORT,"\033[0m"
    );

    
    // Setting all elements to -1: signifies there is no client connected
    int i;
    startServer(PORT);
    mkfifo(pakcet_in_queue, 0666);

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
		sprintf(temp,"%d", getpid());
		pid = temp;
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

void *request_ips() {
    
    pipe_writer(httplog);
	

    if (pipe_reader()==0) {
        return 0;
    }
}

char *response_ips(int flag) {
    if(flag == 1){
        return "200 OK";
    } else {
        return "404 Forbidden";
    }
}

//client connection
void respond(int conn)
{
    int rcvd, bytes_read;
    char *ptr;
    buf = malloc(65535);
    rcvd=recv(conn, buf, 65535, 0);
    char *action_log;
    char *ff = " > ";
    const char *rt = timelog();
    char *action1 = " >> \"ACCEPT\"";
    char *action2 = " >> \"DENY\"";

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

        fprintf(stderr, "\x1b[32m + [%s] %s\x1b[0m", method, uri);
	fprintf(stderr, "\x1b[32m >> %s\x1b[0m\n", rt); 

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
        payload = t+2;
        payload_size = t2 ? atol(t2) : (rcvd-(t-buf));
	
        fprintf(stderr, "[B] %s\n\n", payload);
	
        httplog = malloc(strlen(rt) + strlen(ff) + strlen(payload) + 1);
        strcpy(httplog, rt);
        strcat(httplog, ff);
        strcat(httplog, payload);
	
        if (dup2(conn, STDOUT_FILENO)) {
            route();
        }

        fflush(stdout);
        shutdown(STDOUT_FILENO, SHUT_WR);
        close(STDOUT_FILENO);

        if (flag == 1) {
            action_log = malloc(strlen(httplog) + strlen(action1) + 1);
            strcpy(action_log, httplog);
            strcat(action_log, action1);
        } else if (flag == 0) {
            action_log = malloc(strlen(httplog) + strlen(action2) + 1);
            strcpy(action_log, httplog);
            strcat(action_log, action2);
        }
        logger(action_log);
    }

    //Closing SOCKET
    shutdown(conn, SHUT_RDWR);         
    close(conn);
    free(buf);
   
}

/**
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

    buffer=(char *)colloc(fileLen+1, 0);
    if (!buffer)
    {
        fclose(file);
        return;
    }

    fread(buffer, fileLen, 1, file);
    fclose(file);

    reply = (char*)colloc(fileLen, 0);
    memcpy(reply, buffer, fileLen);
    free(buffer);

    return reply;
}


int rule_update(int conn){
    File *file;
    char*buffer;

    buffer = colloc(payload_size, 0);//+1 ?
    buffer[payload_size] = '\0';

    file_name = strtok(buffer,  " \t\r\n");
    file_size = strtok(NULL, "\t");

    //hash(payload);
    //printf("%s\r\n\r\n", hash);
    buffer = colloc(file_size+1, 0);

    rcvd=recv(conn, buffer, file_size, 0);

    if (rcvd<=0)
        return FALSE;
    else
    {
        file = fopen(file_name, "wb");
        fwrite(buffer, file_size, 1, file);
        fclose(file);
        free(buffer);
    }
    return TRUE;
}
*/
void logger(void *data){
    pthread_mutex_lock(&count_mutex);

    log_file = fopen("logfile.txt","a");
    fputs((char*)data, log_file);
    fputs("\n", log_file);
    fclose(log_file);

    pthread_mutex_unlock(&count_mutex);
}

const char * timelog() {
    static char buff[30];
    char usec_buf[6];
    int millsec;
    struct tm *sTm;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    
    millsec = (int)(tv.tv_usec/1000.0);
    if (millsec>=1000) {
        millsec -=1000;
        tv.tv_sec++;    
    }

    sTm = localtime(&tv.tv_sec);
    
    strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", sTm); 
    strcat(buff, ".");
    sprintf(usec_buf, "%03d",millsec);
    strcat(buff, usec_buf);
    
    return buff;
}


void pipe_writer(void *data) {
    char *msg;
    msg = malloc(strlen((char*)data) + strlen(pid) + strlen(" ") + 1);
    strcpy(msg, pid);
    strcat(msg, " ");
    strcat(msg, (char *)data);

    pthread_mutex_lock(&packet_in_queue_mutex);
    
    packet_in_queue_fd = open(pakcet_in_queue, O_WRONLY);
    write(packet_in_queue_fd, msg, strlen(msg));
    close(packet_in_queue_fd);
    
    pthread_mutex_unlock(&packet_in_queue_mutex);
}

int pipe_reader() {
    int packet_out_queue_fd;
    char *form = "/tmp/packet_out_queue_";
    char *packet_out_queue;
    char buf[16];
    
    packet_out_queue = malloc(strlen(form) + strlen(pid) + 1);
    strcpy(packet_out_queue, form);
    strcat(packet_out_queue, pid);

    packet_out_queue_fd = open(packet_out_queue, O_RDONLY);
    read(packet_out_queue_fd, buf, 16);
    
    flag = atoi(&buf[0]);
    close(packet_out_queue_fd);
    
    return 0;
}

