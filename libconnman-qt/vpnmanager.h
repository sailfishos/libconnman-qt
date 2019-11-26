/*
 * Copyright (c) 2016 - 2019 Jolla Ltd.
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#ifndef VPNMANAGER_H
#define VPNMANAGER_H

#include <QObject>

class VpnManagerPrivate;
class VpnManager;
class VpnConnection;

// ==========================================================================
// VpnManagerFactory
// ==========================================================================

class VpnManagerFactory : public QObject
{
    Q_OBJECT

    Q_PROPERTY(VpnManager* instance READ instance CONSTANT)

public:
    static VpnManager* createInstance();
    VpnManager* instance();
};

// ==========================================================================
// VpnManager
// ==========================================================================

class VpnManager : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(VpnManager)
    Q_DISABLE_COPY(VpnManager)

    Q_PROPERTY(QVector<VpnConnection*> connections READ connections NOTIFY connectionsChanged)
    Q_PROPERTY(bool populated READ populated NOTIFY populatedChanged)

public:
    explicit VpnManager(QObject *parent = nullptr);
    explicit VpnManager(VpnManagerPrivate &dd, QObject *parent);
    virtual ~VpnManager();

    Q_INVOKABLE void createConnection(const QVariantMap &properties);
    Q_INVOKABLE void modifyConnection(const QString &path, const QVariantMap &properties);
    Q_INVOKABLE void deleteConnection(const QString &path);

    Q_INVOKABLE void activateConnection(const QString &path);
    Q_INVOKABLE void deactivateConnection(const QString &path);

    Q_INVOKABLE VpnConnection *connection(const QString &path) const;
    Q_INVOKABLE VpnConnection *get(int index) const;
    Q_INVOKABLE int indexOf(const QString &path) const;
    int count() const;
    QVector<VpnConnection*> connections() const;
    bool populated() const;

signals:
    void connectionsChanged();
    void connectionAdded(const QString &path);
    void connectionRemoved(const QString &path);
    void connectionsRefreshed();
    void connectionsCleared();
    void populatedChanged();

private:
    QScopedPointer<VpnManagerPrivate> d_ptr;
};

#endif // VPNMANAGER_H
