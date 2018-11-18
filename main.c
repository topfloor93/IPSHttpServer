#include "httpd.h"

int main(int c, char** v)
{
    serve_forever("12913");
    return 0;
}

void route(int conn)
{
    ROUTE_START()

    ROUTE_GET("/ips/logs")
    {
        printf("%s\r\n\r\n", file_binary_loader());
    }

    ROUTE_POST("/ips")
    {

    }

    ROUTE_POST("/ips/rules")
    {
        if(!rule_update(conn)){
            printf("HTTP/1.1 400 BAD REQUEST\r\n\r\n");
        }
    }

    ROUTE_END()
}