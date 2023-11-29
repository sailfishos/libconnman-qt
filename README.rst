Qt bindings for ConnMan
=======================

The Qt bindings are direct reflection of ConnMan's D-Bus interfaces:
the class `NetworkManager` represents `net.connman.Manager`,
`NetworkTechnology` represents `net.connman.Technology` and
`NetworkService` represents `net.connman.Service`.
`NetworkSession` represents `net.connman.Session`.
`NetworkTechnology` represents `net.connman.Technology`.

`TechnologyModel` is a QML component representing a list of network
services of a given technology.

`UserAgent` is a QML component providing `net.connman.Agent` D-Bus interface.

These classes are written to be re-usable in the qml environment, by setting the
path property to that of an appropriate dbus path, will re-initialize the object to be used for the path given.


QMake CONFIG flags
-----------
* noplugin: doesn't compile qml plugin

Example:
``qmake CONFIG+=noplugin``

Extra make targets
-----------

* check: run tests directly inside build tree
* coverage: generate code coverage report
