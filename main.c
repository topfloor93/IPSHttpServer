#include "httpd.h"

int main(int c, char** v)
{
    serve_forever("12913");
    return 0;
}

void route()
{
    ROUTE_START()

    ROUTE_GET("/ips/logs")
    {
        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("Hello! You are using %s", request_header("User-Agent"));
    }

    ROUTE_POST("/ips")
    {
        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("Wow, seems that you POSTed %d bytes. \r\n", payload_size);
        //printf("you sent %s\n", payload);
        printf("Fetch the data using `payload` variable.");
    }

    ROUTE_POST("/ips/rules")
    {
        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("Wow, seems that you POSTed %d bytes. \r\n", payload_size);
        //printf("you sent %s\n", payload);
        printf("Fetch the data using `payload` variable.");
    }

    ROUTE_END()
}