#include <iostream>
#include <unistd.h>
#include <cassert>
#include <fcntl.h>
#include <sys/socket.h>
#include "socket.h"

Socket::Socket( const int32_t fd ):
    _fd(fd),
    _timeout(0)
{
    _connectionState = ( ( _fd > 0 ) ? Connected : NotConnected );
}

void Socket::closeConnection(){
    if( _fd != -1 ) {
        close( _fd );
        _fd = -1;
    }
    _connectionState = NotConnected;
}

void Socket::setupSocket(){
    setSocketOptions();
    setSocketToNonBlocking();
}

void Socket::setTimeout( const int32_t microsecs ) {
    _timeout = microsecs;
}

Socket::ReadStatus_T Socket::readData( std::vector<char> &data ){
    const int32_t NumToRead = static_cast<int32_t>( data.size() );
    const int32_t NumRead = read( _fd, &data.front(), static_cast<uint32_t>(NumToRead));
    assert( NumToRead == NumRead || NumRead == -1 );
    const ReadStatus_T Status = ( ( NumToRead != NumRead ) ? ReadFailed : ( ( NumRead == 0 ) ? ReadNoData : ReadOK ) );
    return Status;
}

Socket::WriteStatus_T Socket::writeData( const std::vector<char> &data ){
    const int32_t NumToWrite = static_cast<int32_t>( data.size() );
    const int32_t NumWritten = write( _fd, &data.front(), static_cast<uint32_t>( NumToWrite ));
    const bool Success = ( NumToWrite == NumWritten );
    if ( !Success ){
        std::cerr << "Socket write failed. Tried to write " << NumToWrite << 
                     " but only wrote " << NumWritten << " bytes." << std::endl;
    }
    return Success ? WriteOK : WriteFailed;
}

Socket::ConnectionState_T Socket::getConnectionState() {
    return _connectionState;
}

void Socket::setSocketOptions(){
    const linger LingerTimeout = { 0, 0};
    const int32_t SetLinger = setsockopt( _fd, SOL_SOCKET, SO_LINGER, &LingerTimeout, sizeof( LingerTimeout ) );
    assert( SetLinger == 0 );
}

void Socket::setSocketToNonBlocking(){
    const int SocketSet = fcntl( _fd, F_SETFL, ( fcntl( _fd, F_GETFL ) | O_NONBLOCK ) );
    assert( SocketSet != -1 );
}

bool Socket::isDataAvailable(){
    fd_set read_flags;
    FD_ZERO( &read_flags );
    FD_SET( _fd, &read_flags );

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = _timeout;

    const int32_t SelectResult = select( ( _fd +  1 ), &read_flags, NULL, NULL, &timeout );
    return ( SelectResult > 0 );
}
