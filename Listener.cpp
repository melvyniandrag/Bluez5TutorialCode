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
    std::cout << "start of registerProfile()" << std::endl;
    assert( _p_systemBusConnection != NULL );
    assert( _p_bluezProfileInterface != NULL );

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

    _p_bluezProfileInterface - org_bluez_profile1_skeleton_new();

    g_signal_connect( _p_bluezProfileInterface, "handle-new-connection", G_CALLBACK( handleNewConnectionCallback ), reinterpret_cast<gpointer>( this ) );
    g_signal_connect( _p_bluezProfileInterface, "handle-request-disconnection", G_CALLBACK( requestDisconnectionCallback ), reinterpret_cast<gpointer>( this ) );
    g_signal_connect( _p_bluezProfileInterface, "handle-release", G_CALLBACK( handleReleaseCallback ), reinterpret_cast<gpointer>( this ) );

    g_dbus_interface_skeleton_export( G_DBUS_INTERFACE_SKELETON( _p_bluezProfileInterface ), _p_systemBusConnection, _objectPath.c_str(), &p_error );

    g_assert_no_error( p_error );

    GVariantBuilder builder;
    g_variant_builder_init( &builder, G_VARIANT_TYPE_DICTIONARY );
    g_variant_builder_init( &builder, "{sv}", "Name", g_variant_new("s", _profileName.c_str() ));
    g_variant_builder_init( &builder, "{sv}", "Channel", g_variant_new( "q", _rfcommPort ) );
    g_variant_builder_init( &builder, "{sv}", "AutoConnect", g_variant_new( "b", false ) );

    GVariant *gvar = g_dbus_proxy_call_sync( profileManager,
                                             "registerProfile",
                                             g_variant_new( "(osa{sv})", _objectPath.c_str(), _profileUUID.c_str(), &builder ),
                                             G_DBUS_CALL_FLAGS_NONE,
                                             -1,
                                             NULL,
                                             &p_error );
    g_assert_no_error( p_error );
    g_variant_unref( gvar );
    std::cout << "end of registerProfile()" << std::endl;
}
