#include "../Configuration.hpp"
#include "../Connection.hpp"
#include <cstdio>
#include <netinet/in.h>
#define SERVER_IP INADDR_ANY
int main(){
    Connection conn(SERVER_IP,SERVER_PORT,true);
    conn.open();
    char n[] = "3 I";
    conn.send(n,sizeof(n));
    conn.recv();
    std::printf("11 %s",conn.getBuf());
    return 0;
}