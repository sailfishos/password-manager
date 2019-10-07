/**
 * Nemo Password Manager: D-Bus Service for changing and generating passwords
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/


#include <QDBusReply>
#include <QDBusObjectPath>
#include <QtDebug>
#include "passwordmanager_sshd.h"

static const auto SshdSocket = QStringLiteral("sshd.socket");
static const auto Replace = QStringLiteral("replace");

PasswordManagerSshd::PasswordManagerSshd(QObject *parent)
    : QObject(parent)
    , m_systemdManager("org.freedesktop.systemd1", "/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager", QDBusConnection::systemBus())
    , m_state(Unknown)
    , m_jobState(Idle)
    , m_job(QString())
{
    connect(&m_systemdManager, SIGNAL(JobRemoved(uint, QDBusObjectPath, QString, QString)),
            this, SLOT(handleJobRemoved(uint, QDBusObjectPath, QString, QString)));
    connect(&m_systemdManager, SIGNAL(UnitFilesChanged()),
            this, SLOT(handleUnitFilesChanged()));
    handleUnitFilesChanged();
}

PasswordManagerSshd::~PasswordManagerSshd()
{
}

void
PasswordManagerSshd::enableRemoteLogin()
{
    if (m_jobState != Idle || m_job.size())
        // Work in progress, do nothing
        return;

    m_jobState = Enabling;

    watchPendingCall(m_systemdManager.asyncCall(QStringLiteral("StartUnit"), SshdSocket, Replace),
                &PasswordManagerSshd::handleJobStarted);
    watchPendingCall(m_systemdManager.asyncCall(QStringLiteral("EnableUnitFiles"), QStringList() << SshdSocket, false, false),
                &PasswordManagerSshd::handleUnitFileChanged);
}

void
PasswordManagerSshd::disableRemoteLogin()
{
    if (m_jobState != Idle || m_job.size())
        // Work in progress, do nothing
        return;

    m_jobState = Disabling;

    watchPendingCall(m_systemdManager.asyncCall(QStringLiteral("StopUnit"), SshdSocket, Replace),
                &PasswordManagerSshd::handleJobStarted);

    watchPendingCall(m_systemdManager.asyncCall(QStringLiteral("DisableUnitFiles"), QStringList() << SshdSocket, false),
                &PasswordManagerSshd::handleUnitFileChanged);
}

RemoteLoginState
PasswordManagerSshd::isRemoteLoginEnabled()
{
    return m_state;
}

void
PasswordManagerSshd::watchPendingCall(QDBusPendingCall pendingCall, PendingReplyHandler func)
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, func);
}

void
PasswordManagerSshd::checkIfFinished()
{
    if ((m_jobState & Ready) == Ready) {
        m_state = (m_jobState & Enabling) ? Enabled : Disabled;
        emit remoteLoginEnabledChanged(m_state == Enabled);
        m_jobState = Idle;
    }
}

void
PasswordManagerSshd::handleJobStarted(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QDBusObjectPath> reply = *call;
    if (reply.isValid())
        m_job = reply.value().path();
    else
        qWarning() << "Creating job returned error:" << reply.error();
    m_jobState |= JobRunning;
    call->deleteLater();
}

void
PasswordManagerSshd::handleJobRemoved(uint id, QDBusObjectPath job, QString unit, QString result)
{
    Q_UNUSED(id)

    if (m_jobState == Idle || unit != SshdSocket || m_job != job.path())
        // This is not concerning us
        return;

    if (result != QStringLiteral("done"))
        qWarning() << "SystemdManager job status was not done:" << job.path() << ":" << result;

    m_job = QString();
    m_jobState |= JobFinished;
    checkIfFinished();
}

void
PasswordManagerSshd::handleUnitFileChanged(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<> reply = *call;
    if (reply.isError())
        qWarning() << "Changing unit file returned error:" << reply.error();
    m_jobState |= UnitFileDone;
    checkIfFinished();
    call->deleteLater();
}

void
PasswordManagerSshd::handleUnitFilesChanged()
{
    watchPendingCall(m_systemdManager.asyncCall(QStringLiteral("GetUnitFileState"), SshdSocket),
                &PasswordManagerSshd::handleUnitFileState);
}

void
PasswordManagerSshd::handleUnitFileState(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QString> reply = *call;
    if (reply.isValid()) {
        RemoteLoginState prevState = m_state;
        m_state = (reply.value() == QStringLiteral("enabled")) ? Enabled : Disabled;
        if (prevState != m_state) {
            emit remoteLoginEnabledChanged(m_state == Enabled);
        }
    } else {
        qWarning() << "Unit file state query returned error:" << reply.error();
    }
    call->deleteLater();
}
