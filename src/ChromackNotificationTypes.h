#pragma once

#include <QDBusVariant>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

struct NotificationRequest {
    uint id = 0;
    QString appName;
    QString iconName;
    QString summary;
    QString body;
    int timeoutMs = -1;
    QStringList actions;
    QVariantMap hints;
};

inline QVariant unwrapHintValue(const QVariant &value)
{
    if (value.canConvert<QDBusVariant>()) {
        return value.value<QDBusVariant>().variant();
    }

    return value;
}

inline int notificationUrgency(const QVariantMap &hints)
{
    const QVariant urgency = unwrapHintValue(hints.value(QStringLiteral("urgency")));
    return urgency.isValid() ? urgency.toInt() : 1;
}

inline bool notificationHintBool(const QVariantMap &hints,
                                 const QStringList &keys,
                                 bool fallback = false)
{
    for (const QString &key : keys) {
        const QVariant value = unwrapHintValue(hints.value(key));
        if (!value.isValid()) {
            continue;
        }

        if (value.metaType().id() == QMetaType::Bool) {
            return value.toBool();
        }

        if (value.canConvert<int>()) {
            return value.toInt() != 0;
        }

        const QString lowered = value.toString().trimmed().toLower();
        if (lowered == QStringLiteral("true") || lowered == QStringLiteral("yes") || lowered == QStringLiteral("on")) {
            return true;
        }

        if (lowered == QStringLiteral("false") || lowered == QStringLiteral("no") || lowered == QStringLiteral("off")) {
            return false;
        }
    }

    return fallback;
}

inline QString notificationStackTag(const QVariantMap &hints)
{
    const QStringList keys = {
        QStringLiteral("x-dunst-stack-tag"),
        QStringLiteral("x-dunst-stack_tag"),
        QStringLiteral("x-canonical-private-synchronous"),
        QStringLiteral("x-canonical-private_synchronous"),
        QStringLiteral("private-synchronous"),
        QStringLiteral("private_synchronous")
    };

    for (const QString &key : keys) {
        const QString value = unwrapHintValue(hints.value(key)).toString().trimmed();
        if (!value.isEmpty()) {
            return value;
        }
    }

    return {};
}

inline bool notificationIsTransient(const QVariantMap &hints)
{
    return notificationHintBool(hints,
                                {
                                    QStringLiteral("transient"),
                                    QStringLiteral("x-canonical-private-synchronous")
                                },
                                false);
}
