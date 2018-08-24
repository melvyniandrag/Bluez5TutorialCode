#ifndef LISTENER_H
#define LISTENER_H

class Listener{
    public:
        Listener();
        ~Listener();
        void startListening( Socket &p_socket );
        void stopListening();
        void disconnect();
    private():
        Socket *_p_socket;
        std::thread _listenThread;
        GDBusConnection *_p_systemBusConnection;
        OrgBluezProfile1 *_p_bluezProfileInterface;
        bool _heartbeat;
        std::string _dbusObjectPath;
        std::string _dbusObjectPathCtdDevice;
        
        const std::string _objectPath;
        const std::string _profileName;
        const std::string _profileUUID;
        const int32_t     _rfcommPort;

        void registerProfile();
        void unregisterProfile();
        void removeAllDevicesExcept( const std::string DevicePathToKeep );
        void createNewSocketConnection( const int32_t fd, const std::string DbusPath );
        void disconnectSocket();
        std::vector<std::string> findAllRecognizedDevices();
        void listenTask();

        static gboolean handleNewConnectionCallback( OrgBluezProfile1 *p_object,
                                                      GDBusMethodInvocation *p_invocation,
                                                      GUnixFDList *p_fd_list,
                                                      const gchar *p_arg_device,
                                                      GVariant *p_arg_fd,
                                                      GVariant *p_arg_fd_properties,
                                                      gpointer p_listener );

        static gboolean requestDisconnectionCallback( OrgBluezProfile1 *p_object, 
                                                       GDBusMethodInvocation *p_invocation,
                                                       const gchar *p_arg_device, 
                                                       gpointer p_listener );

        static gboolean handleReleaseCallback( OrgBluezProfile1 *p_object,
                                      GDBusMethodInvocation *p_invocation, 
                                      gpointer *p_listener );
};

#endif
