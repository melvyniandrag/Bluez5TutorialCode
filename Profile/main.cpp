#include <vector>
#include <string>
#include <iostream>
#include "socket.h"
#include "Listener.h"

int main(){
    Listener listener;
    Socket socket(-1);
    listener.startListening( socket);
    while(true){
        std::vector<char> ReadData{'\0'};
        socket.readData( ReadData );
        std::cout << "Header read from socket: " << std::endl;
        const std::string HeaderStr( ReadData.begin(), ReadData.end() );
        std::cout << HeaderStr << std::endl;

        sleep(4);

        const int N = std::atoi( HeaderStr.c_str() );
        std::vector<char> ReadData2( N, '\0');
        socket.readData( ReadData2 );
        std::cout << "Data read from socket:" << std::endl;
        std::cout << std::string(ReadData2.begin(), ReadData2.end() ) << std::endl;
        
        sleep(4);
    }
}
