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
        "width = 520\n"
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
        "scrollbar = \"auto\"\n"
        "title = \"Chromack\"\n"
        "eyedropper_command = \"~/bin/launch-colorpicker\"\n"
        "reopen_delay_ms = 140\n"
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
        "material_css = \"~/.config/chromack/material.css\"\n"
        "state_file = \"${XDG_RUNTIME_DIR}/chromack/panel.state\"\n"
        "recent_colors_file = \"~/.cache/chromack/recent-colors.txt\"\n"
        "active_color_file = \"~/.cache/chromack/active-color.txt\"\n");
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
        "    --color-panel-backdrop: transparent;\n"
        "    --color-picker-backdrop: var(--color-surface);\n"
        "    --color-content-backdrop: var(--color-surface-alt);\n"
        "    --color-tab-bg: var(--color-surface);\n"
        "    --color-tab-active-bg: var(--color-surface-alt);\n"
        "    --color-tab-text: var(--color-muted);\n"
        "    --color-tab-active-text: var(--color-text);\n"
        "    --color-border: #374151;\n"
        "    --color-text: #f3f4f6;\n"
        "    --color-muted: #9ca3af;\n"
        "    --color-success: #34d399;\n"
        "    --color-warning: #f59e0b;\n"
        "    --color-danger: #ef4444;\n"
        "}\n");
}

QString defaultMaterialContents()
{
    return QStringLiteral(
        ":root {\n"
        "    --color-material-01: #f44336;\n"
        "    --color-material-02: #e91e63;\n"
        "    --color-material-03: #9c27b0;\n"
        "    --color-material-04: #673ab7;\n"
        "    --color-material-05: #3f51b5;\n"
        "    --color-material-06: #2196f3;\n"
        "    --color-material-07: #03a9f4;\n"
        "    --color-material-08: #00bcd4;\n"
        "    --color-material-09: #009688;\n"
        "    --color-material-10: #4caf50;\n"
        "    --color-material-11: #8bc34a;\n"
        "    --color-material-12: #cddc39;\n"
        "    --color-material-13: #ffeb3b;\n"
        "    --color-material-14: #ffc107;\n"
        "    --color-material-15: #ff9800;\n"
        "    --color-material-16: #ff5722;\n"
        "    --color-material-17: #ef5350;\n"
        "    --color-material-18: #ec407a;\n"
        "    --color-material-19: #ab47bc;\n"
        "    --color-material-20: #7e57c2;\n"
        "    --color-material-21: #5c6bc0;\n"
        "    --color-material-22: #42a5f5;\n"
        "    --color-material-23: #29b6f6;\n"
        "    --color-material-24: #26c6da;\n"
        "    --color-material-25: #26a69a;\n"
        "    --color-material-26: #66bb6a;\n"
        "    --color-material-27: #9ccc65;\n"
        "    --color-material-28: #d4e157;\n"
        "    --color-material-29: #ffee58;\n"
        "    --color-material-30: #ffca28;\n"
        "    --color-material-31: #ffa726;\n"
        "    --color-material-32: #ff7043;\n"
        "    --color-material-33: #d32f2f;\n"
        "    --color-material-34: #c2185b;\n"
        "    --color-material-35: #7b1fa2;\n"
        "    --color-material-36: #512da8;\n"
        "    --color-material-37: #303f9f;\n"
        "    --color-material-38: #1976d2;\n"
        "    --color-material-39: #0288d1;\n"
        "    --color-material-40: #0097a7;\n"
        "    --color-material-41: #00796b;\n"
        "    --color-material-42: #388e3c;\n"
        "    --color-material-43: #689f38;\n"
        "    --color-material-44: #afb42b;\n"
        "    --color-material-45: #f57f17;\n"
        "    --color-material-46: #ffffff;\n"
        "    --color-material-47: #9e9e9e;\n"
        "    --color-material-48: #000000;\n"
        "}\n");
}

