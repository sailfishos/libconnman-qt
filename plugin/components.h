/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef COMPONENTS_H
#define COMPONENTS_H
#include <QtPlugin>

#include <QtQml>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>

class Components : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "MeeGo.Connman")

public:
    void registerTypes(const char *uri);

    void initializeEngine(QQmlEngine *engine, const char *uri);
};

#endif // COMPONENTS_H
