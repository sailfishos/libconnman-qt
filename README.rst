Qt bindings for ConnMan
=======================

The Qt bindings are direct reflection of ConnMan's D-Bus interfaces:
the class `NetworkManager` represents `net.connman.Manager`,
`NetworkTechnology` represents `net.connman.Technology` and
`NetworkService` represents `net.connman.Service`.

The class `NetworkingModel` is a QML component adapting a static instance of
`NetworkManager`. Also it provides the D-Bus interface `net.connman.Agent`.