QString defaultStyleContents()
{
    return QStringLiteral(R"(@import "./colors.css";
@import "./material.css";

:root {
    --font-family: "Noto Sans";
    --panel-radius: 16px;
    --panel-border-width: 1px;
    --panel-padding: 14px;
    --section-gap: 10px;

    --header-radius: 12px;
    --content-radius: 12px;
    --section-title-size: 13px;
    --title-size: 17px;
    --subtitle-size: 12px;

    --picker-height: 220px;
    --picker-radius: 10px;
    --hue-width: 22px;

    --swatch-size: 24px;
    --swatch-radius: 6px;
    --swatch-gap: 2px;

    --recent-swatch-size: 24px;
    --recent-swatch-radius: 6px;

    --input-height: 34px;
    --input-radius: 8px;

    --button-height: 34px;
    --button-radius: 10px;
    --button-padding-x: 12px;

    --preview-width: 74px;
    --preview-height: 34px;
    --preview-radius: 10px;

    --control-border-width: 1px;
    --control-border-color: var(--color-border);

    --header-bg: var(--color-surface-alt);
    --content-bg: var(--color-surface-alt);
    --content-backdrop-bg: var(--color-content-backdrop);
    --panel-bg: var(--color-surface);
    --panel-backdrop-bg: var(--color-panel-backdrop);
    --picker-backdrop-bg: var(--color-picker-backdrop);
    --palette-scroll-bg: var(--content-bg);

    --border-color: var(--control-border-color);
    --text-color: var(--text-main);
    --text-muted-color: var(--text-muted);
    --input-bg: var(--panel-bg);
    --tab-bg: var(--color-tab-bg);
    --tab-active-bg: var(--color-tab-active-bg);
    --tab-text-color: var(--color-tab-text);
    --tab-active-text-color: var(--color-tab-active-text);

    --text-main: var(--color-text);
    --text-muted: var(--color-muted);

}

QWidget#chromackWindow {
    background: var(--panel-backdrop-bg);
}

QWidget#chromackWindow,
QWidget#chromackWindow * {
    font-family: var(--font-family);
}

QFrame#panelRoot {
    background: var(--panel-bg);
    border: var(--panel-border-width) solid var(--color-border);
    border-radius: var(--panel-radius);
}

QFrame#headerBar {
    background: var(--header-bg);
    border-radius: var(--header-radius);
}

QLabel#headerTitle {
    color: var(--text-main);
    font-size: var(--title-size);
    font-weight: 700;
}

QLabel#headerSubtitle {
    color: var(--text-muted);
    font-size: var(--subtitle-size);
}

QPushButton#closeButton,
QPushButton#eyedropperButton {
    min-height: var(--button-height);
    min-width: var(--button-height);
    border: var(--control-border-width) solid var(--control-border-color);
    border-radius: var(--button-radius);
    background: transparent;
    color: var(--text-main);
    font-weight: 700;
}

QPushButton#eyedropperButton {
    font-family: "Symbols Nerd Font Mono", var(--font-family);
    font-size: 10.67px;
    font-weight: 500;
}

QScrollArea#pickerScrollArea {
    background: var(--content-backdrop-bg);
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}

QTabWidget#panelTabs {
    background: var(--panel-bg);
}

QTabWidget#panelTabs::tab-bar {
    alignment: left;
    top: 2px;
}

QTabWidget#panelTabs QTabBar {
    background: var(--panel-bg);
}

QTabWidget#panelTabs QTabBar::base {
    border: none;
    background: var(--panel-bg);
}

QTabWidget#panelTabs::pane {
    border: none;
    background: var(--content-bg);
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}

QTabWidget#panelTabs QTabBar::tab {
    min-height: 30px;
    padding: 0 12px;
    margin-bottom: -1px;
    border: none;
    border-top-left-radius: var(--input-radius);
    border-top-right-radius: var(--input-radius);
    background: var(--tab-bg, var(--panel-bg));
    color: var(--tab-text-color, var(--text-muted-color));
}

QTabWidget#panelTabs QTabBar::tab:selected {
    background: var(--tab-active-bg, var(--content-bg));
    color: var(--tab-active-text-color, var(--text-color));
}

