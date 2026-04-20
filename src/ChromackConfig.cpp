#include "ChromackConfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <unistd.h>

namespace {

QString expandPath(QString value)
{
    if (value == QStringLiteral("~")) {
        value = QDir::homePath();
    } else if (value.startsWith(QStringLiteral("~/"))) {
        value = QDir::homePath() + value.mid(1);
    }

    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const auto runtimeDirFallback = []() {
        const QString runUserPath = QStringLiteral("/run/user/%1").arg(static_cast<qulonglong>(::getuid()));
        if (QDir(runUserPath).exists()) {
            return runUserPath;
        }

        const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
        if (!runtime.isEmpty()) {
            return runtime;
        }

        return QDir::tempPath();
    };

    static const QRegularExpression bracedPattern(QStringLiteral(R"(\$\{([A-Za-z_][A-Za-z0-9_]*)\})"));
    QRegularExpressionMatchIterator bracedMatches = bracedPattern.globalMatch(value);
    while (bracedMatches.hasNext()) {
        const QRegularExpressionMatch match = bracedMatches.next();
        const QString key = match.captured(1);
        QString replacement = env.value(key);
        if (replacement.isEmpty() && key == QStringLiteral("XDG_RUNTIME_DIR")) {
            replacement = runtimeDirFallback();
        }
        value.replace(match.captured(0), replacement);
    }

    static const QRegularExpression simplePattern(QStringLiteral(R"(\$([A-Za-z_][A-Za-z0-9_]*))"));
    QRegularExpressionMatchIterator simpleMatches = simplePattern.globalMatch(value);
    while (simpleMatches.hasNext()) {
        const QRegularExpressionMatch match = simpleMatches.next();
        const QString key = match.captured(1);
        QString replacement = env.value(key);
        if (replacement.isEmpty() && key == QStringLiteral("XDG_RUNTIME_DIR")) {
            replacement = runtimeDirFallback();
        }
        value.replace(match.captured(0), replacement);
    }

    return value;
}

QString normalizePath(const QString &path)
{
    const QString expanded = expandPath(path);
    const QFileInfo info(expanded);
    return info.exists() ? info.canonicalFilePath() : info.absoluteFilePath();
}

QString configDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/chromack");
}

QString legacyConfigDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/wardnc");
}

QString defaultConfigContents()
{
    return QStringLiteral(
        "# chromack config\n"
        "# paths accept ~, $VAR, and ${VAR}\n"
        "\n"
        "[layout]\n"
        "anchor = \"top-right\"\n"
        "screen = \"auto\"\n"
        "width = 420\n"
        "top_margin = 24\n"
        "right_margin = 24\n"
        "bottom_margin = 24\n"
        "left_margin = 24\n"
        "minimum_height = 360\n"
        "above_windows = true\n"
        "focusable = true\n"
        "\n"
        "[panel]\n"
        "start_open = false\n"
        "close_on_escape = true\n"
        "show_header = true\n"
        "show_footer = true\n"
        "title = \"Chromack\"\n"
        "footer_text = \"style color picker\"\n"
        "\n"
        "[animation]\n"
        "enabled = true\n"
        "duration_ms = 220\n"
        "fade = true\n"
        "fade_duration_ms = 160\n"
        "slide_distance = 24\n"
        "slide_direction = \"right\"\n"
        "easing = \"out-cubic\"\n"
        "\n"
        "[paths]\n"
        "style_css = \"~/.config/chromack/style.css\"\n"
        "colors_css = \"~/.config/chromack/colors.css\"\n"
        "state_file = \"${XDG_RUNTIME_DIR}/chromack/panel.state\"\n");
}

