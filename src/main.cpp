#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>
#include <QDir>
#include <QIODevice>
#include <QLockFile>
#include <QProcessEnvironment>
#include <QTextStream>
#include <QStandardPaths>

#include "ChromackConfig.h"
#include "ChromackControl.h"
#include "ChromackPanel.h"

namespace {

constexpr auto kControlService = "org.dxle.chromack";
constexpr auto kControlPath = "/org/dxle/chromack";
constexpr auto kControlInterface = "org.dxle.chromack.Control";

QString instanceLockPath()
{
    const QString runtimePath = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (!runtimePath.isEmpty()) {
        return runtimePath + QStringLiteral("/chromack.lock");
    }

    return QDir::tempPath() + QStringLiteral("/chromack.lock");
}

int callControlMethod(const QDBusConnection &bus, const QString &method)
{
    QDBusInterface interface(kControlService, kControlPath, kControlInterface, bus);
    if (!interface.isValid()) {
        qCritical() << "No running chromack instance found.";
        return 1;
    }

    const QDBusReply<void> reply = interface.call(method);
    if (!reply.isValid()) {
        qCritical() << "Failed to call" << method << "on running chromack:" << reply.error().message();
        return 1;
    }

    return 0;
}

int callControlMethodWithArgument(const QDBusConnection &bus, const QString &method, const QString &argument)
{
    QDBusInterface interface(kControlService, kControlPath, kControlInterface, bus);
    if (!interface.isValid()) {
        qCritical() << "No running chromack instance found.";
        return 1;
    }

    const QDBusReply<void> reply = interface.call(method, argument);
    if (!reply.isValid()) {
        qCritical() << "Failed to call" << method << "on running chromack:" << reply.error().message();
        return 1;
    }

    return 0;
}

bool registerService(const QDBusConnection &bus,
                     const QString &serviceName,
                     const QString &failureMessage,
                     QDBusConnectionInterface::ServiceQueueOptions queueOptions,
                     QDBusConnectionInterface::ServiceReplacementOptions replacementOptions)
{
    const auto registration = bus.interface()->registerService(
        serviceName,
        queueOptions,
        replacementOptions);

    if (!registration.isValid()) {
        qCritical() << failureMessage << registration.error().message();
        return false;
    }

    if (registration.value() == QDBusConnectionInterface::ServiceRegistered) {
        return true;
    }

    if (registration.value() == QDBusConnectionInterface::ServiceQueued) {
        qWarning() << failureMessage << serviceName
                   << "(service queued behind existing owner)";
        return false;
    }

    qCritical() << failureMessage << serviceName;
    return false;
}

bool registerObject(QDBusConnection &bus, const QString &path, QObject *object)
{
    if (bus.registerObject(path,
                           object,
                           QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals)) {
        return true;
    }

    qCritical() << "Failed to register D-Bus object at" << path << ":" << bus.lastError().message();
    return false;
}

} // namespace

int main(int argc, char *argv[])
{
    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const bool hasQtPlatformOverride = environment.contains(QStringLiteral("QT_QPA_PLATFORM"));
    const bool hasWaylandDisplay = environment.contains(QStringLiteral("WAYLAND_DISPLAY"));
    const bool hasWaylandShellIntegrationOverride =
        environment.contains(QStringLiteral("QT_WAYLAND_SHELL_INTEGRATION"));

    if (!hasQtPlatformOverride && hasWaylandDisplay) {
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("wayland"));
    }
    if (!hasWaylandShellIntegrationOverride && hasWaylandDisplay) {
        qputenv("QT_WAYLAND_SHELL_INTEGRATION", QByteArrayLiteral("layer-shell"));
    }

    QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("chromack color picker daemon"));
    parser.addHelpOption();

    QCommandLineOption reloadOption(QStringList() << QStringLiteral("reload"),
                                    QStringLiteral("Reload a running chromack instance configuration."));
    QCommandLineOption openOption(QStringList() << QStringLiteral("open"),
                                  QStringLiteral("Open a running chromack panel."));
    QCommandLineOption closeOption(QStringList() << QStringLiteral("close"),
                                   QStringLiteral("Close a running chromack panel."));
    QCommandLineOption toggleOption(QStringList() << QStringLiteral("toggle"),
                                    QStringLiteral("Toggle a running chromack panel."));
    QCommandLineOption setColorOption(QStringList() << QStringLiteral("set-color"),
                                      QStringLiteral("Set color on a running chromack instance."),
                                      QStringLiteral("color"));
    QCommandLineOption setColorStdinOption(QStringList() << QStringLiteral("set-color-stdin"),
                                           QStringLiteral("Read color from stdin and set it on a running chromack instance."));

    parser.addOption(reloadOption);
    parser.addOption(openOption);
    parser.addOption(closeOption);
    parser.addOption(toggleOption);
    parser.addOption(setColorOption);
    parser.addOption(setColorStdinOption);
    parser.process(app);

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qCritical() << "Failed to connect to the session D-Bus.";
        return 1;
    }

    if (parser.isSet(reloadOption)) {
        return callControlMethod(bus, QStringLiteral("Reload"));
    }
    if (parser.isSet(openOption)) {
        return callControlMethod(bus, QStringLiteral("Open"));
    }
    if (parser.isSet(closeOption)) {
        return callControlMethod(bus, QStringLiteral("Close"));
    }
    if (parser.isSet(toggleOption)) {
        return callControlMethod(bus, QStringLiteral("Toggle"));
    }
    if (parser.isSet(setColorOption)) {
        const QString color = parser.value(setColorOption).trimmed();
        if (color.isEmpty()) {
            qCritical() << "--set-color requires a non-empty color value.";
            return 1;
        }
        return callControlMethodWithArgument(bus, QStringLiteral("SetColor"), color);
    }
    if (parser.isSet(setColorStdinOption)) {
        QTextStream in(stdin, QIODevice::ReadOnly);
        const QString color = in.readAll().trimmed();
        if (color.isEmpty()) {
            qCritical() << "--set-color-stdin requires piped color input.";
            return 1;
        }
        return callControlMethodWithArgument(bus, QStringLiteral("SetColor"), color);
    }

    QLockFile instanceLock(instanceLockPath());
    instanceLock.setStaleLockTime(0);
    if (!instanceLock.tryLock()) {
        qCritical() << "chromack is already running.";
        return 1;
    }

    ChromackConfigLoader configLoader;
    ChromackPanel panel(&configLoader);
    ChromackControl control;

    if (!registerService(bus,
                         kControlService,
                         QStringLiteral("Failed to acquire chromack control service:"),
                         QDBusConnectionInterface::DontQueueService,
                         QDBusConnectionInterface::DontAllowReplacement)) {
        return 1;
    }

    if (!registerObject(bus, kControlPath, &control)) {
        return 1;
    }

    QObject::connect(&control, &ChromackControl::reloadRequested, &panel, &ChromackPanel::reloadConfiguration);
    QObject::connect(&control, &ChromackControl::openRequested, &panel, &ChromackPanel::openPanel);
    QObject::connect(&control, &ChromackControl::closeRequested, &panel, &ChromackPanel::closePanel);
    QObject::connect(&control, &ChromackControl::toggleRequested, &panel, &ChromackPanel::togglePanel);
    QObject::connect(&control, &ChromackControl::setColorRequested, &panel, &ChromackPanel::applyExternalColor);

    return app.exec();
}
