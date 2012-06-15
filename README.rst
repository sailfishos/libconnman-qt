Qt bindings for ConnMan
=======================

Installation
------------

This package builds into the binary packages:

 1. `connman-qt` - Qt bindings for Connman;
 2. `connman-qt-declarative` - contains QML component adapting Connman's
     functionality;
 3. `connman-qt-controlpanel` - QML and .desktop files of CP pluging;
 4. `connman-qt-tests` - simple test application.

 In order to install the packages you have to

 1. upgrade the package `connman` to the version 1.0;
 2. delete the packages `meegotouchcp-connman`, `meegotouchcp-connman-qt`,
    `meegotouchcp-connman-branding-upstream`,
    `meegotouchcp-bluetooth-libmeegobluetooth`, `meegotouchcp-bluetooth`;
 3. hack out connman plugins from the package `contextkit-meego`.

Architecture
------------

The Qt bindings are dirrect reflection of ConnMan's D-Bus interfaces:
the class `NetworkManager` represents `net.connman.Manager`,
`NetworkTechnology` represents `net.connman.Technology` and
`NetworkService` represents `net.connman.Service`.

The class `NetworkingModel` is a QML component adapting a static instance of
`NetworkManager`. Also it provides the D-Bus interface `net.connman.Agent`.