QString defaultColorsContents()
{
    return QStringLiteral(
        ":root {\n"
        "    --color-primary: #f26f63;\n"
        "    --color-secondary: #4a9dff;\n"
        "    --color-accent: #ffd166;\n"
        "    --color-surface: #111827;\n"
        "    --color-surface-alt: #1f2937;\n"
        "    --color-border: #374151;\n"
        "    --color-text: #f3f4f6;\n"
        "    --color-muted: #9ca3af;\n"
        "    --color-success: #34d399;\n"
        "    --color-warning: #f59e0b;\n"
        "    --color-danger: #ef4444;\n"
        "}\n");
}

QString defaultStyleContents()
{
    return QStringLiteral(
        "@import \"./colors.css\";\n"
        "\n"
        ":root {\n"
        "    --font-family: \"Noto Sans\";\n"
        "    --panel-radius: 16px;\n"
        "    --panel-border-width: 1px;\n"
        "    --panel-padding: 14px;\n"
        "    --section-gap: 10px;\n"
        "    --row-height: 34px;\n"
        "    --row-radius: 8px;\n"
        "    --preview-size: 18px;\n"
        "}\n"
        "\n"
        "QWidget#chromackWindow {\n"
        "    background: transparent;\n"
        "}\n"
        "\n"
        "QWidget#chromackWindow,\n"
        "QWidget#chromackWindow * {\n"
        "    font-family: var(--font-family);\n"
        "}\n"
        "\n"
        "QFrame#panelRoot {\n"
        "    background: var(--color-surface);\n"
        "    border: var(--panel-border-width) solid var(--color-border);\n"
        "    border-radius: var(--panel-radius);\n"
        "}\n"
        "\n"
        "QFrame#headerBar,\n"
        "QFrame#footerBar,\n"
        "QWidget#pickerViewport,\n"
        "QWidget#pickerContainer {\n"
        "    background: var(--color-surface-alt);\n"
        "    border-radius: 10px;\n"
        "}\n"
        "\n"
        "QLabel#headerTitle,\n"
        "QLabel#footerLabel,\n"
        "QLabel#colorNameLabel {\n"
        "    color: var(--color-text);\n"
        "}\n"
        "\n"
        "QLabel#colorValueLabel {\n"
        "    color: var(--color-muted);\n"
        "}\n"
        "\n"
        "QLineEdit#colorInput {\n"
        "    color: var(--color-text);\n"
        "    background: var(--color-surface);\n"
        "    border: 1px solid var(--color-border);\n"
        "    border-radius: var(--row-radius);\n"
        "    padding: 0 8px;\n"
        "}\n"
        "\n"
        "QPushButton#applyButton,\n"
        "QPushButton#reloadButton,\n"
        "QPushButton#closeButton {\n"
        "    background: var(--color-primary);\n"
        "    color: var(--color-surface);\n"
        "    border: none;\n"
        "    border-radius: 8px;\n"
        "    padding: 0 10px;\n"
        "    min-height: 28px;\n"
        "    font-weight: 700;\n"
        "}\n"
        "\n"
        "QPushButton#reloadButton {\n"
        "    background: var(--color-secondary);\n"
        "}\n"
        "\n"
        "QPushButton#closeButton {\n"
        "    background: var(--color-danger);\n"
        "    color: var(--color-text);\n"
        "}\n"
        "\n"
        "QFrame#previewSwatch {\n"
        "    border: 1px solid var(--color-border);\n"
        "    border-radius: 6px;\n"
        "}\n");
}

QString stripInlineComment(const QString &line)
{
    bool insideQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line.at(i);
        if (c == '"') {
            insideQuotes = !insideQuotes;
            continue;
        }
        if (!insideQuotes && c == '#') {
            return line.left(i).trimmed();
        }
    }

    return line.trimmed();
}

bool parseString(const QString &raw, QString *out)
{
    const QString value = stripInlineComment(raw).trimmed();
    if (value.isEmpty()) {
        return false;
    }

    if (value.startsWith('"') && value.endsWith('"') && value.size() >= 2) {
        *out = value.mid(1, value.size() - 2);
        return true;
    }

    *out = value;
    return true;
}