QWidget#pickerViewport,
QWidget#pickerContainer {
    background: var(--content-bg);
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}

QWidget#paletteTab {
    background: var(--content-bg);
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}

QWidget#shadesTab {
    background: var(--content-bg);
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}

QScrollArea#paletteScrollArea {
    background: var(--palette-scroll-bg);
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}

QWidget#paletteViewport,
QWidget#paletteContainer {
    background: var(--palette-scroll-bg);
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}

QFrame#pickerTopFrame {
    background: var(--picker-backdrop-bg);
}

QWidget#svPicker {
    min-height: var(--picker-height);
    border: none;
    border-radius: var(--picker-radius);
}

QSlider#hueSlider {
    min-width: var(--hue-width);
    max-width: var(--hue-width);
    border: none;
    background: transparent;
    margin: 0;
    padding: 0;
}

QSlider#hueSlider::groove:vertical {
    border: none;
    border-radius: 6px;
    margin: 0;
    width: var(--hue-width);
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                stop:0 #ff0000,
                                stop:0.17 #ffff00,
                                stop:0.33 #00ff00,
                                stop:0.50 #00ffff,
                                stop:0.67 #0000ff,
                                stop:0.83 #ff00ff,
                                stop:1 #ff0000);
}

QSlider#hueSlider::add-page:vertical,
QSlider#hueSlider::sub-page:vertical {
    border: none;
    background: transparent;
    margin: 0;
}

QSlider#hueSlider::handle:vertical {
    background: #ffffff;
    border: 1px solid #111111;
    height: 12px;
    margin: -2px -2px;
    border-radius: 6px;
}

QLabel#sectionLabel {
    color: var(--text-main);
    font-size: var(--section-title-size);
    font-weight: 700;
}

QPushButton#materialSwatch {
    min-width: var(--swatch-size);
    max-width: var(--swatch-size);
    min-height: var(--swatch-size);
    max-height: var(--swatch-size);
    border: var(--control-border-width) solid var(--control-border-color);
    border-radius: var(--swatch-radius);
    margin: 0;
    padding: 0;
}

QPushButton#materialSwatch:checked {
    border: 2px solid var(--text-main);
}

QPushButton#recentSwatch {
    min-width: var(--recent-swatch-size);
    max-width: var(--recent-swatch-size);
    min-height: var(--recent-swatch-size);
    max-height: var(--recent-swatch-size);
    border: var(--control-border-width) solid var(--control-border-color);
    border-radius: var(--recent-swatch-radius);
    margin: 0;
    padding: 0;
}

QLabel#rowLabel {
    color: var(--text-main);
}

QSlider#opacitySlider::groove:horizontal {
    height: 10px;
    border-radius: 5px;
    border: var(--control-border-width) solid var(--control-border-color);
    background: #1b1d2a;
}

QSlider#opacitySlider::sub-page:horizontal {
    background: #cbbcff;
    border-radius: 5px;
}

QSlider#opacitySlider::handle:horizontal {
    width: 14px;
    margin: -3px 0;
    border-radius: 7px;
    border: 1px solid #111111;
    background: #ffffff;
}

QFrame#opacityPreview {
    min-width: var(--preview-width);
    min-height: var(--preview-height);
    border-radius: var(--preview-radius);
    border: var(--control-border-width) solid var(--control-border-color);
}

QLineEdit#hexInput,
QLineEdit#rgbaInput,
QLineEdit#paletteInput,
QFrame#paletteInputRow QLineEdit#paletteInput,
QLineEdit#paletteValueInput {
    min-height: var(--input-height);
    border: var(--control-border-width) solid var(--border-color);
    border-radius: var(--input-radius);
    padding: 0 10px;
    color: var(--text-color);
    background: var(--input-bg);
}

QPushButton#inlineCopyButton {
    min-width: var(--input-height);
    max-width: var(--input-height);
    min-height: var(--input-height);
    max-height: var(--input-height);
    border-radius: var(--input-radius);
    border: var(--control-border-width) solid var(--border-color);
    background: var(--input-bg);
    color: var(--text-color);
    padding: 0;
    font-size: 18px;
    font-weight: 600;
}

