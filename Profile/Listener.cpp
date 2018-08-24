#include <iostream>

#include "Listener.h"

Listener::Listener():
    _dbusObjectPath( std::string{"/com/example/Example"}),
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
    _listenThread = std::thread( &Listener::listenTask, this );
}

void Listener::listenTask( void ){
    std::cout << "begin listenTask()" << std::endl;

    while( _heartbeat ){
        g_main_context_iteration( NULL, false );
        std::this_thread::sleep_for( std::chrono::seconds(1));
    }

    std::cout << "end listenTask()" << std::endl;
}


void Listener::stopListening(){
    _heartbeat = false;
    if( _listenThread.joinable() ){
        _listenThread.join();
    }
    unregisterProfile();
}


void Listener::registerProfile(){
    std::cout << "start of registerProfile()" << std::endl;
    assert( _p_systemBusConnection != NULL );
    assert( _p_bluezProfileInterface == NULL );

    g_autoptr( GError ) p_error = NULL;
    g_autoptr( GDBusProxy ) profileManager = NULL;
    profileManager = g_dbus_proxy_new_sync( _p_systemBusConnection,
                                            G_DBUS_PROXY_FLAGS_NONE,
                                            NULL,
                                            "org.bluez",
                                            "/org/bluez",
                                            "org.bluez.ProfileManager1",
                                            NULL,
                                            &p_error );
    g_assert_no_error( p_error );

    std::cout << 1 << std::endl;

    _p_bluezProfileInterface = org_bluez_profile1_skeleton_new();

    g_signal_connect( _p_bluezProfileInterface, "handle-new-connection", G_CALLBACK( handleNewConnectionCallback ), reinterpret_cast<gpointer>( this ) );
    g_signal_connect( _p_bluezProfileInterface, "handle-request-disconnection", G_CALLBACK( requestDisconnectionCallback ), reinterpret_cast<gpointer>( this ) );
    g_signal_connect( _p_bluezProfileInterface, "handle-release", G_CALLBACK( handleReleaseCallback ), reinterpret_cast<gpointer>( this ) );

    g_dbus_interface_skeleton_export( G_DBUS_INTERFACE_SKELETON( _p_bluezProfileInterface ), _p_systemBusConnection, _dbusObjectPath.c_str(), &p_error );

    g_assert_no_error( p_error );
    
    std::cout << 2 << std::endl;

    GVariantBuilder builder;
    g_variant_builder_init( &builder, G_VARIANT_TYPE_DICTIONARY );
    g_variant_builder_add( &builder, "{sv}", "Name", g_variant_new("s", _profileName.c_str() ));
    g_variant_builder_add( &builder, "{sv}", "Channel", g_variant_new( "q", _rfcommPort ) );
    g_variant_builder_add( &builder, "{sv}", "AutoConnect", g_variant_new( "b", false ) );
    g_variant_builder_add( &builder, "{sv}", "RequireAuthentication", g_variant_new("b", true) ); // Is this necessary?

    GVariant *gvar = g_dbus_proxy_call_sync( profileManager,
                                             "RegisterProfile",
                                             g_variant_new( "(osa{sv})", _dbusObjectPath.c_str(), _profileUUID.c_str(), &builder ),
                                             G_DBUS_CALL_FLAGS_NONE,
                                             -1,
                                             NULL,
                                             &p_error );
    g_assert_no_error( p_error );
    g_variant_unref( gvar );
    std::cout << "end of registerProfile()" << std::endl;
}