bool parseInt(const QString &raw, int *out)
{
    bool ok = false;
    const int value = stripInlineComment(raw).trimmed().toInt(&ok);
    if (!ok) {
        return false;
    }
    *out = value;
    return true;
}

bool parseBool(const QString &raw, bool *out)
{
    const QString value = stripInlineComment(raw).trimmed().toLower();
    if (value == QStringLiteral("true")) {
        *out = true;
        return true;
    }
    if (value == QStringLiteral("false")) {
        *out = false;
        return true;
    }
    return false;
}

QString readTextFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

QString resolveImports(const QString &styleText, const QString &baseDir, QSet<QString> *visited)
{
    QString output;
    static const QRegularExpression importPattern(
        QStringLiteral(R"(@import\s+\"([^\"]+)\"\s*;)"));

    int cursor = 0;
    QRegularExpressionMatchIterator it = importPattern.globalMatch(styleText);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        output += styleText.mid(cursor, match.capturedStart() - cursor);

        const QString importPath = match.captured(1).trimmed();
        const QString resolved = normalizePath(QDir(baseDir).absoluteFilePath(importPath));
        if (!visited->contains(resolved)) {
            visited->insert(resolved);
            const QString importedText = readTextFile(resolved);
            if (!importedText.isEmpty()) {
                output += resolveImports(importedText, QFileInfo(resolved).absolutePath(), visited);
                output += QLatin1Char('\n');
            }
        }

        cursor = match.capturedEnd();
    }

    output += styleText.mid(cursor);
    return output;
}

QHash<QString, QString> extractVariables(const QString &text)
{
    QHash<QString, QString> variables;
    static const QRegularExpression pattern(
        QStringLiteral(R"((--[A-Za-z0-9_-]+)\s*:\s*([^;{}]+);)"));
    QRegularExpressionMatchIterator it = pattern.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        variables.insert(match.captured(1).trimmed(), match.captured(2).trimmed());
    }
    return variables;
}

QString substituteVariables(QString text, const QHash<QString, QString> &variables)
{
    static const QRegularExpression pattern(
        QStringLiteral(R"(var\(\s*(--[A-Za-z0-9_-]+)\s*(?:,\s*([^\)]+))?\))"));

    for (int i = 0; i < 4; ++i) {
        int cursor = 0;
        QString resolved;
        QRegularExpressionMatchIterator it = pattern.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            resolved += text.mid(cursor, match.capturedStart() - cursor);
            const QString key = match.captured(1).trimmed();
            const QString fallback = match.captured(2).trimmed();
            resolved += variables.value(key, fallback.isEmpty() ? match.captured(0) : fallback);
            cursor = match.capturedEnd();
        }
        resolved += text.mid(cursor);
        if (resolved == text) {
            break;
        }
        text = resolved;
    }

    return text;
}

QString stripVariableDeclarations(const QString &text)
{
    QString output = text;
    static const QRegularExpression declarationPattern(
        QStringLiteral(R"((^|[\n\r])[\t ]*--[A-Za-z0-9_-]+\s*:\s*[^;{}]+;[\t ]*)"));
    output.replace(declarationPattern, QStringLiteral("\\1"));
    return output;
}

void copyIfMissing(const QString &from, const QString &to)
{
    if (QFile::exists(to) || !QFile::exists(from)) {
        return;
    }

    QFile::copy(from, to);
}