QPushButton#inlineCopyButton:hover {
    border-color: var(--text-color);
}

QPushButton#paletteGenerateButton {
    min-height: var(--input-height);
    border-radius: var(--input-radius);
    border: var(--control-border-width) solid var(--border-color);
    background: var(--input-bg);
    color: var(--text-color);
    padding: 0 10px;
}

QFrame#paletteInputRow QPushButton#paletteGenerateButton {
    min-height: var(--input-height);
    border-radius: var(--input-radius);
    border: var(--control-border-width) solid var(--border-color);
    background: var(--input-bg);
    color: var(--text-color);
    padding: 0 10px;
}

QPushButton#paletteGenerateButton:hover {
    border-color: var(--text-color);
}

QFrame#paletteInputRow QPushButton#paletteGenerateButton:hover {
    border-color: var(--text-color);
}

QPushButton#paletteSwatch {
    min-width: var(--preview-width);
    min-height: var(--preview-height);
    max-height: var(--preview-height);
    border-radius: var(--preview-radius);
    border: var(--control-border-width) solid var(--control-border-color);
    margin: 0;
    padding: 0;
}

QPushButton#paletteSwatch:hover {
    border-color: var(--text-color);
}

QFrame#shadeSwatchRow {
    background: transparent;
}

QPushButton#shadeSwatch {
    min-height: var(--input-height);
    border: var(--control-border-width) solid var(--control-border-color);
    border-radius: 0px;
    margin: 0;
    padding: 0;
}

QPushButton#shadeSwatch:hover {
    border-color: var(--text-color);
}

)");
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
    if (value.startsWith('\'') && value.endsWith('\'') && value.size() >= 2) {
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
            const QString importedText = readTextFile(resolved);
            if (!importedText.isEmpty()) {
                visited->insert(resolved);
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

void appendIfMissing(QString *style, const QString &marker, const QString &block)
{
    if (style->contains(marker)) {
        return;
    }
    if (!style->endsWith(QLatin1Char('\n'))) {
        *style += QLatin1Char('\n');
    }
    *style += block;
    if (!style->endsWith(QLatin1Char('\n'))) {
        *style += QLatin1Char('\n');
    }
}

QString ensurePaletteAndTabStyle(QString style)
{
    appendIfMissing(
        &style,
        QStringLiteral("QTabWidget#panelTabs::pane"),
        QStringLiteral(R"(
QTabWidget#panelTabs::pane {
    border: none;
    background: var(--content-bg);
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}

QTabWidget#panelTabs {
    background: var(--panel-bg);
}

QTabWidget#panelTabs::tab-bar {
    alignment: left;
    top: 2px;
}

QTabWidget#panelTabs QTabBar {
    background: var(--panel-bg);
}

QTabWidget#panelTabs QTabBar::base {
    border: none;
    background: var(--panel-bg);
}

QTabWidget#panelTabs QTabBar::tab {
    min-height: 30px;
    padding: 0 12px;
    margin-bottom: -1px;
    border: none;
    border-top-left-radius: var(--input-radius);
    border-top-right-radius: var(--input-radius);
    background: var(--tab-bg, var(--panel-bg));
    color: var(--tab-text-color, var(--text-muted-color));
}

QTabWidget#panelTabs QTabBar::tab:selected {
    background: var(--tab-active-bg, var(--content-bg));
    color: var(--tab-active-text-color, var(--text-color));
}

QWidget#paletteTab {
    background: var(--content-bg);
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}

QWidget#pickerViewport,
QWidget#pickerContainer {
    background: var(--content-bg);
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}

QScrollArea#pickerScrollArea {
    background: var(--content-backdrop-bg);
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}

QScrollArea#paletteScrollArea {
    background: var(--palette-scroll-bg, var(--content-bg));
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}

