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

#ifndef VPNMANAGER_P_H
#define VPNMANAGER_P_H

#ifdef CONNMANQT_CMAKE
#include "connman_vpn_managerinterface.h"
#else
#include "connman_vpn_manager_interface.h"
#endif

#include "vpnmanager.h"

class VpnManagerPrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(VpnManager)

public:
    VpnManagerPrivate(VpnManager &qq);
    void init();
    void fetchVpnList();
    void setPopulated(bool populated);

    static VpnManagerPrivate *get(VpnManager *manager) { return manager->d_func(); }
    static const VpnManagerPrivate *get(const VpnManager *manager) { return manager->d_func(); }

Q_SIGNALS:
    void beginConnectionsReset();
    void endConnectionsReset();

public:
    NetConnmanVpnManagerInterface m_connmanVpn;
    QVector<VpnConnection*> m_items;
    bool m_populated;

    VpnManager *q_ptr;
};

#endif // VPNMANAGER_P_H