void loadConfig(const QString &path, ChromackConfig *config)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&file);
    QString section;

    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        if (line.startsWith('[') && line.endsWith(']')) {
            section = line.mid(1, line.size() - 2).trimmed().toLower();
            continue;
        }

        const int separator = line.indexOf('=');
        if (separator <= 0) {
            continue;
        }

        const QString key = line.left(separator).trimmed().toLower();
        const QString value = line.mid(separator + 1).trimmed();

        if (section == QStringLiteral("layout")) {
            if (key == QStringLiteral("anchor")) {
                parseString(value, &config->layout.anchor);
            } else if (key == QStringLiteral("screen")) {
                parseString(value, &config->layout.screen);
            } else if (key == QStringLiteral("width")) {
                parseInt(value, &config->layout.width);
            } else if (key == QStringLiteral("top_margin")) {
                parseInt(value, &config->layout.topMargin);
            } else if (key == QStringLiteral("right_margin")) {
                parseInt(value, &config->layout.rightMargin);
            } else if (key == QStringLiteral("bottom_margin")) {
                parseInt(value, &config->layout.bottomMargin);
            } else if (key == QStringLiteral("left_margin")) {
                parseInt(value, &config->layout.leftMargin);
            } else if (key == QStringLiteral("minimum_height")) {
                parseInt(value, &config->layout.minimumHeight);
            } else if (key == QStringLiteral("above_windows")) {
                parseBool(value, &config->layout.aboveWindows);
            } else if (key == QStringLiteral("focusable")) {
                parseBool(value, &config->layout.focusable);
            }
        } else if (section == QStringLiteral("panel")) {
            if (key == QStringLiteral("start_open")) {
                parseBool(value, &config->panel.startOpen);
            } else if (key == QStringLiteral("close_on_escape")) {
                parseBool(value, &config->panel.closeOnEscape);
            } else if (key == QStringLiteral("show_header")) {
                parseBool(value, &config->panel.showHeader);
            } else if (key == QStringLiteral("show_footer")) {
                parseBool(value, &config->panel.showFooter);
            } else if (key == QStringLiteral("title")) {
                parseString(value, &config->panel.title);
            } else if (key == QStringLiteral("footer_text")) {
                parseString(value, &config->panel.footerText);
            }
        } else if (section == QStringLiteral("animation")) {
            if (key == QStringLiteral("enabled")) {
                parseBool(value, &config->animation.enabled);
            } else if (key == QStringLiteral("duration_ms")) {
                parseInt(value, &config->animation.durationMs);
            } else if (key == QStringLiteral("fade")) {
                parseBool(value, &config->animation.fade);
            } else if (key == QStringLiteral("fade_duration_ms")) {
                parseInt(value, &config->animation.fadeDurationMs);
            } else if (key == QStringLiteral("slide_distance")) {
                parseInt(value, &config->animation.slideDistance);
            } else if (key == QStringLiteral("slide_direction")) {
                parseString(value, &config->animation.slideDirection);
            } else if (key == QStringLiteral("easing")) {
                parseString(value, &config->animation.easing);
            }
        } else if (section == QStringLiteral("paths")) {
            if (key == QStringLiteral("style_css")) {
                parseString(value, &config->paths.styleCss);
            } else if (key == QStringLiteral("colors_css")) {
                parseString(value, &config->paths.colorsCss);
            } else if (key == QStringLiteral("state_file")) {
                parseString(value, &config->paths.stateFile);
            }
        }
    }

    config->layout.width = qMax(220, config->layout.width);
    config->layout.minimumHeight = qMax(220, config->layout.minimumHeight);
    config->layout.topMargin = qMax(0, config->layout.topMargin);
    config->layout.rightMargin = qMax(0, config->layout.rightMargin);
    config->layout.bottomMargin = qMax(0, config->layout.bottomMargin);
    config->layout.leftMargin = qMax(0, config->layout.leftMargin);

    config->animation.durationMs = qMax(0, config->animation.durationMs);
    config->animation.fadeDurationMs = qMax(0, config->animation.fadeDurationMs);
    config->animation.slideDistance = qMax(0, config->animation.slideDistance);

    const QString direction = config->animation.slideDirection.trimmed().toLower();
    if (direction != QStringLiteral("right") && direction != QStringLiteral("left") &&
        direction != QStringLiteral("auto")) {
        config->animation.slideDirection = QStringLiteral("right");
    }
}

QString resolvedStylePath(const ChromackConfig &config)
{
    return normalizePath(config.paths.styleCss);
}

QString resolvedColorsPath(const ChromackConfig &config)
{
    return normalizePath(config.paths.colorsCss);
}

} // namespace