QWidget#paletteViewport,
QWidget#paletteContainer {
    background: var(--palette-scroll-bg, var(--content-bg));
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}
)")
    );

    appendIfMissing(
        &style,
        QStringLiteral("QLineEdit#paletteInput"),
        QStringLiteral(R"(
QLineEdit#paletteInput,
QFrame#paletteInputRow QLineEdit#paletteInput,
QLineEdit#paletteValueInput {
    min-height: var(--input-height);
    border: var(--control-border-width) solid var(--border-color);
    border-radius: var(--input-radius);
    padding: 0 10px;
    color: var(--text-color);
    background: var(--input-bg);
}
)")
    );

    appendIfMissing(
        &style,
        QStringLiteral("QPushButton#paletteGenerateButton"),
        QStringLiteral(R"(
QPushButton#paletteGenerateButton {
    min-height: var(--input-height);
    border-radius: var(--input-radius);
    border: var(--control-border-width) solid var(--border-color);
    background: var(--input-bg);
    color: var(--text-color);
    padding: 0 10px;
}

QFrame#paletteInputRow QPushButton#paletteGenerateButton {
    min-height: var(--input-height);
    border-radius: var(--input-radius);
    border: var(--control-border-width) solid var(--border-color);
    background: var(--input-bg);
    color: var(--text-color);
    padding: 0 10px;
}

QPushButton#paletteGenerateButton:hover {
    border-color: var(--text-color);
}

QFrame#paletteInputRow QPushButton#paletteGenerateButton:hover {
    border-color: var(--text-color);
}
)")
    );

    appendIfMissing(
        &style,
        QStringLiteral("QTabWidget#panelTabs::tab-bar"),
        QStringLiteral(R"(
QTabWidget#panelTabs::tab-bar {
    alignment: left;
    top: 2px;
}
)")
    );

    appendIfMissing(
        &style,
        QStringLiteral("QWidget#paletteTab"),
        QStringLiteral(R"(
QWidget#paletteTab {
    background: var(--content-bg);
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}
)")
    );

    appendIfMissing(
        &style,
        QStringLiteral("QWidget#shadesTab"),
        QStringLiteral(R"(
QWidget#shadesTab {
    background: var(--content-bg);
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}
)")
    );

    appendIfMissing(
        &style,
        QStringLiteral("QScrollArea#paletteScrollArea"),
        QStringLiteral(R"(
QScrollArea#paletteScrollArea {
    background: var(--palette-scroll-bg, var(--content-bg));
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}

QWidget#paletteViewport,
QWidget#paletteContainer {
    background: var(--palette-scroll-bg, var(--content-bg));
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}
)")
    );

    appendIfMissing(
        &style,
        QStringLiteral("QPushButton#shadeSwatch"),
        QStringLiteral(R"(
QFrame#shadeSwatchRow {
    background: transparent;
}

QPushButton#shadeSwatch {
    min-height: var(--input-height);
    border: var(--control-border-width) solid var(--control-border-color);
    border-radius: 0px;
    margin: 0;
    padding: 0;
}

QPushButton#shadeSwatch:hover {
    border-color: var(--text-color);
}
)")
    );

    appendIfMissing(
        &style,
        QStringLiteral("chromack-tabs-layout-v5"),
        QStringLiteral(R"(
/* chromack-tabs-layout-v5 */
QTabWidget#panelTabs::tab-bar {
    alignment: left;
    top: 2px;
}

QTabWidget#panelTabs QTabBar::tab {
    margin-bottom: -1px;
    border: none;
    background: var(--tab-bg, var(--panel-bg));
    color: var(--tab-text-color, var(--text-muted-color));
}

QTabWidget#panelTabs QTabBar::tab:selected {
    background: var(--tab-active-bg, var(--content-bg));
    color: var(--tab-active-text-color, var(--text-color));
}

QTabWidget#panelTabs::pane {
    border: none;
    background: var(--content-bg);
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}

QWidget#pickerViewport,
QWidget#pickerContainer,
QWidget#paletteTab,
QWidget#shadesTab {
    background: var(--content-bg);
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}

