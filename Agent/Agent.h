#ifndef AGENT_H
#define AGENT_H

#include <signal.h>
#include <gio/gio.h>
#include <string>
#include <Agent1.h>

class Agent{
    public:
        Agent();
        ~Agent();
        void agentTask();

    private:
        GDBusConnection    *_p_systemBusConnection;
        OrgBluezAgent1     *_p_bluezAgentInterface;
        GDBusObjectManager *_p_bluezDevicesObjectManager;
        std::string        _dbusObjectPath;
        std::string        _pinCode;

        void establishDeviceObserver();
        void createAndExportAgent();
        void removeAgent();
        void setDeviceTrusted( const std::string DevicePath );
        void enableAdapterPower( const std::string AdapterPath );
        void disableSSP( const std::string AdapterPath );
        void enablePScan( const std::string AdapterPath );
        void initializeAdapter( const std::string AdapterPath );
        std::string generatePinCode();

        static gboolean releaseCallback( OrgBluezAgent1 *p_object,
                                         GDBusMethodInvocation *p_invocation,
                                         gpointer p_agent );

        static gboolean requestPinCodeCallback( OrgBluezAgent1 *p_object,
                                                GDBusMethodInvocation *p_invocation,
                                                const gchar *p_arg_device,
                                                gpointer p_agent);

        static gboolean displayPinCodeCallback( OrgBluezAgent1 *p_object,
                                                GDBusMethodInvocation *p_invocation,
                                                const gchar *p_arg_device,
                                                const gchar *p_arg_pincode,
                                                gpointer p_agent );

        static gboolean requestPasskeyCallBack( OrgBluezAgent1 *p_object,
                                                GDBusMethodInvocation *p_invocation,
                                                const gchar *p_arg_device,    
                                                gpointer p_agent );

        static gboolean displayPasskeyCallback( OrgBluezAgent1 *p_object,
                                                GDBusMethodInvocation *p_invocation,
                                                const gchar *p_arg_device,
                                                const guint32 *p_arg_passkey,
                                                const guint16 *p_arg-entered,
                                                gpointer p_agent );

        static gboolean requestConfirmationCallback( OrgBluezAgent1 *p_object,
                                                     GDBusMethodInvocation *p_invocation,
                                                     const gchar *p_arg_device,
                                                     const guint32 *p_arg_passkey,
                                                     gpointer p_agent );

        static gboolean requestAuthorizationCallback( OrgBluezAgent1 *p_object,
                                                      GDBusMethodInvocation *p_invocation,
                                                      const gchar *p_arg_device,
                                                      gpointer p_agent );

        static gboolean authorizeServiceCallback( OrgBluezAgent1 *p_object,
                                                  GDBusMethodInvocation *p_invocation,
                                                  const gchar *p_arg_device,
                                                  const gchar *p_arg_uuid,
                                                  gpointer p_agent );

        static gboolean cancelCallback( OrgBluezAgent1 *p_object,
                                        GDBusMethodInvocation *p_invocation,
                                        gpointer p_agent );

        /* Object Manager Callbacks */
        static gboolean objectAddedCallback( GDBusObjectManager *p_object_manager,
                                             GDBusObjectProxy *p_added_object,
                                             gpointer p_agent );

        static gboolean objectRemovedCallback( GDBusObjectManager *p_object_manager,
                                               GDBusObjectProxy *p_removed_object,
                                               gpointer p_agent );
};


#endif