void Listener::unregisterProfile(){
    std::cout << "begin of unregisterProfile()" << std::endl;
    assert( _p_systemBusConnection != NULL );
    assert( _p_bluezProfileInterface != NULL );

    g_autoptr( GError ) p_error = NULL;

    g_autoptr( GDBusProxy ) profileManager = NULL;
    profileManager = g_dbus_proxy_new_sync( _p_systemBusConnection ,
                                            G_DBUS_PROXY_FLAGS_NONE,
                                            NULL,
                                            "org.bluez",
                                            "/org/bluez",
                                            "org.bluez.ProfileManager1",
                                            NULL,
                                            &p_error );

    g_assert_no_error( p_error );
    GVariant *gvar = g_dbus_proxy_call_sync( profileManager ,
                                             "UnregisterProfile",
                                             g_variant_new( "(o)", _dbusObjectPath.c_str() ),
                                             G_DBUS_CALL_FLAGS_NONE,
                                             -1,
                                             NULL,
                                             &p_error) ;
    g_assert_no_error( p_error );
    g_variant_unref( gvar );

    g_dbus_interface_skeleton_unexport( G_DBUS_INTERFACE_SKELETON( _p_bluezProfileInterface ) );

    g_object_unref( _p_bluezProfileInterface );
    _p_bluezProfileInterface = NULL;

    disconnectSocket();
    std::cout << "end unregisterProfile() " << std::endl;
}

void Listener::disconnect(){
    std::cout << "begin disconnect()" << std::endl;

    assert( _p_systemBusConnection != NULL );
    g_autoptr( GError ) p_error;
    g_autoptr( GDBusProxy ) p_proxyDevice = NULL;

    p_proxyDevice = g_dbus_proxy_new_sync( _p_systemBusConnection,
                                           G_DBUS_PROXY_FLAGS_NONE,
                                           NULL,
                                           "org.bluez",
                                           _dbusObjectPathCtdDevice.c_str(),
                                           "org.bluez.Device1",
                                           NULL,
                                           &p_error);
    if ( p_proxyDevice != NULL ){
        g_dbus_proxy_call( p_proxyDevice,
                           "Disconnect",
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL,
                           NULL);
    } else {
        std::cout << "The dbus object representing the connected device has already been removed." << std::endl;
    }

    disconnectSocket();
    std::cout << "end disconnect()" << std::endl;
}

void Listener::removeAllDevicesExcept( const std::string DevicePathToKeep ){
    assert( _p_systemBusConnection != NULL );

    g_autoptr( GError ) p_error = NULL;
    const std::string AdapterPath = "/org/bluez/hci0"; //that is adapter 0 at least, there might be more I think.

    g_autoptr( GDBusProxy ) btAdapter = NULL;
    btAdapter = g_dbus_proxy_new_sync( _p_systemBusConnection,
                                       G_DBUS_PROXY_FLAGS_NONE,
                                       NULL,
                                       "org.bluez",
                                       AdapterPath.c_str(),
                                       "org.bluez.Adapter1",
                                       NULL,
                                       &p_error );
    g_assert_no_error( p_error );

    std::vector<std::string> foundDevices = findAllRecognizedDevices();
    for( auto device : foundDevices ){
        if( device != DevicePathToKeep ){ // verify this works.
            GVariant *gvar = g_dbus_proxy_call_sync( btAdapter,
                                                     "RemoveDevice",
                                                     g_variant_new( "(o)", device.c_str() ),
                                                     G_DBUS_CALL_FLAGS_NONE,
                                                     -1,
                                                     NULL,
                                                     &p_error);
            if( gvar != NULL ){
                g_assert_no_error( p_error );
                g_variant_unref( gvar );
            }
        }
    }

}

