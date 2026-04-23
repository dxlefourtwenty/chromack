#include "ChromackNotificationServer.h"

NotificationServer::NotificationServer(QObject *parent)
    : QObject(parent)
{
}

uint NotificationServer::Notify(const QString &appName,
                                uint replacesId,
                                const QString &icon,
                                const QString &summary,
                                const QString &body,
                                const QStringList &actions,
                                const QVariantMap &hints,
                                int timeout)
{
    const uint notificationId = replacesId > 0 ? replacesId : nextId_++;

    emit newNotification(notificationId,
                         appName,
                         summary,
                         body,
                         icon,
                         timeout,
                         actions,
                         hints);

    return notificationId;
}

QStringList NotificationServer::GetCapabilities() const
{
    return {
        QStringLiteral("body"),
        QStringLiteral("icon-static"),
        QStringLiteral("actions"),
        QStringLiteral("persistence")
    };
}

void NotificationServer::GetServerInformation(QString &name,
                                              QString &vendor,
                                              QString &version,
                                              QString &specVersion) const
{
    name = QStringLiteral("chromack");
    vendor = QStringLiteral("dxle");
    version = QStringLiteral("0.1.0");
    specVersion = QStringLiteral("1.3");
}

void NotificationServer::CloseNotification(uint id)
{
    emit closeRequested(id, 3);
}

void NotificationServer::handleNotificationClosed(uint id, uint reason)
{
    emit NotificationClosed(id, reason);
}

void NotificationServer::invokeAction(uint id, const QString &actionKey)
{
    emit ActionInvoked(id, actionKey);
}
