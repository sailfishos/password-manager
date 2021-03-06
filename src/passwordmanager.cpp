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


#include "passwordmanager.h"
#include "passwordmanageradaptor.h"
#include "passwordmanager_pam.h"
#include "passwordmanager_pwgen.h"

#include <QDBusConnection>


const char *
PasswordManager::SERVICE_NAME = "org.nemo.passwordmanager";

const char *
PasswordManager::OBJECT_PATH = "/org/nemo/passwordmanager";


PasswordManager::PasswordManager(QObject *parent)
    : QObject(parent)
    , QDBusContext()
    , store()
    , sshdManager(this)
    , autoclose()
{
    // Inactivity timeout after which the service quits to save resources
    const int AUTOCLOSE_TIMEOUT_MS = 60 * 1000;

    // Configure auto-close timer to quit the service after inactivity
    autoclose.setSingleShot(true);
    autoclose.setInterval(AUTOCLOSE_TIMEOUT_MS);
    QObject::connect(&autoclose, SIGNAL(timeout()),
                     this, SLOT(quit()));

    QDBusConnection connection = QDBusConnection::systemBus();
    if (!connection.registerObject(OBJECT_PATH, this)) {
        qFatal("Cannot register object at %s", OBJECT_PATH);
    }

    if (!connection.registerService(SERVICE_NAME)) {
        qFatal("Cannot register D-Bus service at %s", SERVICE_NAME);
    }

    connect(&sshdManager, &PasswordManagerSshd::remoteLoginEnabledChanged,
            this, &PasswordManager::handleRemoteLoginEnabledChanged);

    new PasswordManagerAdaptor(this);

    // Every time a client action is carried out, we call autoclose.start() to
    // reset the timer. Do it here for the first time.
    autoclose.start();
}

PasswordManager::~PasswordManager()
{
}

void
PasswordManager::generatePassword()
{
    if (!isPrivileged()) return;

    bool passwordEnabled = isLoginEnabled();
    QString password = PasswordManagerPwGen::generate();

    QString message;
    if (PasswordManagerPAM::set(password, &message)) {
        if (!(store.set(password))) {
            emit error("Could not save password");
        }
        emit passwordChanged();
        if (!passwordEnabled) {
            emit loginEnabledChanged(true);
        }
    } else {
        emit error(message);
    }

    autoclose.start();
}

QString
PasswordManager::getGeneratedPassword()
{
    if (!isPrivileged()) return QString();

    autoclose.start();
    return store.get();
}

void
PasswordManager::setPassword(const QString &password)
{
    if (!isPrivileged()) return;

    bool passwordEnabled = isLoginEnabled();

    QString message;
    if (PasswordManagerPAM::set(password, &message)) {
        if (password == "") {
            store.disablePassword();
        } else {
            store.set("");
        }
        emit passwordChanged();
        if (!passwordEnabled && password != "") {
            emit loginEnabledChanged(true);
        } else if (passwordEnabled && password == "") {
            emit loginEnabledChanged(false);
        }
    } else {
        emit error(message);
    }

    autoclose.start();
}

bool
PasswordManager::isLoginEnabled()
{
    // No privileges needed for password enabled check

    return store.isPasswordEnabled();
}

void
PasswordManager::setRemoteLoginEnabled(bool enabled)
{
    if (!isPrivileged()) return;

    RemoteLoginState state = sshdManager.isRemoteLoginEnabled();
    if (state != Unknown && (state == Enabled) == enabled)
        return;

    if (enabled)
        sshdManager.enableRemoteLogin();
    else
        sshdManager.disableRemoteLogin();

    autoclose.start();
}

bool
PasswordManager::isRemoteLoginEnabled()
{
    // No privileges needed for remote login enabled check

    autoclose.start();
    RemoteLoginState state = sshdManager.isRemoteLoginEnabled();
    if (state == Unknown) {
        // Still querying for status, delay the reply
        setDelayedReply(true);
        replyQueue.enqueue(message().createReply());
        return false;
    }
    return state == Enabled;
}

void
PasswordManager::quit()
{
    if (!isPrivileged()) return;

    QCoreApplication::quit();
}

void
PasswordManager::handleRemoteLoginEnabledChanged(bool enabled)
{
    while (!replyQueue.isEmpty()) {
        QDBusMessage reply = replyQueue.dequeue();
        reply << enabled;
        QDBusConnection::systemBus().send(reply);
    }
    emit remoteLoginEnabledChanged(enabled);
}

bool
PasswordManager::isPrivileged()
{
    if (!calledFromDBus()) {
        // Local function calls are always privileged
        return true;
    }

    // Get the PID of the calling process
    pid_t pid = connection().interface()->servicePid(message().service());

    // The /proc/<pid> directory is owned by EUID:EGID of the process
    QFileInfo info(QString("/proc/%1").arg(pid));
    if (info.group() != "privileged" && info.owner() != "root") {
        sendErrorReply(QDBusError::AccessDenied,
                QString("PID %1 is not in privileged group").arg(pid));
        return false;
    }

    return true;
}
