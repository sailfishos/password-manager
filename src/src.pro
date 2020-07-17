TEMPLATE=app
TARGET=nemo-password-manager

QT += dbus
QT -= gui

LIBS += -lpam

CONFIG += link_pkgconfig
PKGCONFIG += libshadowutils

# Uncomment the following line for debugging output on stderr
#DEFINES += PASSWORDMANAGER_DEBUG

DEPENDPATH += . ..
INCLUDEPATH += . ..

SOURCES += main.cpp

SOURCES += passwordmanager.cpp
HEADERS += passwordmanager.h

SOURCES += passwordmanager_pam.cpp
HEADERS += passwordmanager_pam.h

SOURCES += passwordmanager_store.cpp
HEADERS += passwordmanager_store.h

SOURCES += passwordmanager_pwgen.cpp
HEADERS += passwordmanager_pwgen.h

SOURCES += passwordmanager_sshd.cpp
HEADERS += passwordmanager_sshd.h


# Generated at qmake time by ../password-manager.pro
SOURCES += passwordmanageradaptor.cpp
HEADERS += passwordmanageradaptor.h


# Well-known name for our D-Bus service
DBUS_SERVICE_NAME=org.nemo.passwordmanager


# Installation
systemd.files = ../systemd/dbus-$${DBUS_SERVICE_NAME}.service
systemd.path = /usr/lib/systemd/system/

service.files = ../dbus/$${DBUS_SERVICE_NAME}.service
service.path = /usr/share/dbus-1/system-services/

conf.files = ../dbus/$${DBUS_SERVICE_NAME}.conf
conf.path = /etc/dbus-1/system.d/

target.path = /usr/bin/

INSTALLS += target systemd service conf
