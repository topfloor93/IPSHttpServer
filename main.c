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
        printf("%s\n", file_binary_loader());
    }

    ROUTE_POST("/ips")
    {

    }

    ROUTE_POST("/ips/rules")
    {

    }

    ROUTE_END()
}