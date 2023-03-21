#include "../Configuration.hpp"
#include "../Connection.hpp"
#include <cstdio>
int main(){
    Connection conn(SERVER_IP,SERVER_PORT,true);
    conn.open();
    char n[] = "0 192.1.1.1 no";
    conn.send(n,sizeof(n));
    conn.recv();
    std::printf("%s",conn.getBuf());
    return 0;
}