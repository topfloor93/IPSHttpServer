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
    }

    ROUTE_POST("/ips")
    {
        printf("HTTP/1.1 200 OK\r\n\r\n");
    }

    ROUTE_POST("/ips/rules")
    {
        printf("HTTP/1.1 200 OK\r\n\r\n");
    }

    ROUTE_END()
}