std::vector<std::string> Listener::findAllRecognizedDevices(){
    std::cout << "begin findAllRecognizedDevices()" << std::endl;
    assert( _p_systemBusConnection != NULL );
    g_autoptr( GError ) p_error = NULL;
    g_autoptr( GDBusProxy ) objectManager = NULL;
    objectManager = g_dbus_proxy_new_sync( _p_systemBusConnection,
                                           G_DBUS_PROXY_FLAGS_NONE,
                                           NULL,
                                           "org.bluez",
                                           "/",
                                           "org.freedesktop.DBus.ObjectManager",
                                           NULL,
                                           &p_error);
    g_assert_no_error( p_error );
    g_autoptr( GVariant ) gvar = g_dbus_proxy_call_sync( objectManager,
                                                         "GetManagedObjects",
                                                         NULL,
                                                         G_DBUS_CALL_FLAGS_NONE,
                                                         -1,
                                                         NULL,
                                                         &p_error);
    g_assert_no_error(p_error);
    std::vector<std::string> foundDevices;
    assert( g_variant_n_children( gvar) == 1 );
    g_autoptr( GVariant ) objects = g_variant_get_child_value( gvar, 0 );
    g_autoptr( GVariantIter ) iter1 = NULL;
    g_autoptr( GVariantIter ) iter2 = NULL;
    const gchar *opath;
    g_variant_get( objects, "a{oa{sa{sv}}}", &iter1 );

    while ( g_variant_iter_loop( iter1, "{&oa{sa{sv}}}", &opath, &iter2 ) ){
        std::string newPath( opath );
        if ( newPath.find( "dev_" ) != std::string::npos ){
            foundDevices.push_back( newPath );
            std::cout << "found device: " << newPath << std::endl;
        }
    }

    std::cout << "end findAllRecognizedDevices()" << std::endl;
    return foundDevices;
}

void Listener::createNewSocketConnection( const int32_t fd, const std::string DBusPath ){
    std::cout << "begin createNewSocketConnection()" << std::endl;
    assert( _p_socket != NULL );
    Socket newSocket( fd );
    newSocket.setupSocket();

    *_p_socket = newSocket;
    _dbusObjectPathCtdDevice = DBusPath;
    std::cout << "end createNewSocketConnection() " << std::endl;
}

void Listener::disconnectSocket(){
    std::cout << " begin disconnectSocket()" << std::endl;
    assert( _p_socket != NULL );
    _p_socket->closeConnection();
    std::cout << "end disconnectSocket()" << std::endl;
}

/********************************* CALLBACK FUNCTIONS ****************************************/

gboolean Listener::handleNewConnectionCallback( OrgBluezProfile1 *p_object,
                                                GDBusMethodInvocation *p_invocation,
                                                GUnixFDList *p_fd_list,
                                                const gchar *p_arg_device,
                                                GVariant *p_arg_fd,
                                                GVariant *p_arg_fd_properties,
                                                gpointer p_listener )
{
    std::cout << "begin handleNewConnectionCallback()" << std::endl;
    g_autoptr( GError ) p_error = NULL;
    const int32_t NewConnectionFD = static_cast<int32_t>( g_unix_fd_list_get( p_fd_list, 0, &p_error) );
    g_assert_no_error( p_error );
    Listener * const p_Listener = reinterpret_cast<Listener *>( p_listener );
    p_Listener->removeAllDevicesExcept( std::string( p_arg_device ) );
    p_Listener->createNewSocketConnection( NewConnectionFD, std::string( p_arg_device ) );
    std::cout << "end handleNewConnectionCallback()" << std::endl;
    return true;
}

gboolean Listener::requestDisconnectionCallback( OrgBluezProfile1 *p_object,
                                                 GDBusMethodInvocation *p_invocation,
                                                 const gchar *p_arg_device,
                                                 gpointer p_listener )
{
    std::cout << "begin requestDisconnectionCallback()" << std::endl;
    Listener *const p_Listener = reinterpret_cast<Listener *>( p_listener );
    p_Listener->disconnectSocket();
    std::cout << "end requestDisconnectionCallback()" << std::endl;
    return true;
}

gboolean Listener::handleReleaseCallback( OrgBluezProfile1 *p_object,
                                          GDBusMethodInvocation *p_invocation,
                                          gpointer p_listener )
{
    std::cout << "begin handleReleaseCallback()" << std::endl;
    Listener *const p_Listener = reinterpret_cast<Listener *>( p_listener );
    p_Listener->disconnectSocket();
    std::cout << "end handleReleaseCallback()" << std::endl;
    return true;
}                                          
