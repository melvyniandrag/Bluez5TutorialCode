#include "Agent.h"

Agent::Agent():
    _p_systemBusConnection( NULL ),
    _p_bluezAgentInterface( NULL ),
    _p_bluezDevicesObjectManager( NULL ),
    _dbusObjectPath( "/com/example/CustomBluetoothAgent" ),
    _pinCode()
{
    g_autoptr( GError ) p_error = NULL;
    _p_systemBusConnection = g_bus_get_sync( G_BUS_TYPE_SYSTEM, NULL, &p_error );
    g_assert_no_error( p_error );
    _pinCode = generatePinCode();
    establishDeviceObserver();
    createAndExportAgent();
}

Agent::~Agent(){
    removeAgent();
    if ( _p_systemBusConnection != NULL ){
        if( _p_bluezDevicesObjectManager != NULL ){
            g_object_unref( _p_bluezDevicesObjectManager);
            _p_bluezDevicesObjectManager = NULL;
        }   

        g_dbus_connection_close_sync( _p_systemBusConnection, NULL, NULL );
        g_object_unref( _p_systemBusConnection );
        _p_systemBusConnection = NULL;
    }
}

void Agent::agentTask(){
    g_autoptr( GMainLoop ) p_mainLoop = g_main_loop_new( NULL, false );
    g_main_loop_run( p_mainLoop );
}

void Agent::establishDeviceObserver(){
    assert( _p_systemBusConnection != NULL );
    assert( _p_bluezDevicesObjectManager == NULL );
    g_autoptr( GError ) p_error = NULL;
    _p_bluezDevicesObjectManager = g_dbus_object_manager_client_new_sync( _p_systemBusConnection,
                                                                          G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                                                          "org.bluez",
                                                                          "/",
                                                                          NULL,
                                                                          NULL,
                                                                          NULL,
                                                                          NULL,
                                                                          &p_error ); 

    g_assert_no_error( p_error );
    g_signal_connect( _p_bluezDevicesObjectManager, "object-added", G_CALLBACK( objectAddedCallback ), reinterpret_cast<gpointer>( this ) );
    g_signal_connect( _p_bluezDevicesObjectManager, "object-removed", G_CALLBACK( objectRemovedCallback ), reinterpret_cast<gpointer>( this ) );

    GList *p_objectList = g_dbus_object_manager_get_objects( _p_bluezDevicesObjectManager );
    GList *p_objectListIterator = NULL;
    for( p_objectListIterator = p_objectList; p_objectListIterator != NULL; p_objectListIterator = p_objectListIterator->next ){
        const gchar *p_objectPath = g_dbus_object_get_object_path( G_DBUS_OBJECT( p_objectListIterator->data ) );
        GDBusInterface *p_possibleAdapterInterface = g_dbus_object_manager_get_interface( _p_bluezDevicesObjectManager, p_objectPath, "org.bluez.Adapter1");
        if( p_possibleAdapterInterface != NULL ){
            g_object_unref( p_possibleAdapterInterface );
            assert( p_objectPath != NULL );
            const std::string DevicePath( p_objectPath );
            initializeAdapter( DevicePath );
        } else {
            //Nothing
        }
    }
    g_list_free_full( p_objectList, g_object_unref );
}

