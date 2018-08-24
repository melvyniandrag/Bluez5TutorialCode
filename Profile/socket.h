#ifndef SOCKET_H
#define SOCKET_H

#include <cstdint>
#include <vector>

class Socket {
    public:
        enum ReadStatus_T {
            ReadOK,
            ReadFailed,
            ReadNoData
        };

        enum WriteStatus_T {
            WriteOK,
            WriteFailed
        };

        enum ConnectionState_T {
            Connected,
            NotConnected,
            WaitingForOther
        };

        explicit Socket( const int32_t fd );

        void closeConnection();

        void setupSocket();

        bool isDataAvailable();

        void setTimeout( const int32_t millisecs );

        ReadStatus_T readData( std::vector<char> &data );

        WriteStatus_T writeData( const std::vector<char> &data );

        ConnectionState_T getConnectionState();

    private:
        int32_t _fd;
        int32_t _timeout;
        ConnectionState_T _connectionState;

        void setSocketOptions();

        void setSocketToNonBlocking();
};

#endif
