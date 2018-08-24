#include "listener.h"

Listener::Listener():
    _objectPath( std::string{"/com/example/Example"}),
    _profileName( std::string{"Example"} ),
    _profileUUID( std::string{"b298b822-a5bc-4ab0-a4ec-fadb980002b6"}),
    _rfcommPort( 22 ),
    _p_socket( nullptr ),
    _p_systemBusConnection( NULL ),
    _p_bluezProfileInterface( NULL ),
    _heartbeat( false )
{
    g_autoptr( GError ) p_error = NULL;
    _p_systemBusConnection = g_bus_get_sync( G_BUS_TYPE_SYSTEM, NULL, &p_error );
    g_assert_no_error( p_error );
}

Listener::~Listener(){
    stopListening();
    if( _p_systemBusConnection != NULL ){
        g_dbus_connection_close_sync( _p_systemBusConnection, NULL, NULL );
        g_object_unref( _p_systemBusConnection );
        _p_systemBusConnection = NULL;
    }
}

void Listener::startListening( Socket &p_socket){
    registerProfile();
    _p_socket = &p_socket;
    _heartbeat = true;
    _listenThread = std::thread( Listener::listenTask, this );
}

void Listener::stopListening(){
    _heartbeat = false;
    if( _listenThread.joinable() ){
        _listenThread.join();
    }
    unregisterProfile();
}


void Listener::registerProfile(){
}