void Agentt::createAndExportAgent(){
    assert( _p_systemBusConnection != NULL );
    assert( _p_bluezAgentInterface ==  NULL );
    g_autoptr( GError ) p_error;
    g_autoptr( GDBusProxy ) p_agentManager = NULL;
    p_agentManager = g_dbus_proxy_new_sync( _p_systemBusConnection,
                                            G_DBUS_PROXY_FLAGS_NONE,
                                            NULL,
                                            "org.bluez",
                                            "/org/bluez",
                                            "org.bluez.AgentManager1",
                                            NULL,
                                            &p_error );
    g_assert_no_error( p_error );
    _p_bluezAgentInterface = org_bluez_agent1_skeleton_new();
    g_signal_connect( _p_bluezAgentInterface, "handle-release",               G_CALLBACK( releaseCallback ),              reinterpret_cast<gpointer>( this ) );
    g_signal_connect( _p_bluezAgentInterface, "handle-request-pin-code",      G_CALLBACK( requestPinCodeCallback ),       reinterpret_cast<gpointer>( this ) );
    g_signal_connect( _p_bluezAgentInterface, "handle-display-pin-code",      G_CALLBACK( displayPinCodeCallback ),       reinterpret_cast<gpointer>( this ) );
    g_signal_connect( _p_bluezAgentInterface, "handle-request-passkey",       G_CALLBACK( requestPasskeyCallBack ),       reinterpret_cast<gpointer>( this ) );
    g_signal_connect( _p_bluezAgentInterface, "handle-display-passkey",       G_CALLBACK( displayPasskeyCallback ),       reinterpret_cast<gpointer>( this ) );
    g_signal_connect( _p_bluezAgentInterface, "handle-request-confirmation",  G_CALLBACK( requestConfirmationCallback ),  reinterpret_cast<gpointer>( this ) );
    g_signal_connect( _p_bluezAgentInterface, "handle-request-authorization", G_CALLBACK( requestAuthorizationCallback ), reinterpret_cast<gpointer>( this ) );
    g_signal_connect( _p_bluezAgentInterface, "handle-authorize-service",     G_CALLBACK( authorizeServiceCallback ),     reinterpret_cast<gpointer>( this ) );
    g_signal_connect( _p_bluezAgentInterface, "handle-cancel",                G_CALLBACK( cancelCallback ),               reinterpret_cast<gpointer>( this ) );


    g_dbus_interface_skeleton_export( G_DBUS_INTERFACE_SKELETON( _p_bluezAgentInterface ), _p_systemBusConnection, _dbusObjectPath.c_str(), &p_error );
    g_assert_no_error( p_error );

    GVariant * registerGVar = g_dbus_proxy_call_sync( p_agentManager,
                                                      "RegisterAgent",
                                                      g_variant_new( "(os)", _dbusObjectPath.c_str(), "DisplayYesNo" ),
                                                      G_DBUS_CALL_FLAGS_NONE,
                                                      -1,
                                                      NULL,
                                                      &p_error);

    g_assert_no_error( p_error );
    g_variant_unref( registerGVar );

    GVariant * defaultAgentGVar = g_dbus_proxy_call_sync( p_agentManager,
                                                          "RequestDefaultAgent",
                                                          g_variant_new( "(o)", _dbusObjectPath.c_str() ),
                                                          G_DBUS_CALL_FLAGS_NONE,
                                                          -1,
                                                          NULL,
                                                          &p_error );
    g_assert_no_error( p_error );
    g_variant_unref( defaultAgentGVar );
}

void Agent:removeAgent(){
    assert( _p_systemBusConnection != NULL );
    assert( _p_bluezAgentInterface != NULL );
    g_autoptr( GError ) p_error = NULL;
    g_autoptr( GDBusProxy ) agentManager = g_dbus_proxy_new_sync( _p_systemBusConnection,
                                                                  G_DBUS_PROXY_FLAGS_NONE,
                                                                  NULL,
                                                                  "org.bluez",
                                                                  "/org/bluez",
                                                                  "org.bluez.AgentManager1",
                                                                  NULL,
                                                                  &p_error );
    g_assert_no_error( p_error );
    GVariant * unregisterGvar_ptr = g_dbus_proxy_call_sync( agentManager,
                                                            "UnregisterAgent",
                                                            g_variant_new( "(o)", _dbusObjectPath.c_str() ),
                                                            G_DBUS_CALL_FLAGS_NONE,
                                                            -1,
                                                            NULL,
                                                            &p_error );
    g_assert_no_error( p_error );
    g_variant_unref( unregisterGvar_ptr );
    g_dbus_interface_skeleton_unexport( G_DBUS_INTERFACE_SKELETON( _p_bluezAgentInterface ) );

    g_object_unref( _p_bluezAgentInterface ) ;
    _p_bluezAgentInterface = NULL;
}

void Agent::setDeviceTrusted( const std::string DevicePath ){
    assert( _p_systemBusConnection != NULL );
    g_autoptr( GError ) p_error;
    g_autoptr( GDBusProxy ) deviceProperties_ptr = NULL;
    deviceProperties_ptr = g_dbus_proxy_new_sync( _p_systemBusConnection,
                                                  G_DBUS_PROXY_FLAGS_NONE,
                                                  NULL,
                                                  "org.bluez",
                                                  DevicePath.c_str(),
                                                  "org.freedekstop.DBus.Properties",
                                                  NULL,
                                                  &p_error);
    g_assert_no_error( p_error );
    GVariant *unregisterGvar_ptr = g_dbus_proxy_call_sync( deviceProperties_ptr,
                                                           "Set",
                                                           g_variant_new( "(ssv)", "org.bluez.Device1", "Trusted", g_variant_new( "b", true) ),
                                                           G_DBUS_CALL_FLAGS_NONE,
                                                           -1,
                                                           NULL,
                                                           &p_error );
    g_assert_no_error( p_error );
    g_variant_unref( unregisterGvar_ptr );
}

