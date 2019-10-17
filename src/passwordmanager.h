/**
 * Nemo Password Manager: D-Bus Service for changing and generating passwords
 * Copyright (c) 2013 - 2019 Jolla Ltd.
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


#ifndef ORG_NEMO_PASSWORDMANAGER_H
#define ORG_NEMO_PASSWORDMANAGER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QDBusContext>
#include <QQueue>

#include "passwordmanager_store.h"
#include "passwordmanager_sshd.h"

class PasswordManager : public QObject, protected QDBusContext {
    Q_OBJECT

    public:
        PasswordManager(QObject *parent=NULL);
        virtual ~PasswordManager();

    public slots:
        void generatePassword();
        QString getGeneratedPassword();
        void setPassword(const QString &password);
        // Password login
        bool isLoginEnabled();
        // Remote login
        void setRemoteLoginEnabled(bool enabled);
        bool isRemoteLoginEnabled();
        void quit();

    private slots:
        void handleRemoteLoginEnabledChanged(bool enabled);

    signals:
        void passwordChanged();
        // Password login
        void loginEnabledChanged(bool enabled);
        // Remote login
        void remoteLoginEnabledChanged(bool enabled);
        void error(const QString &message);

    private:
        static const char *SERVICE_NAME;
        static const char *OBJECT_PATH;

    private:
        bool isPrivileged();

    private:
        PasswordManagerStore store;
        PasswordManagerSshd sshdManager;
        QTimer autoclose;
        QQueue<QDBusMessage> replyQueue;
};

#endif /* ORG_NEMO_PASSWORDMANAGER_H */
