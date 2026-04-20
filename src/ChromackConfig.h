#pragma once

#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QString>

struct ChromackLayoutConfig {
    QString anchor = QStringLiteral("top-right");
    QString screen = QStringLiteral("auto");
    int width = 420;
    int topMargin = 24;
    int rightMargin = 24;
    int bottomMargin = 24;
    int leftMargin = 24;
    int minimumHeight = 360;
    bool aboveWindows = true;
    bool focusable = true;
};

struct ChromackPanelConfig {
    bool startOpen = false;
    bool closeOnEscape = true;
    bool showHeader = true;
    bool showFooter = true;
    QString title = QStringLiteral("Chromack");
    QString footerText = QStringLiteral("style color picker");
    QString scrollbar = QStringLiteral("auto");
};

struct ChromackAnimationConfig {
    bool enabled = true;
    int durationMs = 220;
    bool fade = true;
    int fadeDurationMs = 160;
    int slideDistance = 24;
    QString slideDirection = QStringLiteral("right");
    QString easing = QStringLiteral("out-cubic");
};

struct ChromackPathsConfig {
    QString styleCss = QStringLiteral("~/.config/chromack/style.css");
    QString colorsCss = QStringLiteral("~/.config/chromack/colors.css");
    QString stateFile = QStringLiteral("$XDG_RUNTIME_DIR/chromack/panel.state");
};

struct ChromackConfig {
    ChromackLayoutConfig layout;
    ChromackPanelConfig panel;
    ChromackAnimationConfig animation;
    ChromackPathsConfig paths;
};

class ChromackConfigLoader : public QObject {
    Q_OBJECT

public:
    explicit ChromackConfigLoader(QObject *parent = nullptr);

    const ChromackConfig &config() const;
    const QString &styleSheet() const;
    const QHash<QString, QString> &styleVariables() const;
    QString configPath() const;

public slots:
    void reload();

signals:
    void configChanged(const ChromackConfig &config);
    void styleChanged(const QString &styleSheet, const QHash<QString, QString> &styleVariables);

private:
    void ensureConfigFiles();
    void refreshWatchers();

    QFileSystemWatcher watcher_;
    ChromackConfig config_;
    QString styleSheet_;
    QHash<QString, QString> styleVariables_;
};
