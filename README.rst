Qt bindings for ConnMan
=======================

The Qt bindings are direct reflection of ConnMan's D-Bus interfaces:
the class `NetworkManager` represents `net.connman.Manager`,
`NetworkTechnology` represents `net.connman.Technology` and
`NetworkService` represents `net.connman.Service`.

`TechnologyModel` is a QML component representing a list of network
services of a given technology.

`UserAgent` is a QML component providing `net.connman.Agent` D-Bus interface.

The class `NetworkingModel` is a QML component adapting a static instance of
`NetworkManager`. Also it provides the D-Bus interface `net.connman.Agent`.

.. warning:: `NetworkingModel` is going to be deprecated.

QMake CONFIG flags
-----------
* notest: doesn't compile test program
* noplugin: doesn't compile qml plugin

Example:
``qmake CONFIG+=notest``