ChromackConfigLoader::ChromackConfigLoader(QObject *parent)
    : QObject(parent)
{
    ensureConfigFiles();
    connect(&watcher_, &QFileSystemWatcher::fileChanged, this, &ChromackConfigLoader::reload);
    connect(&watcher_, &QFileSystemWatcher::directoryChanged, this, &ChromackConfigLoader::reload);
    reload();
}

const ChromackConfig &ChromackConfigLoader::config() const
{
    return config_;
}

const QString &ChromackConfigLoader::styleSheet() const
{
    return styleSheet_;
}

const QHash<QString, QString> &ChromackConfigLoader::styleVariables() const
{
    return styleVariables_;
}

QString ChromackConfigLoader::configPath() const
{
    return configDirectory() + QStringLiteral("/config.toml");
}

void ChromackConfigLoader::reload()
{
    ChromackConfig loaded;
    loadConfig(configPath(), &loaded);
    config_ = loaded;

    const QString stylePath = resolvedStylePath(config_);
    const QString styleText = readTextFile(stylePath);

    QSet<QString> visited;
    visited.insert(stylePath);

    QString resolvedText = resolveImports(styleText, QFileInfo(stylePath).absolutePath(), &visited);
    if (resolvedText.trimmed().isEmpty()) {
        resolvedText = defaultColorsContents() + QStringLiteral("\n") + defaultStyleContents();
    }

    styleVariables_ = extractVariables(resolvedText);
    QString flatStyle = stripVariableDeclarations(resolvedText);
    styleSheet_ = substituteVariables(flatStyle, styleVariables_);

    emit configChanged(config_);
    emit styleChanged(styleSheet_, styleVariables_);
    refreshWatchers();
}

void ChromackConfigLoader::ensureConfigFiles()
{
    const QString cfgDir = configDirectory();
    const QString oldDir = legacyConfigDirectory();

    QDir().mkpath(cfgDir);

    copyIfMissing(oldDir + QStringLiteral("/config.toml"), cfgDir + QStringLiteral("/config.toml"));
    copyIfMissing(oldDir + QStringLiteral("/style.css"), cfgDir + QStringLiteral("/style.css"));
    copyIfMissing(oldDir + QStringLiteral("/colors.css"), cfgDir + QStringLiteral("/colors.css"));

    const QString configFile = cfgDir + QStringLiteral("/config.toml");
    const QString styleFile = cfgDir + QStringLiteral("/style.css");
    const QString colorsFile = cfgDir + QStringLiteral("/colors.css");

    if (!QFile::exists(configFile)) {
        QFile file(configFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(defaultConfigContents().toUtf8());
        }
    }

    if (!QFile::exists(styleFile)) {
        QFile file(styleFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(defaultStyleContents().toUtf8());
        }
    }

    if (!QFile::exists(colorsFile)) {
        QFile file(colorsFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(defaultColorsContents().toUtf8());
        }
    }
}

void ChromackConfigLoader::refreshWatchers()
{
    if (!watcher_.files().isEmpty()) {
        watcher_.removePaths(watcher_.files());
    }
    if (!watcher_.directories().isEmpty()) {
        watcher_.removePaths(watcher_.directories());
    }

    const QString configFile = normalizePath(configPath());
    const QString styleFile = resolvedStylePath(config_);
    const QString colorsFile = resolvedColorsPath(config_);

    const QStringList files = {configFile, styleFile, colorsFile};
    QStringList dirs = {
        QFileInfo(configFile).absolutePath(),
        QFileInfo(styleFile).absolutePath(),
        QFileInfo(colorsFile).absolutePath(),
        configDirectory()
    };

    for (const QString &path : files) {
        if (QFileInfo::exists(path)) {
            watcher_.addPath(path);
        }
    }

    dirs.removeDuplicates();
    for (const QString &path : dirs) {
        if (QDir(path).exists()) {
            watcher_.addPath(path);
        }
    }
}