QScrollArea#pickerScrollArea {
    background: var(--content-backdrop-bg);
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: var(--content-radius);
    border-bottom-right-radius: var(--content-radius);
}
)")
    );

    return style;
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
            } else if (key == QStringLiteral("scrollbar")) {
                parseString(value, &config->panel.scrollbar);
            } else if (key == QStringLiteral("title")) {
                parseString(value, &config->panel.title);
            } else if (key == QStringLiteral("eyedropper_command")) {
                parseString(value, &config->panel.eyedropperCommand);
            } else if (key == QStringLiteral("reopen_delay_ms")) {
                parseInt(value, &config->panel.reopenDelayMs);
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
            } else if (key == QStringLiteral("material_css")) {
                parseString(value, &config->paths.materialCss);
            } else if (key == QStringLiteral("state_file")) {
                parseString(value, &config->paths.stateFile);
            } else if (key == QStringLiteral("recent_colors_file")) {
                parseString(value, &config->paths.recentColorsFile);
            } else if (key == QStringLiteral("active_color_file")) {
                parseString(value, &config->paths.activeColorFile);
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
    config->panel.reopenDelayMs = qMax(0, config->panel.reopenDelayMs);

    const QString direction = config->animation.slideDirection.trimmed().toLower();
    if (direction != QStringLiteral("right") && direction != QStringLiteral("left") &&
        direction != QStringLiteral("auto")) {
        config->animation.slideDirection = QStringLiteral("right");
    }

    const QString scrollbar = config->panel.scrollbar.trimmed().toLower();
    if (scrollbar != QStringLiteral("auto") &&
        scrollbar != QStringLiteral("none") &&
        scrollbar != QStringLiteral("always")) {
        config->panel.scrollbar = QStringLiteral("auto");
    } else {
        config->panel.scrollbar = scrollbar;
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

QString resolvedMaterialPath(const ChromackConfig &config)
{
    return normalizePath(config.paths.materialCss);
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
    const QString colorsPath = resolvedColorsPath(config_);
    const QString materialPath = resolvedMaterialPath(config_);
    const QString styleText = readTextFile(stylePath);

    QSet<QString> visited;
    visited.insert(stylePath);

    QString resolvedText = resolveImports(styleText, QFileInfo(stylePath).absolutePath(), &visited);
    if (!visited.contains(colorsPath)) {
        const QString colorsText = readTextFile(colorsPath);
        if (!colorsText.isEmpty()) {
            visited.insert(colorsPath);
            resolvedText += QLatin1Char('\n') + colorsText;
        }
    }
    if (!visited.contains(materialPath)) {
        const QString materialText = readTextFile(materialPath);
        if (!materialText.isEmpty()) {
            visited.insert(materialPath);
            resolvedText += QLatin1Char('\n') + materialText;
        }
    }
    if (resolvedText.trimmed().isEmpty()) {
        resolvedText = defaultColorsContents() + QStringLiteral("\n") +
            defaultMaterialContents() + QStringLiteral("\n") + defaultStyleContents();
    }

    styleVariables_ = extractVariables(resolvedText);
    QString flatStyle = stripVariableDeclarations(resolvedText);
    flatStyle = ensurePaletteAndTabStyle(flatStyle);
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
    copyIfMissing(oldDir + QStringLiteral("/material.css"), cfgDir + QStringLiteral("/material.css"));

    const QString configFile = cfgDir + QStringLiteral("/config.toml");
    const QString styleFile = cfgDir + QStringLiteral("/style.css");
    const QString colorsFile = cfgDir + QStringLiteral("/colors.css");
    const QString materialFile = cfgDir + QStringLiteral("/material.css");

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

    if (!QFile::exists(materialFile)) {
        QFile file(materialFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(defaultMaterialContents().toUtf8());
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
    const QString materialFile = resolvedMaterialPath(config_);

    const QStringList files = {configFile, styleFile, colorsFile, materialFile};
    QStringList dirs = {
        QFileInfo(configFile).absolutePath(),
        QFileInfo(styleFile).absolutePath(),
        QFileInfo(colorsFile).absolutePath(),
        QFileInfo(materialFile).absolutePath(),
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
