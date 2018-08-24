Agent::Agent():
    _p_systemBusConnection( NULL ),
    _p_bluezAgentInterface( NULL ),
    _p_bluezDevicesObjectManager( NULL ),
    _dbusObjectPath( "/com/example/CustomBluetoothAgent" ),
    _pinCode()
{
}
