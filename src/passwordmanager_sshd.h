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


#ifndef ORG_NEMO_PASSWORDMANAGER_SSHD_H
#define ORG_NEMO_PASSWORDMANAGER_SSHD_H

#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QObject>
#include <QString>

enum RemoteLoginState {
    Unknown,
    Enabled,
    Disabled
};

class PasswordManagerSshd : public QObject {
    Q_OBJECT
    Q_ENUM(RemoteLoginState)

    public:
        explicit PasswordManagerSshd(QObject *parent = nullptr);
        ~PasswordManagerSshd();
        void enableRemoteLogin();
        void disableRemoteLogin();

        RemoteLoginState isRemoteLoginEnabled();

        enum JobState {
            Idle = 0,
            Enabling = 0x01,
            Disabling = 0x02,
            UnitFileDone = 0x04,
            JobRunning = 0x08,
            JobFinished = 0x10,
            Ready = UnitFileDone | JobRunning | JobFinished,
            EnabledAndStarted = Enabling | Ready,
            DisabledAndStopped = Disabling | Ready
        };
        Q_DECLARE_FLAGS(JobStates, JobState)

    signals:
        void remoteLoginEnabledChanged(bool enabled);

    private:
        using PendingReplyHandler = void (PasswordManagerSshd::*)(QDBusPendingCallWatcher *);
        void inline watchPendingCall(QDBusPendingCall pendingCall, PendingReplyHandler func);
        void inline checkIfFinished();

    private slots:
        void handleJobRemoved(uint id, QDBusObjectPath job, QString unit, QString result);
        void handleJobStarted(QDBusPendingCallWatcher *watcher);
        void handleUnitFileChanged(QDBusPendingCallWatcher *watcher);
        void handleUnitFilesChanged();
        void handleUnitFileState(QDBusPendingCallWatcher *watcher);

    private:
        QDBusInterface m_systemdManager;

        RemoteLoginState m_state;

        JobStates m_jobState;
        QString m_job;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(PasswordManagerSshd::JobStates)

#endif /* ORG_NEMO_PASSWORDMANAGER_SSHD_H */