void Agent::enableAdapterPower( const std::string AdapterPath ){
    std::cout << "begin enableAdapterPower()" << std::endl;
    assert( _p_systemBusConnection != NULL );
    g_autoptr( GError ) p_error = NULL;
    g_autoptr( GDBusProxy ) adapterProperties_ptr = NULL;
    adapterProperties_ptr = g_dbus_proxy_new_sync( _p_systemBusConnection,
                                                   G_DBUS_PROXY_FLAGS_NONE,
                                                   NULL,
                                                   "org.bluez",
                                                   AdapterPath.c_str(),
                                                   "org.freedesktop.DBus.Properties",
                                                   NULL,
                                                   &p_error);
    g_assert_no_error( p_error );
    GVariant * unregisterGvar_ptr = g_dbus_proxy_call_sync( adapterProperties_ptr,
                                                            "Set",
                                                            g_variant_new( "(ssv)", "org.bluez.Adapter1", "Powered", g_variant_new( "b", true ) ),
                                                            G_DBUS_CALL_FLAGS_NONE,
                                                            -1,
                                                            NULL,
                                                            &p_error ); 
    g_assert_no_error( p_error );
    g_variant_unref( unregisterGvar_ptr );
    std::cout << "end enableAdapterPower()" << std::endl;
}

void Agent::disableSSP( const std::string AdapterPath ){
    std::cout << "begin disableSSP()" << std::endl;
    const size_t StartLeafName = AdapterPath.rfind( '/' );
    const std::string Command = "hciconfig " + AdapterPath.substr( StartLeafName, std::string::npos ) + " sspmode 0";
    static_cast<void>( system( Command.c_str() ) );
    std::cout << "end disableSSP()" << std::endl;
}

void Agent::enablePScan( const std::string AdapterPath ){
    std::cout << "begin enablePScan()" << std::endl;
    const size_t StartLeafName = AdapterPath.rfind( '/' );
    const std::string Command = "hciconfig " + AdapterPath.substr( StartLeafName, std::string::npos ) + " pscan";
    static_cast<void>( system( Command.c_str() ) );
    std::cout << "end enablePScan()" << std::endl;
}

void Agent::initializeAdapter( const std::string AdapterPath ){
    enableAdapterPower( AdapterPath );
    disableSSP( AdapterPath );
    enablePScan( AdapterPath );
}

std::string Agent::generatePinCode(){
    // This is the pincode the device will expect for pairing requests?
    return std::string{"1234"};
}

gboolean Agent::releaseCallback( OrgBluezAgent1 *object_ptr,
                                 GDBusMethodInvocation * invocation_ptr,
                                 gpointer agent_ptr )
{
    return true;
}


gboolean Agent::requestPinCodeCallback( OrgBluezAgent1 *p_object,
                                        GDBusMethodInvocation *p_invocation,
                                        const gchar *p_arg_device,
                                        gpointer p_agent);
{
    
}

gboolean Agent::displayPinCodeCallback( OrgBluezAgent1 *p_object,
                                        GDBusMethodInvocation *p_invocation,
                                        const gchar *p_arg_device,
                                        const gchar *p_arg_pincode,
                                        gpointer p_agent );

gboolean Agent::requestPasskeyCallBack( OrgBluezAgent1 *p_object,
                                        GDBusMethodInvocation *p_invocation,
                                        const gchar *p_arg_device,    
                                        gpointer p_agent );

gboolean Agent::displayPasskeyCallback( OrgBluezAgent1 *p_object,
                                        GDBusMethodInvocation *p_invocation,
                                        const gchar *p_arg_device,
                                        const guint32 *p_arg_passkey,
                                        const guint16 *p_arg-entered,
                                        gpointer p_agent );

gboolean Agent::requestConfirmationCallback( OrgBluezAgent1 *p_object,
                                                GDBusMethodInvocation *p_invocation,
                                                const gchar *p_arg_device,
                                                const guint32 *p_arg_passkey,
                                                gpointer p_agent );

gboolean Agent::requestAuthorizationCallback( OrgBluezAgent1 *p_object,
                                                GDBusMethodInvocation *p_invocation,
                                                const gchar *p_arg_device,
                                                gpointer p_agent );

gboolean Agent::authorizeServiceCallback( OrgBluezAgent1 *p_object,
                                            GDBusMethodInvocation *p_invocation,
                                            const gchar *p_arg_device,
                                            const gchar *p_arg_uuid,
                                            gpointer p_agent );

gboolean Agent::cancelCallback( OrgBluezAgent1 *p_object,
                                GDBusMethodInvocation *p_invocation,
                                gpointer p_agent );

/* Object Manager Callbacks */
gboolean Agent::objectAddedCallback( GDBusObjectManager *p_object_manager,
                                        GDBusObjectProxy *p_added_object,
                                        gpointer p_agent );

gboolean Agent::Agent::objectRemovedCallback( GDBusObjectManager *p_object_manager,
                                        GDBusObjectProxy *p_removed_object,
                                        gpointer p_agent );
