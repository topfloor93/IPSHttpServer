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
        //printf("%s\r\n\r\n", file_binary_loader());
    }

    ROUTE_POST("/ips")
    {
        request_ips();

        printf("%s \r\n\r\n", "HTTP/1.1 200 OK"); //ALEART!! don't remove it 
        char buff[255];
        char *res = "HTTP/1.1 ";

        strcat(buff, res);
        strcat(buff, response_ips(flag));
        printf("%s \r\n", buff);
    }

    ROUTE_POST("/ips/rules")
    {
        //if(!rule_update(conn)){
        //    printf("HTTP/1.1 400 BAD REQUEST\r\n\r\n");
        //}
    }

    ROUTE_END()
}
