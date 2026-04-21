#include "ChromackPanel.h"

#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QDir>
#include <QEasingCurve>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QScrollArea>
#include <QSlider>
#include <QStandardPaths>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QTabBar>
#include <QTabWidget>
#include <QTextStream>
#include <QTimer>
#include <QVariantAnimation>
#include <QVBoxLayout>
#include <QWindow>
#include <algorithm>
#include <cmath>
#include <functional>
#include <unistd.h>
#if CHROMACK_HAS_LAYERSHELLQT
#include <LayerShellQt/window.h>
#endif

class SaturationValuePicker : public QWidget {
public:
    explicit SaturationValuePicker(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMouseTracking(true);
        setMinimumSize(280, 180);
    }

    void setHue(qreal hue)
    {
        hue_ = qBound(0.0, hue, 1.0);
        update();
    }

    void setSaturationValue(qreal saturation, qreal value)
    {
        saturation_ = qBound(0.0, saturation, 1.0);
        value_ = qBound(0.0, value, 1.0);
        update();
    }

    qreal saturation() const { return saturation_; }
    qreal value() const { return value_; }

    std::function<void(qreal, qreal)> onChanged;

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);

        const QRect area = rect();
        if (!area.isValid()) {
            return;
        }

        painter.fillRect(area, QColor::fromHsvF(hue_, 1.0, 1.0));

        QLinearGradient whiteGradient(area.topLeft(), area.topRight());
        whiteGradient.setColorAt(0.0, QColor(255, 255, 255, 255));
        whiteGradient.setColorAt(1.0, QColor(255, 255, 255, 0));
        painter.fillRect(area, whiteGradient);

        QLinearGradient blackGradient(area.topLeft(), area.bottomLeft());
        blackGradient.setColorAt(0.0, QColor(0, 0, 0, 0));
        blackGradient.setColorAt(1.0, QColor(0, 0, 0, 255));
        painter.fillRect(area, blackGradient);

        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(QColor(255, 255, 255), 2.0));
        const qreal x = area.left() + saturation_ * area.width();
        const qreal y = area.top() + (1.0 - value_) * area.height();
        painter.drawEllipse(QPointF(x, y), 7.0, 7.0);

        painter.setPen(QPen(QColor(20, 20, 20), 1.0));
        painter.drawEllipse(QPointF(x, y), 8.0, 8.0);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        handlePointer(event->position());
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (event->buttons().testFlag(Qt::LeftButton)) {
            handlePointer(event->position());
        }
    }

private:
    void handlePointer(const QPointF &position)
    {
        const QRect area = rect();
        if (!area.isValid()) {
            return;
        }

        const qreal x = qBound(static_cast<qreal>(area.left()), position.x(), static_cast<qreal>(area.right()));
        const qreal y = qBound(static_cast<qreal>(area.top()), position.y(), static_cast<qreal>(area.bottom()));

        saturation_ = (x - area.left()) / static_cast<qreal>(qMax(1, area.width()));
        value_ = 1.0 - ((y - area.top()) / static_cast<qreal>(qMax(1, area.height())));

        update();
        if (onChanged) {
            onChanged(saturation_, value_);
        }
    }

    qreal hue_ = 0.0;
    qreal saturation_ = 1.0;
    qreal value_ = 1.0;
};

class SeekOnClickSlider : public QSlider {
public:
    using QSlider::QSlider;

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            seekTo(event->position());
        }
        QSlider::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (event->buttons().testFlag(Qt::LeftButton)) {
            seekTo(event->position());
        }
        QSlider::mouseMoveEvent(event);
    }

private:
    void seekTo(const QPointF &position)
    {
        QStyleOptionSlider option;
        initStyleOption(&option);

        const QRect grooveRect = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, this);
        const QRect handleRect = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, this);

        const bool isHorizontal = orientation() == Qt::Horizontal;
        const int sliderMin = isHorizontal ? grooveRect.x() : grooveRect.y();
        const int sliderMax = isHorizontal
            ? grooveRect.right() - handleRect.width() + 1
            : grooveRect.bottom() - handleRect.height() + 1;
        const int pointer = isHorizontal ? qRound(position.x()) : qRound(position.y());
        const int handleCenterOffset = isHorizontal ? handleRect.width() / 2 : handleRect.height() / 2;
        const int boundedSliderPos = qBound(sliderMin, pointer - handleCenterOffset, sliderMax);

        const int span = qMax(1, sliderMax - sliderMin);
        const int value = QStyle::sliderValueFromPosition(
            minimum(),
            maximum(),
            boundedSliderPos - sliderMin,
            span,
            option.upsideDown);
        setValue(value);
    }
};

namespace {

constexpr int kMaterialSwatchGapPx = 2;
constexpr int kMaterialGridColumns = 16;
constexpr int kMaterialGridRows = 3;
constexpr int kRecentColorSlots = kMaterialGridColumns;

QString materialPaletteKey(int index)
{
    return QStringLiteral("--color-material-%1").arg(index + 1, 2, 10, QLatin1Char('0'));
}

const QStringList &materialPaletteColors()
{
    static const QStringList colors = {
        QStringLiteral("#f44336"), QStringLiteral("#e91e63"), QStringLiteral("#9c27b0"), QStringLiteral("#673ab7"),
        QStringLiteral("#3f51b5"), QStringLiteral("#2196f3"), QStringLiteral("#03a9f4"), QStringLiteral("#00bcd4"),
        QStringLiteral("#009688"), QStringLiteral("#4caf50"), QStringLiteral("#8bc34a"), QStringLiteral("#cddc39"),
        QStringLiteral("#ffeb3b"), QStringLiteral("#ffc107"), QStringLiteral("#ff9800"), QStringLiteral("#ff5722"),
        QStringLiteral("#ef5350"), QStringLiteral("#ec407a"), QStringLiteral("#ab47bc"), QStringLiteral("#7e57c2"),
        QStringLiteral("#5c6bc0"), QStringLiteral("#42a5f5"), QStringLiteral("#29b6f6"), QStringLiteral("#26c6da"),
        QStringLiteral("#26a69a"), QStringLiteral("#66bb6a"), QStringLiteral("#9ccc65"), QStringLiteral("#d4e157"),
        QStringLiteral("#ffee58"), QStringLiteral("#ffca28"), QStringLiteral("#ffa726"), QStringLiteral("#ff7043"),
        QStringLiteral("#d32f2f"), QStringLiteral("#c2185b"), QStringLiteral("#7b1fa2"), QStringLiteral("#512da8"),
        QStringLiteral("#303f9f"), QStringLiteral("#1976d2"), QStringLiteral("#0288d1"), QStringLiteral("#0097a7"),
        QStringLiteral("#00796b"), QStringLiteral("#388e3c"), QStringLiteral("#689f38"), QStringLiteral("#afb42b"),
        QStringLiteral("#f57f17"), QStringLiteral("#ffffff"), QStringLiteral("#9e9e9e"), QStringLiteral("#000000")
    };
    return colors;
}

QEasingCurve::Type easingFromName(const QString &name)
{
    const QString normalized = name.trimmed().toLower();

    if (normalized == QStringLiteral("linear")) {
        return QEasingCurve::Linear;
    }
    if (normalized == QStringLiteral("out-quad")) {
        return QEasingCurve::OutQuad;
    }
    if (normalized == QStringLiteral("out-quart")) {
        return QEasingCurve::OutQuart;
    }
    if (normalized == QStringLiteral("out-quint")) {
        return QEasingCurve::OutQuint;
    }

    return QEasingCurve::OutCubic;
}

QString runtimeDir()
{
    const QString envRuntime = QString::fromUtf8(qgetenv("XDG_RUNTIME_DIR"));
    if (!envRuntime.trimmed().isEmpty()) {
        return envRuntime;
    }

    const QString runUserPath = QStringLiteral("/run/user/%1").arg(static_cast<qulonglong>(::getuid()));
    if (QFileInfo::exists(runUserPath)) {
        return runUserPath;
    }

    return QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
}

QString displayNameForKey(const QString &key)
{
    QString name = key;
    name.remove(QStringLiteral("--color-"));
    name.replace('-', ' ');
    if (!name.isEmpty()) {
        name[0] = name.at(0).toUpper();
    }
    return name;
}

QColor parseColorValue(const QString &value)
{
    const QString trimmed = value.trimmed();
    QColor color(trimmed);
    if (color.isValid()) {
        return color;
    }

    static const QRegularExpression rgbaPattern(
        QStringLiteral(
            R"(^rgba\s*\(\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})(?:\s*,\s*([0-9]*\.?[0-9]+)\s*)?\)$)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = rgbaPattern.match(trimmed);
    if (!match.hasMatch()) {
        return QColor();
    }

    bool okR = false;
    bool okG = false;
    bool okB = false;
    bool okA = true;

    const int r = match.captured(1).toInt(&okR);
    const int g = match.captured(2).toInt(&okG);
    const int b = match.captured(3).toInt(&okB);
    qreal a = 1.0;
    if (!match.captured(4).isEmpty()) {
        a = match.captured(4).toDouble(&okA);
    }
    if (!okR || !okG || !okB || !okA) {
        return QColor();
    }

    QColor parsed(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255));
    parsed.setAlphaF(qBound(0.0, a, 1.0));
    return parsed;
}

QColor parseColorFromPastelOutput(QString output)
{
    static const QRegularExpression ansiSequence(QStringLiteral(R"(\x1B\[[0-9;]*[A-Za-z])"));
    static const QRegularExpression tokenPattern(
        QStringLiteral(R"((#[0-9A-Fa-f]{6,8}|rgba?\([^\)]+\)))"));

    output.remove(ansiSequence);
    QRegularExpressionMatchIterator tokenIt = tokenPattern.globalMatch(output);
    QColor parsed;
    while (tokenIt.hasNext()) {
        const QString token = tokenIt.next().captured(1).trimmed();
        QColor candidate = parseColorValue(token);
        if (candidate.isValid()) {
            parsed = candidate;
        }
    }

    if (parsed.isValid()) {
        return parsed;
    }

    return parseColorValue(output.trimmed());
}

qreal relativeLuminance(const QColor &color)
{
    const auto linearize = [](qreal channel) {
        const qreal normalized = channel / 255.0;
        if (normalized <= 0.03928) {
            return normalized / 12.92;
        }
        return std::pow((normalized + 0.055) / 1.055, 2.4);
    };

    const qreal r = linearize(color.red());
    const qreal g = linearize(color.green());
    const qreal b = linearize(color.blue());
    return (0.2126 * r) + (0.7152 * g) + (0.0722 * b);
}

QColor blendColors(const QColor &left, const QColor &right, qreal amount)
{
    const qreal t = qBound(0.0, amount, 1.0);
    const qreal inv = 1.0 - t;
    return QColor(
        qRound((left.red() * inv) + (right.red() * t)),
        qRound((left.green() * inv) + (right.green() * t)),
        qRound((left.blue() * inv) + (right.blue() * t)),
        qRound((left.alpha() * inv) + (right.alpha() * t)));
}

QColor hsvColor(qreal hue, qreal saturation, qreal value)
{
    const qreal wrappedHue = hue - std::floor(hue);
    return QColor::fromHsvF(
        wrappedHue,
        qBound(0.0, saturation, 1.0),
        qBound(0.0, value, 1.0));
}

QList<QColor> buildTerminal24FromBase(const QColor &base)
{
    float hue = 0.0f;
    float saturation = 0.0f;
    float value = 0.0f;
    base.getHsvF(&hue, &saturation, &value);
    if (hue < 0.0) {
        hue = 0.0;
    }

    const qreal neutralSaturation = qBound(0.0, saturation * 0.12, 0.14);
    const qreal normalSaturation = qBound(0.42, 0.36 + (saturation * 0.42), 0.82);
    const qreal normalValue = qBound(0.38, 0.40 + (value * 0.25), 0.72);
    const qreal brightSaturation = qBound(0.48, normalSaturation + 0.08, 0.90);
    const qreal brightValue = qBound(0.64, normalValue + 0.22, 0.96);

    QList<QColor> colors;
    colors.reserve(24);

    const QColor normalBlack = hsvColor(hue, neutralSaturation, qBound(0.06, value * 0.18, 0.20));
    const QColor normalWhite = hsvColor(hue, qBound(0.0, saturation * 0.06, 0.08), qBound(0.72, value * 0.75 + 0.16, 0.90));
    const QColor brightBlack = hsvColor(hue, neutralSaturation, qBound(0.20, value * 0.34 + 0.06, 0.40));
    const QColor brightWhite = hsvColor(hue, qBound(0.0, saturation * 0.04, 0.06), qBound(0.92, value * 0.92 + 0.18, 1.0));

    colors << normalBlack;
    colors << hsvColor(hue + (0.0 / 360.0), normalSaturation, normalValue);
    colors << hsvColor(hue + (120.0 / 360.0), normalSaturation, normalValue);
    colors << hsvColor(hue + (60.0 / 360.0), normalSaturation, normalValue);
    colors << hsvColor(hue + (240.0 / 360.0), normalSaturation, normalValue);
    colors << hsvColor(hue + (300.0 / 360.0), normalSaturation, normalValue);
    colors << hsvColor(hue + (180.0 / 360.0), normalSaturation, normalValue);
    colors << normalWhite;

    colors << brightBlack;
    colors << hsvColor(hue + (0.0 / 360.0), brightSaturation, brightValue);
    colors << hsvColor(hue + (120.0 / 360.0), brightSaturation, brightValue);
    colors << hsvColor(hue + (60.0 / 360.0), brightSaturation, brightValue);
    colors << hsvColor(hue + (240.0 / 360.0), brightSaturation, brightValue);
    colors << hsvColor(hue + (300.0 / 360.0), brightSaturation, brightValue);
    colors << hsvColor(hue + (180.0 / 360.0), brightSaturation, brightValue);
    colors << brightWhite;

    const QColor dimAnchor = blendColors(normalBlack, base, 0.10);
    for (int i = 0; i < 8; ++i) {
        const QColor dim = blendColors(colors.at(i), dimAnchor, 0.46);
        colors << hsvColor(
            dim.hsvHueF() < 0.0 ? hue : dim.hsvHueF(),
            qBound(0.0, dim.hsvSaturationF() * 0.72, 1.0),
            qBound(0.0, dim.valueF() * 0.80, 1.0));
    }

    return colors;
}

QStringList terminalPaletteRowNames()
{
    QStringList names = {
        QStringLiteral("Foreground"),
        QStringLiteral("Background")
    };
    for (int i = 0; i < 24; ++i) {
        names << QStringLiteral("Color %1").arg(i, 2, 10, QLatin1Char('0'));
    }
    return names;
}

QString toCssColor(const QColor &color)
{
    if (!color.isValid()) {
        return QStringLiteral("#000000");
    }

    if (color.alpha() >= 255) {
        return color.name(QColor::HexRgb).toLower();
    }

    QString alpha = QString::number(color.alphaF(), 'f', 2);
    while (alpha.endsWith('0')) {
        alpha.chop(1);
    }
    if (alpha.endsWith('.')) {
        alpha.chop(1);
    }

    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(alpha);
}

QString toRgbaColor(const QColor &color)
{
    if (!color.isValid()) {
        return QStringLiteral("rgba(0, 0, 0, 1)");
    }

    QString alpha = QString::number(color.alphaF(), 'f', 2);
    while (alpha.endsWith('0')) {
        alpha.chop(1);
    }
    if (alpha.endsWith('.')) {
        alpha.chop(1);
    }

    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(alpha);
}

QList<QColor> buildShadeScale(const QColor &base, int steps)
{
    QList<QColor> colors;
    if (!base.isValid() || steps <= 0) {
        return colors;
    }

    colors.reserve(steps);
    const QColor black(0, 0, 0, base.alpha());
    const int divisor = qMax(1, steps - 1);
    for (int i = 0; i < steps; ++i) {
        const qreal amount = static_cast<qreal>(i) / static_cast<qreal>(divisor);
        colors << blendColors(black, base, amount);
    }
    return colors;
}

QList<QColor> buildTintScale(const QColor &base, int steps)
{
    QList<QColor> colors;
    if (!base.isValid() || steps <= 0) {
        return colors;
    }

    colors.reserve(steps);
    const QColor white(255, 255, 255, base.alpha());
    const int divisor = qMax(1, steps - 1);
    for (int i = 0; i < steps; ++i) {
        const qreal amount = static_cast<qreal>(i) / static_cast<qreal>(divisor);
        colors << blendColors(base, white, amount);
    }
    return colors;
}

QList<QColor> buildToneScale(const QColor &base, int steps)
{
    QList<QColor> colors;
    if (!base.isValid() || steps <= 0) {
        return colors;
    }

    float hue = 0.0f;
    float saturation = 0.0f;
    float value = 0.0f;
    float alpha = 1.0f;
    base.getHsvF(&hue, &saturation, &value, &alpha);
    if (hue < 0.0) {
        hue = 0.0;
    }

    const qreal minSaturation = qBound(0.0, saturation * 0.12, 0.28);
    const qreal maxSaturation = qBound(0.0, saturation + 0.28, 1.0);
    const int divisor = qMax(1, steps - 1);

    colors.reserve(steps);
    for (int i = 0; i < steps; ++i) {
        const qreal amount = static_cast<qreal>(i) / static_cast<qreal>(divisor);
        const qreal sat = minSaturation + ((maxSaturation - minSaturation) * amount);
        const qreal toneValue = qBound(0.0, (value * 0.92) + (0.08 * amount), 1.0);
        colors << QColor::fromHsvF(hue, sat, toneValue, alpha);
    }

    return colors;
}

bool isMaterialColorKey(const QString &key)
{
    return key.startsWith(QStringLiteral("--color-material-"));
}

QList<QPair<QString, QString>> extractVariables(const QString &text)
{
    QList<QPair<QString, QString>> variables;
    static const QRegularExpression pattern(
        QStringLiteral(R"((--[A-Za-z0-9_-]+)\s*:\s*([^;{}]+);)"));

    QHash<QString, QString> valuesByKey;
    QStringList keyOrder;
    QRegularExpressionMatchIterator it = pattern.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString key = match.captured(1).trimmed();
        if (!valuesByKey.contains(key)) {
            keyOrder.append(key);
        }
        valuesByKey.insert(key, match.captured(2).trimmed());
    }

    for (const QString &key : keyOrder) {
        variables.append({key, valuesByKey.value(key)});
    }

    return variables;
}

bool copyToWaylandClipboard(const QString &text)
{
    QProcess process;
    process.start(QStringLiteral("wl-copy"));
    if (!process.waitForStarted(200)) {
        return false;
    }

    process.write(text.toUtf8());
    process.closeWriteChannel();

    if (!process.waitForFinished(500)) {
        process.kill();
        return false;
    }

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

QString copyButtonGlyph()
{
    return QString::fromUtf8(u8"󰆏");
}

void configureInlineCopyButton(QPushButton *button)
{
    if (!button) {
        return;
    }

    QFont glyphFont = button->font();
    glyphFont.setPixelSize(20);
    glyphFont.setBold(true);
    button->setFont(glyphFont);
    button->setText(copyButtonGlyph());
}

void notifyCopiedColor(const QString &value, const QColor &color)
{
    if (value.isEmpty()) {
        return;
    }

    const QString hex = color.isValid() ? color.name(QColor::HexRgb).toUpper() : QString();
    QStringList args;
    args << QStringLiteral("-lc")
         << QStringLiteral(
                "value=\"$1\";"
                "hex=\"$2\";"
                "icon=\"/tmp/colorpick.png\";"
                "state_file=\"/tmp/chromack-colorpicker-notify.state\";"
                "timeout_ms=5000;"
                "notify_bin=\"/home/dxle/bin/notify-send\";"
                "if [[ ! -x \"$notify_bin\" ]]; then notify_bin=\"/usr/bin/notify-send\"; fi;"
                "if [[ ! -x \"$notify_bin\" ]]; then exit 0; fi;"
                "if [[ -n \"$hex\" ]] && command -v /usr/bin/magick >/dev/null 2>&1; then "
                "/usr/bin/magick -size 32x32 xc:\"$hex\" \"$icon\" >/dev/null 2>&1 || true; "
                "fi;"
                "active_id=\"\";"
                "if [[ -r \"$state_file\" ]]; then "
                "mapfile -t lines < \"$state_file\" || true;"
                "saved_id=\"${lines[0]-}\";"
                "saved_created=\"${lines[1]-}\";"
                "saved_timeout=\"${lines[2]-}\";"
                "if [[ \"$saved_id\" =~ ^[0-9]+$ && \"$saved_created\" =~ ^[0-9]+$ && \"$saved_timeout\" =~ ^[0-9]+$ ]]; then "
                "if [[ \"$saved_timeout\" -eq 0 ]]; then "
                "active_id=\"$saved_id\";"
                "else "
                "now_s=\"$(/usr/bin/date +%s)\";"
                "expires_s=$(( saved_created + ((saved_timeout + 999) / 1000) ));"
                "if [[ \"$now_s\" -lt \"$expires_s\" ]]; then active_id=\"$saved_id\"; fi;"
                "fi;"
                "fi;"
                "fi;"
                "notify_args=(-a \"chromack-color-picker\" -p -t \"$timeout_ms\");"
                "if [[ -n \"$active_id\" ]]; then notify_args+=(-r \"$active_id\"); fi;"
                "if [[ -f \"$icon\" ]]; then notify_args+=(-i \"$icon\"); fi;"
                "notify_args+=(\"Color Picker\" \"$value\");"
                "notify_output=\"$(\"$notify_bin\" \"${notify_args[@]}\" 2>/dev/null || true)\";"
                "new_id=\"$(printf '%s\\n' \"$notify_output\" | /usr/bin/awk 'NF { value=$NF } END { print value }')\";"
                "if [[ \"$new_id\" =~ ^[0-9]+$ ]]; then "
                "tmp_state=\"$(/usr/bin/mktemp /tmp/.chromack-colorpicker-notify.XXXXXX)\";"
                "printf '%s\\n%s\\n%s\\n' \"$new_id\" \"$(/usr/bin/date +%s)\" \"$timeout_ms\" > \"$tmp_state\";"
                "/usr/bin/mv -f \"$tmp_state\" \"$state_file\";"
                "else "
                "/usr/bin/rm -f \"$state_file\";"
                "fi")
         << QStringLiteral("--")
         << value
         << hex;
    if (!QProcess::startDetached(QStringLiteral("/usr/bin/bash"), args)) {
        QProcess::startDetached(QStringLiteral("/usr/bin/notify-send"),
                                QStringList()
                                    << QStringLiteral("-t")
                                    << QStringLiteral("5000")
                                    << QStringLiteral("Color Picker")
                                    << value);
    }
}

bool envWantsOpenByDefault(bool configDefault)
{
    const QString raw = QString::fromUtf8(qgetenv("CHROMACK_START_OPEN")).trimmed().toLower();
    if (raw.isEmpty()) {
        return configDefault;
    }

    if (raw == QStringLiteral("1") ||
        raw == QStringLiteral("true") ||
        raw == QStringLiteral("yes") ||
        raw == QStringLiteral("on")) {
        return true;
    }

    if (raw == QStringLiteral("0") ||
        raw == QStringLiteral("false") ||
        raw == QStringLiteral("no") ||
        raw == QStringLiteral("off")) {
        return false;
    }

    return configDefault;
}

} // namespace

ChromackPanel::ChromackPanel(ChromackConfigLoader *configLoader, QWidget *parent)
    : QWidget(parent)
    , configLoader_(configLoader)
{
    setObjectName(QStringLiteral("chromackWindow"));
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAutoFillBackground(false);

    if (configLoader_) {
        config_ = configLoader_->config();
        styleSheet_ = configLoader_->styleSheet();
        styleVariables_ = configLoader_->styleVariables();

        connect(configLoader_, &ChromackConfigLoader::configChanged,
                this, &ChromackPanel::applyConfig);
        connect(configLoader_, &ChromackConfigLoader::styleChanged,
                this, &ChromackPanel::applyStyle);
    }

    buildUi();
    loadRecentColors();
    buildColorRows();
    applyStyle(styleSheet_, styleVariables_);
    applyConfig(config_);

    open_ = envWantsOpenByDefault(config_.panel.startOpen);
    updatePanelGeometry(false);
    createWinId();
    if (open_) {
        setWindowOpacity(1.0);
        show();
        raise();
    } else {
        setWindowOpacity(0.0);
        hide();
    }
}

ChromackPanel::~ChromackPanel() = default;

void ChromackPanel::buildUi()
{
    panelRoot_ = new QFrame(this);
    panelRoot_->setObjectName(QStringLiteral("panelRoot"));

    panelLayout_ = new QVBoxLayout(panelRoot_);
    panelLayout_->setContentsMargins(14, 20, 14, 14);
    panelLayout_->setSpacing(10);

    headerBar_ = new QFrame(panelRoot_);
    headerBar_->setObjectName(QStringLiteral("headerBar"));
    headerLayout_ = new QHBoxLayout(headerBar_);
    headerLayout_->setContentsMargins(10, 8, 10, 8);
    headerLayout_->setSpacing(8);

    auto *titleWrap = new QFrame(headerBar_);
    titleWrap->setObjectName(QStringLiteral("titleWrap"));
    auto *titleWrapLayout = new QVBoxLayout(titleWrap);
    titleWrapLayout->setContentsMargins(0, 0, 0, 0);
    titleWrapLayout->setSpacing(2);

    titleLabel_ = new QLabel(headerBar_);
    titleLabel_->setObjectName(QStringLiteral("headerTitle"));

    subtitleLabel_ = new QLabel(QStringLiteral("Select a color from the palette or use custom sliders"), headerBar_);
    subtitleLabel_->setObjectName(QStringLiteral("headerSubtitle"));

    titleWrapLayout->addWidget(titleLabel_);
    titleWrapLayout->addWidget(subtitleLabel_);

    closeButton_ = new QPushButton(QStringLiteral("X"), headerBar_);
    closeButton_->setObjectName(QStringLiteral("closeButton"));
    closeButton_->setMouseTracking(true);
    closeButton_->setAttribute(Qt::WA_Hover, true);
    eyedropperButton_ = new QPushButton(headerBar_);
    eyedropperButton_->setObjectName(QStringLiteral("eyedropperButton"));
    eyedropperButton_->setMouseTracking(true);
    eyedropperButton_->setAttribute(Qt::WA_Hover, true);
    eyedropperButton_->setText(QString::fromUtf8(u8""));
    eyedropperButton_->setToolTip(QStringLiteral("Launch color picker"));

    headerLayout_->addWidget(titleWrap, 1);
    headerLayout_->addWidget(eyedropperButton_);
    headerLayout_->addWidget(closeButton_);

    panelLayout_->addWidget(headerBar_);
    panelLayout_->addSpacing(4);

    tabWidget_ = new QTabWidget(panelRoot_);
    tabWidget_->setObjectName(QStringLiteral("panelTabs"));
    tabWidget_->setDocumentMode(true);
    if (QTabBar *tabBar = tabWidget_->tabBar()) {
        tabBar->setDrawBase(false);
        tabBar->setExpanding(true);
        tabBar->setUsesScrollButtons(false);
    }

    pickerTab_ = new QWidget(tabWidget_);
    pickerTab_->setObjectName(QStringLiteral("pickerTab"));
    auto *pickerTabLayout = new QVBoxLayout(pickerTab_);
    pickerTabLayout->setContentsMargins(0, 0, 0, 0);
    pickerTabLayout->setSpacing(0);

    shadesTab_ = new QWidget(tabWidget_);
    shadesTab_->setObjectName(QStringLiteral("shadesTab"));
    auto *shadesLayout = new QVBoxLayout(shadesTab_);
    shadesLayout->setContentsMargins(10, 10, 10, 10);
    shadesLayout->setSpacing(8);
    shadesLayout->setAlignment(Qt::AlignTop);

    paletteTab_ = new QWidget(tabWidget_);
    paletteTab_->setObjectName(QStringLiteral("paletteTab"));
    paletteLayout_ = new QVBoxLayout(paletteTab_);
    paletteLayout_->setContentsMargins(10, 10, 10, 10);
    paletteLayout_->setSpacing(8);
    paletteLayout_->setAlignment(Qt::AlignTop);

    scrollArea_ = new QScrollArea(pickerTab_);
    scrollArea_->setObjectName(QStringLiteral("pickerScrollArea"));
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setFrameShape(QFrame::NoFrame);
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    pickerContainer_ = new QWidget(scrollArea_);
    pickerContainer_->setObjectName(QStringLiteral("pickerContainer"));

    pickerLayout_ = new QVBoxLayout(pickerContainer_);
    pickerLayout_->setContentsMargins(10, 10, 10, 10);
    pickerLayout_->setSpacing(8);
    pickerLayout_->setAlignment(Qt::AlignTop);

    pickerTopFrame_ = new QFrame(pickerContainer_);
    pickerTopFrame_->setObjectName(QStringLiteral("pickerTopFrame"));
    pickerTopLayout_ = new QHBoxLayout(pickerTopFrame_);
    pickerTopLayout_->setContentsMargins(0, 0, 0, 0);
    pickerTopLayout_->setSpacing(8);

    svPicker_ = new SaturationValuePicker(pickerTopFrame_);
    svPicker_->setObjectName(QStringLiteral("svPicker"));

    hueSlider_ = new SeekOnClickSlider(Qt::Vertical, pickerTopFrame_);
    hueSlider_->setObjectName(QStringLiteral("hueSlider"));
    hueSlider_->setRange(0, 359);
    hueSlider_->setValue(120);

    pickerTopLayout_->addWidget(svPicker_, 1);
    pickerTopLayout_->addWidget(hueSlider_);

    materialLabel_ = new QLabel(QStringLiteral("Material Colors"), pickerContainer_);
    materialLabel_->setObjectName(QStringLiteral("sectionLabel"));

    materialGridFrame_ = new QFrame(pickerContainer_);
    materialGridFrame_->setObjectName(QStringLiteral("materialGridFrame"));
    materialGridLayout_ = new QGridLayout(materialGridFrame_);
    materialGridLayout_->setContentsMargins(0, 0, 0, 0);
    materialGridLayout_->setHorizontalSpacing(kMaterialSwatchGapPx);
    materialGridLayout_->setVerticalSpacing(kMaterialSwatchGapPx);

    recentLabel_ = new QLabel(QStringLiteral("Recent Colors"), pickerContainer_);
    recentLabel_->setObjectName(QStringLiteral("sectionLabel"));

    recentRowFrame_ = new QFrame(pickerContainer_);
    recentRowFrame_->setObjectName(QStringLiteral("recentRowFrame"));
    recentRowLayout_ = new QHBoxLayout(recentRowFrame_);
    recentRowLayout_->setContentsMargins(0, 0, 0, 0);
    recentRowLayout_->setSpacing(kMaterialSwatchGapPx);

    opacityRow_ = new QFrame(pickerContainer_);
    opacityRow_->setObjectName(QStringLiteral("opacityRow"));
    opacityLayout_ = new QHBoxLayout(opacityRow_);
    opacityLayout_->setContentsMargins(0, 0, 0, 0);
    opacityLayout_->setSpacing(8);

    opacityLabel_ = new QLabel(QStringLiteral("Opacity"), opacityRow_);
    opacityLabel_->setObjectName(QStringLiteral("rowLabel"));

    opacitySlider_ = new SeekOnClickSlider(Qt::Horizontal, opacityRow_);
    opacitySlider_->setObjectName(QStringLiteral("opacitySlider"));
    opacitySlider_->setRange(0, 255);
    opacitySlider_->setValue(255);

    opacityPreview_ = new QFrame(opacityRow_);
    opacityPreview_->setObjectName(QStringLiteral("opacityPreview"));

    opacityLayout_->addWidget(opacityLabel_);
    opacityLayout_->addWidget(opacitySlider_, 1);
    opacityLayout_->addWidget(opacityPreview_);

    hexRow_ = new QFrame(pickerContainer_);
    hexRow_->setObjectName(QStringLiteral("hexRow"));
    hexLayout_ = new QHBoxLayout(hexRow_);
    hexLayout_->setContentsMargins(0, 0, 0, 0);
    hexLayout_->setSpacing(8);

    hexLabel_ = new QLabel(QStringLiteral("Hex"), hexRow_);
    hexLabel_->setObjectName(QStringLiteral("rowLabel"));

    hexInput_ = new QLineEdit(hexRow_);
    hexInput_->setObjectName(QStringLiteral("hexInput"));
    hexInput_->setMaxLength(64);

    copyHexButton_ = new QPushButton(hexRow_);
    copyHexButton_->setObjectName(QStringLiteral("inlineCopyButton"));
    configureInlineCopyButton(copyHexButton_);
    copyHexButton_->setToolTip(QStringLiteral("Copy hex"));

    rgbaLabel_ = new QLabel(QStringLiteral("RGBA"), hexRow_);
    rgbaLabel_->setObjectName(QStringLiteral("rowLabel"));

    rgbaInput_ = new QLineEdit(hexRow_);
    rgbaInput_->setObjectName(QStringLiteral("rgbaInput"));
    rgbaInput_->setReadOnly(false);

    copyRgbaButton_ = new QPushButton(hexRow_);
    copyRgbaButton_->setObjectName(QStringLiteral("inlineCopyButton"));
    configureInlineCopyButton(copyRgbaButton_);
    copyRgbaButton_->setToolTip(QStringLiteral("Copy rgba"));

    hexLayout_->addWidget(hexLabel_);
    hexLayout_->addWidget(hexInput_, 1);
    hexLayout_->addWidget(copyHexButton_);
    hexLayout_->addWidget(rgbaLabel_);
    hexLayout_->addWidget(rgbaInput_, 1);
    hexLayout_->addWidget(copyRgbaButton_);

    pickerLayout_->addWidget(pickerTopFrame_);
    pickerLayout_->addWidget(materialLabel_);
    pickerLayout_->addWidget(materialGridFrame_);
    pickerLayout_->addWidget(recentLabel_);
    pickerLayout_->addWidget(recentRowFrame_);
    pickerLayout_->addWidget(opacityRow_);
    pickerLayout_->addWidget(hexRow_);
    pickerLayout_->addStretch(1);

    scrollArea_->setWidget(pickerContainer_);
    scrollArea_->viewport()->setObjectName(QStringLiteral("pickerViewport"));
    pickerTabLayout->addWidget(scrollArea_);

    paletteInputRow_ = new QFrame(paletteTab_);
    paletteInputRow_->setObjectName(QStringLiteral("paletteInputRow"));
    paletteInputLayout_ = new QHBoxLayout(paletteInputRow_);
    paletteInputLayout_->setContentsMargins(0, 0, 0, 0);
    paletteInputLayout_->setSpacing(8);

    paletteInputLabel_ = new QLabel(QStringLiteral("Base"), paletteInputRow_);
    paletteInputLabel_->setObjectName(QStringLiteral("rowLabel"));

    paletteInput_ = new QLineEdit(paletteInputRow_);
    paletteInput_->setObjectName(QStringLiteral("paletteInput"));
    paletteInput_->setPlaceholderText(QStringLiteral("#5f6b7b or rgba(95, 107, 123, 1)"));

    paletteInputSwatch_ = new QPushButton(paletteInputRow_);
    paletteInputSwatch_->setObjectName(QStringLiteral("paletteInputSwatch"));
    paletteInputSwatch_->setEnabled(false);
    paletteInputSwatch_->setFocusPolicy(Qt::NoFocus);
    paletteInputSwatch_->setToolTip(QString());
    paletteInputSwatch_->setStyleSheet(QStringLiteral("background: transparent;"));

    paletteGenerateButton_ = new QPushButton(QStringLiteral("Generate"), paletteInputRow_);
    paletteGenerateButton_->setObjectName(QStringLiteral("paletteGenerateButton"));

    paletteInputLayout_->addWidget(paletteInputLabel_);
    paletteInputLayout_->addWidget(paletteInputSwatch_);
    paletteInputLayout_->addWidget(paletteInput_, 1);
    paletteInputLayout_->addWidget(paletteGenerateButton_);

    paletteStatusLabel_ = new QLabel(
        QStringLiteral("Generate 24-based color palette from a base hex or rgba value"),
        paletteTab_);
    paletteStatusLabel_->setObjectName(QStringLiteral("headerSubtitle"));
    paletteStatusLabel_->setWordWrap(true);

    paletteScrollArea_ = new QScrollArea(paletteTab_);
    paletteScrollArea_->setObjectName(QStringLiteral("paletteScrollArea"));
    paletteScrollArea_->setWidgetResizable(true);
    paletteScrollArea_->setFrameShape(QFrame::NoFrame);
    paletteScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    paletteContainer_ = new QWidget(paletteScrollArea_);
    paletteContainer_->setObjectName(QStringLiteral("paletteContainer"));
    paletteContainerLayout_ = new QVBoxLayout(paletteContainer_);
    paletteContainerLayout_->setContentsMargins(0, 0, 0, 0);
    paletteContainerLayout_->setSpacing(0);
    paletteContainerLayout_->setAlignment(Qt::AlignTop);

    paletteGridFrame_ = new QFrame(paletteContainer_);
    paletteGridFrame_->setObjectName(QStringLiteral("paletteGridFrame"));
    paletteGridLayout_ = new QGridLayout(paletteGridFrame_);
    paletteGridLayout_->setContentsMargins(0, 0, 0, 0);
    paletteGridLayout_->setHorizontalSpacing(8);
    paletteGridLayout_->setVerticalSpacing(6);

    buildPaletteRows();

    paletteContainerLayout_->addWidget(paletteGridFrame_);
    paletteContainerLayout_->addStretch(1);
    paletteScrollArea_->setWidget(paletteContainer_);
    paletteScrollArea_->viewport()->setObjectName(QStringLiteral("paletteViewport"));

    paletteLayout_->addWidget(paletteInputRow_);
    paletteLayout_->addWidget(paletteStatusLabel_);
    paletteLayout_->addWidget(paletteScrollArea_, 1);

    buildShadesRows();

    tabWidget_->addTab(pickerTab_, QStringLiteral("Color Picker"));
    tabWidget_->addTab(shadesTab_, QStringLiteral("Shades"));
    tabWidget_->addTab(paletteTab_, QStringLiteral("Palette Generator"));

    panelLayout_->addWidget(tabWidget_, 1);

    connect(closeButton_, &QPushButton::clicked, this, [this]() {
        writeColors();
        reloadConfiguration();
        closePanel();
    });
    connect(eyedropperButton_, &QPushButton::clicked, this, &ChromackPanel::launchEyedropper);
    connect(copyHexButton_, &QPushButton::clicked, this, &ChromackPanel::copyHexValue);
    connect(copyRgbaButton_, &QPushButton::clicked, this, &ChromackPanel::copyRgbaValue);
    connect(paletteGenerateButton_, &QPushButton::clicked, this, &ChromackPanel::generateTerminalPalette);
    connect(paletteInput_, &QLineEdit::returnPressed, this, &ChromackPanel::generateTerminalPalette);
    connect(paletteInput_, &QLineEdit::editingFinished, this, &ChromackPanel::applyPaletteInputToActiveColor);
    connect(paletteInput_, &QLineEdit::textChanged, this, [this](const QString &value) {
        updatePaletteInputSwatch(parseColorValue(value.trimmed()));
    });

    connect(hueSlider_, &QSlider::valueChanged, this, [this](int hue) {
        if (syncingUi_ || activeColorKey_.isEmpty()) {
            return;
        }

        svPicker_->setHue(hue / 359.0);
        QColor color = QColor::fromHsv(hue,
                                       qRound(svPicker_->saturation() * 255.0),
                                       qRound(svPicker_->value() * 255.0),
                                       opacitySlider_->value());
        setActiveColor(color, false);
    });

    svPicker_->onChanged = [this](qreal saturation, qreal value) {
        if (syncingUi_ || activeColorKey_.isEmpty()) {
            return;
        }

        QColor color = QColor::fromHsv(hueSlider_->value(),
                                       qRound(saturation * 255.0),
                                       qRound(value * 255.0),
                                       opacitySlider_->value());
        setActiveColor(color, false);
    };

    connect(opacitySlider_, &QSlider::valueChanged, this, [this](int alpha) {
        if (syncingUi_ || activeColorKey_.isEmpty()) {
            return;
        }

        QColor color = activeColor();
        color.setAlpha(alpha);
        setActiveColor(color, false);
    });

    connect(hexInput_, &QLineEdit::editingFinished, this, [this]() {
        if (activeColorKey_.isEmpty()) {
            return;
        }

        QColor parsed = parseColorValue(hexInput_->text());
        if (!parsed.isValid()) {
            updateColorPreview();
            return;
        }

        if (parsed.alpha() == 255) {
            parsed.setAlpha(opacitySlider_->value());
        }

        setActiveColor(parsed, false);
    });

    connect(rgbaInput_, &QLineEdit::editingFinished, this, [this]() {
        if (activeColorKey_.isEmpty()) {
            return;
        }

        const QColor parsed = parseColorValue(rgbaInput_->text());
        if (!parsed.isValid()) {
            updateColorPreview();
            return;
        }

        setActiveColor(parsed, false);
    });
}

void ChromackPanel::buildColorRows()
{
    while (QLayoutItem *item = materialGridLayout_->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    colorRows_.clear();

    QStringList orderedKeys;
    QList<int> materialIndices;
    for (auto it = styleVariables_.cbegin(); it != styleVariables_.cend(); ++it) {
        const QString key = it.key();
        if (!isMaterialColorKey(key)) {
            continue;
        }

        bool ok = false;
        const int index = key.mid(QStringLiteral("--color-material-").size()).toInt(&ok);
        if (ok && index > 0) {
            materialIndices.append(index);
        }
    }

    std::sort(materialIndices.begin(), materialIndices.end());
    materialIndices.erase(std::unique(materialIndices.begin(), materialIndices.end()), materialIndices.end());
    for (const int index : materialIndices) {
        orderedKeys.append(QStringLiteral("--color-material-%1").arg(index, 2, 10, QLatin1Char('0')));
    }

    const QStringList materialColors = materialPaletteColors();
    int paletteIndex = 0;
    while (paletteIndex < materialColors.size()) {
        const QString key = materialPaletteKey(paletteIndex);
        ++paletteIndex;
        if (orderedKeys.contains(key)) {
            continue;
        }
        orderedKeys.append(key);
        if (!styleVariables_.contains(key)) {
            styleVariables_.insert(key, materialColors.at(paletteIndex - 1));
        }
    }

    const int targetMaterialSlots = kMaterialGridColumns * kMaterialGridRows;
    if (orderedKeys.size() > targetMaterialSlots) {
        orderedKeys = orderedKeys.mid(0, targetMaterialSlots);
    }

    int row = 0;
    int col = 0;
    const int maxCols = kMaterialGridColumns;
    for (const QString &key : orderedKeys) {
        auto colorRow = ColorRow {};
        colorRow.key = key;

        auto *button = new QPushButton(materialGridFrame_);
        button->setObjectName(QStringLiteral("materialSwatch"));
        button->setCheckable(true);
        button->setToolTip(displayNameForKey(key));
        colorRow.materialButton = button;

        connect(button, &QPushButton::clicked, this, [this, key]() {
            setActiveColorKey(key);
        });

        materialGridLayout_->addWidget(button, row, col);
        colorRows_.append(colorRow);

        ++col;
        if (col >= maxCols) {
            col = 0;
            ++row;
        }
    }

    if (!activeColorKey_.isEmpty() && !styleVariables_.contains(activeColorKey_)) {
        activeColorKey_.clear();
    }

    if (activeColorKey_.isEmpty() && !colorRows_.isEmpty()) {
        activeColorKey_ = colorRows_.first().key;
    }

    refreshMaterialButtons();
    refreshRecentButtons();
    setActiveColorKey(activeColorKey_);
}

void ChromackPanel::buildPaletteRows()
{
    while (QLayoutItem *item = paletteGridLayout_->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    paletteRows_.clear();

    const QStringList rowNames = terminalPaletteRowNames();

    for (int rowIndex = 0; rowIndex < rowNames.size(); ++rowIndex) {
        PaletteRow row;
        row.nameLabel = new QLabel(rowNames.at(rowIndex), paletteGridFrame_);
        row.nameLabel->setObjectName(QStringLiteral("rowLabel"));

        row.swatch = new QPushButton(paletteGridFrame_);
        row.swatch->setObjectName(QStringLiteral("paletteSwatch"));
        row.swatch->setEnabled(false);
        row.swatch->setStyleSheet(QStringLiteral("background: transparent;"));

        row.valueInput = new QLineEdit(paletteGridFrame_);
        row.valueInput->setObjectName(QStringLiteral("paletteInput"));
        row.valueInput->setReadOnly(true);

        row.copyButton = new QPushButton(paletteGridFrame_);
        row.copyButton->setObjectName(QStringLiteral("inlineCopyButton"));
        configureInlineCopyButton(row.copyButton);
        row.copyButton->setToolTip(QStringLiteral("Copy color"));
        row.copyButton->setEnabled(false);

        connect(row.copyButton, &QPushButton::clicked, this, [this, rowIndex]() {
            if (rowIndex < 0 || rowIndex >= paletteRows_.size()) {
                return;
            }
            const QString value = paletteRows_.at(rowIndex).valueInput->text().trimmed();
            copyTextValue(value);
        });

        connect(row.swatch, &QPushButton::clicked, this, [this, rowIndex]() {
            if (rowIndex < 0 || rowIndex >= paletteRows_.size()) {
                return;
            }
            const QString value = paletteRows_.at(rowIndex).valueInput->text().trimmed();
            if (value.isEmpty()) {
                return;
            }
            copyTextValue(value);
        });

        paletteGridLayout_->addWidget(row.nameLabel, rowIndex, 0);
        paletteGridLayout_->addWidget(row.swatch, rowIndex, 1);
        paletteGridLayout_->addWidget(row.valueInput, rowIndex, 2);
        paletteGridLayout_->addWidget(row.copyButton, rowIndex, 3);
        paletteGridLayout_->setRowMinimumHeight(rowIndex, 30);
        paletteRows_.append(row);
    }

    if (paletteGridLayout_) {
        paletteGridLayout_->setColumnStretch(2, 1);
    }
}

void ChromackPanel::buildShadesRows()
{
    if (!shadesTab_) {
        return;
    }

    auto *shadesLayout = qobject_cast<QVBoxLayout *>(shadesTab_->layout());
    if (!shadesLayout) {
        return;
    }

    while (QLayoutItem *item = shadesLayout->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    shadeRows_.clear();

    auto *subtitle = new QLabel(
        QStringLiteral("Shades add black, tints add white, and tones adjust saturation with gray."),
        shadesTab_);
    subtitle->setObjectName(QStringLiteral("headerSubtitle"));
    subtitle->setWordWrap(true);
    shadesLayout->addWidget(subtitle);

    auto *sectionsContainer = new QWidget(shadesTab_);
    sectionsContainer->setObjectName(QStringLiteral("shadeSectionsContainer"));
    auto *sectionsLayout = new QVBoxLayout(sectionsContainer);
    sectionsLayout->setContentsMargins(0, 5, 0, 0);
    sectionsLayout->setSpacing(8);

    const QStringList titles = {
        QStringLiteral("Shades"),
        QStringLiteral("Tints"),
        QStringLiteral("Tones")
    };

    for (int rowIndex = 0; rowIndex < titles.size(); ++rowIndex) {
        ShadeScaleRow row;

        auto *sectionFrame = new QFrame(sectionsContainer);
        sectionFrame->setObjectName(QStringLiteral("shadeSection"));
        auto *sectionLayout = new QVBoxLayout(sectionFrame);
        sectionLayout->setContentsMargins(0, 0, 0, 0);
        sectionLayout->setSpacing(6);

        row.titleLabel = new QLabel(titles.at(rowIndex), sectionFrame);
        row.titleLabel->setObjectName(QStringLiteral("sectionLabel"));
        sectionLayout->addWidget(row.titleLabel);

        row.statusLabel = new QLabel(sectionFrame);
        row.statusLabel->setObjectName(QStringLiteral("headerSubtitle"));
        row.statusLabel->setWordWrap(true);
        sectionLayout->addWidget(row.statusLabel);

        auto *swatchFrame = new QFrame(sectionFrame);
        swatchFrame->setObjectName(QStringLiteral("shadeSwatchRow"));
        auto *swatchLayout = new QGridLayout(swatchFrame);
        swatchLayout->setContentsMargins(0, 0, 0, 0);
        swatchLayout->setHorizontalSpacing(0);
        swatchLayout->setVerticalSpacing(0);
        row.swatchLayout = swatchLayout;

        constexpr int kShadeSwatchRows = 2;
        constexpr int kShadeSwatchColumns = 6;
        constexpr int kShadeSwatchCount = kShadeSwatchRows * kShadeSwatchColumns;
        for (int swatchIndex = 0; swatchIndex < kShadeSwatchCount; ++swatchIndex) {
            auto *swatch = new QPushButton(swatchFrame);
            swatch->setObjectName(QStringLiteral("shadeSwatch"));
            swatch->setEnabled(false);
            swatch->setFocusPolicy(Qt::NoFocus);
            swatch->setProperty("shadeColor", QString());
            const int swatchRow = swatchIndex / kShadeSwatchColumns;
            const int swatchColumn = swatchIndex % kShadeSwatchColumns;
            swatchLayout->addWidget(swatch, swatchRow, swatchColumn);
            row.swatches.append(swatch);

            connect(swatch, &QPushButton::clicked, this, [this, swatch]() {
                const QString value = swatch->property("shadeColor").toString();
                if (value.isEmpty()) {
                    return;
                }
                copyTextValue(value);
            });
        }

        for (int swatchColumn = 0; swatchColumn < kShadeSwatchColumns; ++swatchColumn) {
            swatchLayout->setColumnStretch(swatchColumn, 1);
        }
        for (int swatchRow = 0; swatchRow < kShadeSwatchRows; ++swatchRow) {
            swatchLayout->setRowStretch(swatchRow, 1);
        }

        sectionLayout->addWidget(swatchFrame);
        sectionLayout->addStretch(1);
        sectionsLayout->addWidget(sectionFrame, 1);
        shadeRows_.append(row);
    }

    shadesLayout->addWidget(sectionsContainer, 1);
    refreshShadesRows();
}

void ChromackPanel::refreshShadesRows()
{
    if (shadeRows_.size() < 3) {
        return;
    }

    const QColor base = activeColor();
    if (!base.isValid()) {
        return;
    }

    const auto updateRow = [](const ShadeScaleRow &row, const QList<QColor> &colors) {
        for (int i = 0; i < row.swatches.size(); ++i) {
            QPushButton *swatch = row.swatches.at(i);
            if (!swatch) {
                continue;
            }

            if (i >= colors.size()) {
                swatch->setEnabled(false);
                swatch->setToolTip(QString());
                swatch->setStyleSheet(QStringLiteral("background: transparent;"));
                swatch->setProperty("shadeColor", QString());
                continue;
            }

            const QString value = toCssColor(colors.at(i));
            swatch->setEnabled(true);
            swatch->setToolTip(value);
            swatch->setStyleSheet(QStringLiteral("background:%1;").arg(value));
            swatch->setProperty("shadeColor", value);
        }
    };

    const QList<QColor> shades = buildShadeScale(base, shadeRows_.at(0).swatches.size());
    const QList<QColor> tints = buildTintScale(base, shadeRows_.at(1).swatches.size());
    const QList<QColor> tones = buildToneScale(base, shadeRows_.at(2).swatches.size());

    updateRow(shadeRows_.at(0), shades);
    updateRow(shadeRows_.at(1), tints);
    updateRow(shadeRows_.at(2), tones);

    if (!shades.isEmpty()) {
        shadeRows_[0].statusLabel->setText(
            QStringLiteral("%1 is darkest while %2 is closest to base.")
                .arg(shades.first().name(QColor::HexRgb).toLower(),
                     shades.last().name(QColor::HexRgb).toLower()));
    }
    if (!tints.isEmpty()) {
        shadeRows_[1].statusLabel->setText(
            QStringLiteral("%1 starts at base and %2 is lightest.")
                .arg(tints.first().name(QColor::HexRgb).toLower(),
                     tints.last().name(QColor::HexRgb).toLower()));
    }
    if (!tones.isEmpty()) {
        shadeRows_[2].statusLabel->setText(
            QStringLiteral("%1 is least saturated while %2 is most saturated.")
                .arg(tones.first().name(QColor::HexRgb).toLower(),
                     tones.last().name(QColor::HexRgb).toLower()));
    }
}

void ChromackPanel::refreshMaterialButtons()
{
    for (ColorRow &row : colorRows_) {
        if (!row.materialButton) {
            continue;
        }

        const QColor color = colorForKey(row.key);
        row.materialButton->setChecked(row.key == activeColorKey_);
        row.materialButton->setStyleSheet(QStringLiteral("background:%1;").arg(toCssColor(color)));
    }
}

void ChromackPanel::refreshRecentButtons()
{
    while (QLayoutItem *item = recentRowLayout_->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    recentButtons_.clear();

    for (int i = 0; i < kRecentColorSlots; ++i) {
        auto *button = new QPushButton(recentRowFrame_);
        button->setObjectName(QStringLiteral("recentSwatch"));
        if (i < recentColors_.size()) {
            const QColor color = recentColors_.at(i);
            button->setToolTip(toCssColor(color));
            button->setStyleSheet(QStringLiteral("background:%1;").arg(toCssColor(color)));
            connect(button, &QPushButton::clicked, this, [this, color]() {
                setActiveColor(color, false);
            });
        } else {
            button->setEnabled(false);
            button->setToolTip(QString());
            button->setStyleSheet(QStringLiteral("background:transparent;"));
        }

        recentRowLayout_->addWidget(button);
        recentButtons_.append(button);
    }
}

void ChromackPanel::setActiveColorKey(const QString &key)
{
    if (key.isEmpty()) {
        return;
    }

    activeColorKey_ = key;
    refreshMaterialButtons();

    const QColor color = colorForKey(activeColorKey_);

    syncingUi_ = true;

    int hue = color.hsvHue();
    if (hue < 0) {
        hue = 0;
    }

    hueSlider_->setValue(hue);
    opacitySlider_->setValue(color.alpha());
    svPicker_->setHue(hue / 359.0);

    qreal saturation = color.hsvSaturationF();
    if (saturation < 0.0) {
        saturation = 0.0;
    }
    svPicker_->setSaturationValue(saturation, color.valueF());
    hexInput_->setText(color.name(QColor::HexRgb).toLower());
    rgbaInput_->setText(toRgbaColor(color));
    if (paletteInput_ && !paletteInput_->hasFocus()) {
        paletteInput_->setText(hexInput_->text());
    }

    syncingUi_ = false;

    updateColorPreview();
    refreshShadesRows();
}

QColor ChromackPanel::colorForKey(const QString &key) const
{
    QColor color = parseColorValue(styleVariables_.value(key));
    if (color.isValid()) {
        return color;
    }
    return QColor(QStringLiteral("#000000"));
}

QColor ChromackPanel::activeColor() const
{
    if (activeColorKey_.isEmpty()) {
        return QColor(QStringLiteral("#000000"));
    }
    return colorForKey(activeColorKey_);
}

void ChromackPanel::setActiveColor(const QColor &color, bool pushRecent)
{
    if (!color.isValid() || activeColorKey_.isEmpty()) {
        return;
    }

    styleVariables_.insert(activeColorKey_, toCssColor(color));

    if (pushRecent) {
        pushRecentColor(color, true);
    }

    syncingUi_ = true;

    int hue = color.hsvHue();
    if (hue < 0) {
        hue = hueSlider_->value();
    }

    hueSlider_->setValue(hue);
    opacitySlider_->setValue(color.alpha());
    svPicker_->setHue(hue / 359.0);

    qreal saturation = color.hsvSaturationF();
    if (saturation < 0.0) {
        saturation = 0.0;
    }
    svPicker_->setSaturationValue(saturation, color.valueF());
    hexInput_->setText(color.name(QColor::HexRgb).toLower());
    rgbaInput_->setText(toRgbaColor(color));
    if (paletteInput_ && !paletteInput_->hasFocus()) {
        paletteInput_->setText(hexInput_->text());
    }

    syncingUi_ = false;

    updateColorPreview();
    refreshShadesRows();
    refreshMaterialButtons();
    refreshRecentButtons();
}

void ChromackPanel::updateColorPreview()
{
    const QColor color = activeColor();
    const QString css = toCssColor(color);

    opacityPreview_->setStyleSheet(QStringLiteral("background:%1;").arg(css));

    const QString keyName = activeColorKey_.isEmpty()
                                ? QStringLiteral("None")
                                : displayNameForKey(activeColorKey_);
    subtitleLabel_->setText(QStringLiteral("%1 · %2").arg(keyName, css));
}

void ChromackPanel::copyHexValue()
{
    copyTextValue(hexInput_ ? hexInput_->text().trimmed() : QString());
}

void ChromackPanel::copyRgbaValue()
{
    copyTextValue(rgbaInput_ ? rgbaInput_->text().trimmed() : QString());
}

void ChromackPanel::copyTextValue(const QString &value)
{
    if (value.isEmpty()) {
        return;
    }

    if (!copyToWaylandClipboard(value)) {
        QApplication::clipboard()->setText(value);
    }

    const QColor copiedColor = parseColorValue(value);
    if (copiedColor.isValid()) {
        pushRecentColor(copiedColor, true);
    }

    notifyCopiedColor(value, copiedColor);
}

void ChromackPanel::launchEyedropper()
{
    const QString command = config_.panel.eyedropperCommand.trimmed();
    if (command.isEmpty()) {
        return;
    }

    setPanelOpen(false, true, true);

    const int launchDelayMs = config_.animation.enabled
        ? (qMax(80, config_.animation.durationMs + 30) + 10)
        : 0;

    const auto launchCommand = [command]() {
        const QStringList args = {
            QStringLiteral("-lc"),
            command
        };
        if (!QProcess::startDetached(QStringLiteral("/usr/bin/bash"), args)) {
            QProcess::startDetached(QStringLiteral("/usr/bin/notify-send"),
                                    QStringList()
                                        << QStringLiteral("-t")
                                        << QStringLiteral("5000")
                                        << QStringLiteral("Chromack")
                                        << QStringLiteral("Failed to launch eyedropper command"));
        }
    };

    if (launchDelayMs <= 0) {
        launchCommand();
        return;
    }

    QTimer::singleShot(launchDelayMs, this, launchCommand);
}

void ChromackPanel::applyExternalColor(const QString &value)
{
    const QColor parsed = parseColorFromPastelOutput(value);
    if (!parsed.isValid()) {
        return;
    }

    setActiveColor(parsed, true);
    writeColors();
    const int reopenDelayMs = qMax(0, config_.panel.reopenDelayMs);
    if (reopenDelayMs == 0) {
        setPanelOpen(true, true, true);
        return;
    }

    QTimer::singleShot(reopenDelayMs, this, [this]() {
        setPanelOpen(true, true, true);
    });
}

void ChromackPanel::applyPaletteInputToActiveColor()
{
    if (!paletteInput_) {
        return;
    }

    const QColor parsed = parseColorValue(paletteInput_->text().trimmed());
    if (!parsed.isValid()) {
        return;
    }

    setActiveColor(parsed, false);
}

void ChromackPanel::generateTerminalPalette()
{
    if (!paletteInput_ || !paletteStatusLabel_ || paletteRows_.isEmpty()) {
        return;
    }

    const QString rawInput = paletteInput_->text().trimmed();
    const QColor baseInput = parseColorValue(rawInput);
    if (!baseInput.isValid()) {
        updatePaletteInputSwatch(QColor());
        paletteStatusLabel_->setText(QStringLiteral("Invalid color. Use hex or rgba."));
        return;
    }

    setActiveColor(baseInput, false);
    updatePaletteInputSwatch(baseInput);
    const QString baseText = toCssColor(baseInput);
    const QList<QColor> terminalColors = buildTerminal24FromBase(baseInput);

    const QColor background = blendColors(terminalColors.first(), baseInput, 0.20);
    QColor foreground = blendColors(terminalColors.at(15), baseInput, 0.08);
    if (relativeLuminance(background) > 0.42) {
        foreground = blendColors(terminalColors.first(), baseInput, 0.18);
    }

    QList<QColor> colors = {
        foreground,
        background
    };
    colors.append(terminalColors);

    for (int i = 0; i < paletteRows_.size() && i < colors.size(); ++i) {
        const QColor color = colors.at(i);
        const QString value = toCssColor(color);
        paletteRows_[i].swatch->setStyleSheet(QStringLiteral("background:%1;").arg(value));
        paletteRows_[i].swatch->setToolTip(value);
        paletteRows_[i].swatch->setEnabled(true);
        paletteRows_[i].valueInput->setText(value);
        paletteRows_[i].copyButton->setEnabled(true);
    }

    paletteStatusLabel_->setText(QStringLiteral("Generated 24-base palette from %1").arg(baseText));
}

void ChromackPanel::updatePaletteInputSwatch(const QColor &color)
{
    if (!paletteInputSwatch_) {
        return;
    }

    if (!color.isValid()) {
        paletteInputSwatch_->setEnabled(false);
        paletteInputSwatch_->setToolTip(QString());
        paletteInputSwatch_->setStyleSheet(QStringLiteral("background: transparent;"));
        return;
    }

    const QString cssColor = toCssColor(color);
    paletteInputSwatch_->setEnabled(true);
    paletteInputSwatch_->setToolTip(cssColor);
    paletteInputSwatch_->setStyleSheet(QStringLiteral("background:%1;").arg(cssColor));
}

void ChromackPanel::applyConfig(const ChromackConfig &config)
{
    config_ = config;
    loadRecentColors();

    Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::Tool;
    if (config_.layout.aboveWindows) {
        flags |= Qt::WindowStaysOnTopHint;
    }
    if (!config_.layout.focusable) {
        flags |= Qt::WindowDoesNotAcceptFocus;
    }

    if (flags != windowFlags()) {
        setWindowFlags(flags);
    }

    titleLabel_->setText(config_.panel.title);
    headerBar_->setVisible(config_.panel.showHeader);
    const auto applyScrollBarPolicy = [this](QScrollArea *area) {
        if (!area) {
            return;
        }
        if (config_.panel.scrollbar == QStringLiteral("none")) {
            area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        } else if (config_.panel.scrollbar == QStringLiteral("always")) {
            area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        } else {
            area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
    };
    applyScrollBarPolicy(scrollArea_);
    applyScrollBarPolicy(paletteScrollArea_);

    updatePanelGeometry(false);
}

void ChromackPanel::pushRecentColor(const QColor &color, bool persist)
{
    if (!color.isValid()) {
        return;
    }

    int existingIndex = -1;
    for (int i = 0; i < recentColors_.size(); ++i) {
        if (recentColors_.at(i).rgba() == color.rgba()) {
            existingIndex = i;
            break;
        }
    }

    if (existingIndex >= 0) {
        recentColors_.removeAt(existingIndex);
    }

    recentColors_.prepend(color);
    while (recentColors_.size() > kRecentColorSlots) {
        recentColors_.removeLast();
    }

    refreshRecentButtons();
    if (persist) {
        writeRecentColors();
    }
}

void ChromackPanel::loadRecentColors()
{
    recentColors_.clear();

    QFile file(recentColorsFilePath());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString line = stream.readLine().trimmed();
            if (line.isEmpty()) {
                continue;
            }
            const QColor color = parseColorValue(line);
            if (!color.isValid()) {
                continue;
            }
            bool duplicate = false;
            for (const QColor &existing : recentColors_) {
                if (existing.rgba() == color.rgba()) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) {
                continue;
            }
            recentColors_.append(color);
            if (recentColors_.size() >= kRecentColorSlots) {
                break;
            }
        }
    }

    refreshRecentButtons();
}

void ChromackPanel::writeRecentColors()
{
    const QString path = recentColorsFilePath();
    const QFileInfo info(path);
    QDir().mkpath(info.absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return;
    }

    QTextStream stream(&file);
    for (const QColor &color : recentColors_) {
        stream << toCssColor(color) << '\n';
    }
}

void ChromackPanel::applyStyle(const QString &styleSheet, const QHash<QString, QString> &styleVariables)
{
    styleSheet_ = styleSheet;
    styleVariables_ = styleVariables;

    setStyleSheet(styleSheet_);
    buildColorRows();
}

void ChromackPanel::reloadConfiguration()
{
    if (configLoader_) {
        configLoader_->reload();
    }
}

void ChromackPanel::openPanel()
{
    setPanelOpen(true, true, true);
}

void ChromackPanel::closePanel()
{
    setPanelOpen(false, true, true);
}

void ChromackPanel::togglePanel()
{
    setPanelOpen(!open_, true, true);
}

void ChromackPanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (panelRoot_) {
        panelRoot_->setFixedSize(size());
        panelRoot_->move(0, 0);
    }
}

void ChromackPanel::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && config_.panel.closeOnEscape) {
        closePanel();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

QPoint ChromackPanel::openPosition() const
{
    QScreen *screen = targetScreen();
    const QRect geo = screen ? screen->geometry() : QRect(0, 0, 1920, 1080);
    const int rightMarginPx = qMax(0, config_.layout.rightMargin);
    const int minX = geo.left() + qMax(0, config_.layout.leftMargin);
    const int x = qMax(minX, geo.right() - rightMarginPx - width() + 1);
    const int y = geo.top() + ((geo.height() - height()) / 2);

    return QPoint(x, y);
}

QPoint ChromackPanel::closedPosition() const
{
    const QPoint opened = openPosition();
    return QPoint(opened.x() + closedPanelOffset(), opened.y());
}

QScreen *ChromackPanel::targetScreen() const
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.isEmpty()) {
        return nullptr;
    }

    const QString selected = config_.layout.screen.trimmed().toLower();
    if (selected.isEmpty() || selected == QStringLiteral("auto") || selected == QStringLiteral("primary")) {
        return QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen() : screens.first();
    }

    if (selected == QStringLiteral("cursor")) {
        QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
        return screen ? screen : (QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen() : screens.first());
    }

    bool ok = false;
    const int index = selected.toInt(&ok);
    if (ok && index >= 0 && index < screens.size()) {
        return screens.at(index);
    }

    for (QScreen *screen : screens) {
        if (screen && screen->name().trimmed().toLower() == selected) {
            return screen;
        }
    }

    return QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen() : screens.first();
}

int ChromackPanel::closedPanelOffset() const
{
    const int hiddenDistance = width() + qMax(0, config_.animation.slideDistance);
    return slideFromRight() ? hiddenDistance : -hiddenDistance;
}

int ChromackPanel::panelOffsetForState(bool open) const
{
    Q_UNUSED(open);
    return 0;
}

void ChromackPanel::applyPanelOffset(int offset)
{
    panelOffsetX_ = offset;
    if (!panelRoot_) {
        return;
    }

    panelRoot_->move(0, 0);
}

void ChromackPanel::animatePanelOffsetTo(int offset)
{
    Q_UNUSED(offset);
    animateToPosition(open_ ? openPosition() : closedPosition());
}

bool ChromackPanel::slideFromRight() const
{
    const QString direction = config_.animation.slideDirection.trimmed().toLower();
    if (direction == QStringLiteral("right")) {
        return true;
    }
    if (direction == QStringLiteral("left")) {
        return false;
    }

    return config_.layout.anchor.trimmed().toLower().contains(QStringLiteral("right"));
}

bool ChromackPanel::usesLayerShellPlacement() const
{
#if CHROMACK_HAS_LAYERSHELLQT
    return QGuiApplication::platformName().startsWith(QStringLiteral("wayland"));
#else
    return false;
#endif
}

#if CHROMACK_HAS_LAYERSHELLQT
void ChromackPanel::configureLayerShell(QScreen *screen, int panelHeight)
{
    if (!usesLayerShellPlacement()) {
        return;
    }

    if (!windowHandle()) {
        winId();
    }
    if (!windowHandle()) {
        return;
    }

    if (!layerShellWindow_) {
        layerShellWindow_ = LayerShellQt::Window::get(windowHandle());
    }
    if (!layerShellWindow_) {
        return;
    }

    LayerShellQt::Window::Anchors anchors;
    anchors |= LayerShellQt::Window::AnchorTop;
    anchors |= LayerShellQt::Window::AnchorRight;

    layerShellWindow_->setAnchors(anchors);
    layerShellWindow_->setLayer(config_.layout.aboveWindows
                                    ? LayerShellQt::Window::LayerOverlay
                                    : LayerShellQt::Window::LayerTop);
    layerShellWindow_->setKeyboardInteractivity(
        config_.layout.focusable
            ? LayerShellQt::Window::KeyboardInteractivityOnDemand
            : LayerShellQt::Window::KeyboardInteractivityNone);
    layerShellWindow_->setActivateOnShow(config_.layout.focusable);
    layerShellWindow_->setScope(QStringLiteral("chromack"));
    layerShellWindow_->setDesiredSize(QSize(width(), panelHeight));
    layerShellWindow_->setExclusiveZone(-1);

    if (screen) {
        layerShellWindow_->setScreen(screen);
    }
}

void ChromackPanel::applyLayerShellPlacement(const QPoint &position, const QRect &availableGeometry)
{
    if (!layerShellWindow_ || !availableGeometry.isValid()) {
        return;
    }

    const int right = availableGeometry.right() - (position.x() + width() - 1);
    const int top = position.y() - availableGeometry.top();
    layerShellWindow_->setMargins(QMargins(0, top, right, 0));

    update();
    if (QWindow *window = windowHandle()) {
        window->requestUpdate();
    }
}
#else
void ChromackPanel::configureLayerShell(QScreen *, int)
{
}

void ChromackPanel::applyLayerShellPlacement(const QPoint &, const QRect &)
{
}
#endif

void ChromackPanel::updatePanelGeometry(bool animated)
{
    QScreen *screen = targetScreen();
    const QRect geo = screen ? screen->geometry() : QRect(0, 0, 1920, 1080);
    const int maxPanelWidth = qMax(
        220,
        geo.width() - (config_.layout.leftMargin + config_.layout.rightMargin));
    const int panelWidth = qBound(220, config_.layout.width, maxPanelWidth);
    const int maxPanelHeight = qMax(
        220,
        geo.height() - (config_.layout.topMargin + config_.layout.bottomMargin));
    const int desiredPanelHeight = (geo.height() * 55) / 100;
    const int panelHeight = qBound(config_.layout.minimumHeight, desiredPanelHeight, maxPanelHeight);

    setFixedWidth(panelWidth);
    setFixedHeight(panelHeight);

    const QPoint targetPosition = open_ ? openPosition() : closedPosition();
    if (usesLayerShellPlacement()) {
        configureLayerShell(screen, panelHeight);
#if CHROMACK_HAS_LAYERSHELLQT
        layerShellPlacementGeometryCache_ = geo;
#endif
    }

    if (panelRoot_) {
        panelRoot_->setFixedSize(size());
        panelRoot_->move(0, 0);
    }

    if (!animated || !config_.animation.enabled) {
        if (usesLayerShellPlacement()) {
#if CHROMACK_HAS_LAYERSHELLQT
            layerShellPosition_ = targetPosition;
            applyLayerShellPlacement(layerShellPosition_, layerShellPlacementGeometryCache_);
#endif
        } else {
            move(targetPosition);
        }
        return;
    }

    animateToPosition(targetPosition);
}

void ChromackPanel::animateToPosition(const QPoint &position)
{
    if (!config_.animation.enabled) {
        if (usesLayerShellPlacement()) {
#if CHROMACK_HAS_LAYERSHELLQT
            layerShellPosition_ = position;
            applyLayerShellPlacement(layerShellPosition_, layerShellPlacementGeometryCache_);
#endif
        } else {
            move(position);
        }
        return;
    }

    if (slideAnimation_) {
        slideAnimation_->stop();
        slideAnimation_->deleteLater();
        slideAnimation_ = nullptr;
    }

    slideAnimation_ = new QVariantAnimation(this);
    slideAnimation_->setDuration(config_.animation.durationMs);
    slideAnimation_->setEasingCurve(easingFromName(config_.animation.easing));
    slideAnimation_->setStartValue(usesLayerShellPlacement()
#if CHROMACK_HAS_LAYERSHELLQT
                                       ? layerShellPosition_
#else
                                       ? QPoint()
#endif
                                       : pos());
    slideAnimation_->setEndValue(position);
    connect(slideAnimation_, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        const QPoint current = value.toPoint();
        if (usesLayerShellPlacement()) {
#if CHROMACK_HAS_LAYERSHELLQT
            layerShellPosition_ = current;
            applyLayerShellPlacement(layerShellPosition_, layerShellPlacementGeometryCache_);
#endif
            return;
        }

        move(current);
    });
    slideAnimation_->start(QAbstractAnimation::DeleteWhenStopped);
}

void ChromackPanel::setPanelOpen(bool open, bool animated, bool writeState)
{
    const bool openingFromHidden = open && !isVisible();
    open_ = open;

    if (openingFromHidden) {
        open_ = false;
        updatePanelGeometry(false);
        open_ = true;
        if (config_.animation.fade) {
            setWindowOpacity(0.0);
        }
        show();
    } else if (open_ && !isVisible()) {
        show();
    }

    if (open_) {
        raise();
    }

    if (fadeAnimation_) {
        fadeAnimation_->stop();
    }

    if (animated && config_.animation.enabled && config_.animation.fade) {
        auto *fade = new QPropertyAnimation(this, "windowOpacity");
        fadeAnimation_ = fade;
        fade->setDuration(config_.animation.fadeDurationMs);
        fade->setStartValue(windowOpacity());
        fade->setEndValue(open_ ? 1.0 : 0.0);
        fade->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        setWindowOpacity(open_ ? 1.0 : 0.0);
    }

    updatePanelGeometry(animated);

    if (!open_) {
        const bool shouldDelayHide = animated && config_.animation.enabled;
        if (shouldDelayHide) {
            const int hideDelayMs = qMax(80, config_.animation.durationMs + 30);
            QTimer::singleShot(hideDelayMs, this, [this]() {
                if (!open_) {
                    hide();
                }
            });
        } else {
            hide();
        }
    }

    if (writeState) {
        writeStateFile(open_ ? QStringLiteral("open") : QStringLiteral("closed"));
    }
}

void ChromackPanel::writeColors()
{
    const QString colorsPath = colorsFilePath();
    const QString materialPath = materialFilePath();

    const auto writeVariablesFile = [this](const QString &path, const std::function<bool(const QString &)> &keyFilter) {
        QFile file(path);
        const QFileInfo info(path);
        QDir().mkpath(info.absolutePath());

        QString existingText;
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            existingText = QString::fromUtf8(file.readAll());
            file.close();
        }
        const QList<QPair<QString, QString>> existingVariables = extractVariables(existingText);
        QHash<QString, QString> valuesByKey;
        QStringList keyOrder;

        for (const auto &entry : existingVariables) {
            const QString key = entry.first;
            const bool isColor = key.startsWith(QStringLiteral("--color-"));
            const bool shouldKeep = keyFilter(key) || !isColor;
            if (!shouldKeep) {
                continue;
            }
            if (!valuesByKey.contains(key)) {
                keyOrder.append(key);
            }
            valuesByKey.insert(key, entry.second);
        }

        for (const ColorRow &row : colorRows_) {
            if (!keyFilter(row.key)) {
                continue;
            }
            if (!valuesByKey.contains(row.key)) {
                keyOrder.append(row.key);
            }
            valuesByKey.insert(row.key, toCssColor(colorForKey(row.key)));
        }

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream stream(&file);
        stream << ":root {\n";

        for (const QString &key : keyOrder) {
            stream << "    " << key << ": " << valuesByKey.value(key) << ";\n";
        }

        stream << "}\n";
        return true;
    };

    if (colorsPath == materialPath) {
        writeVariablesFile(colorsPath, [](const QString &) {
            return true;
        });
        return;
    }

    writeVariablesFile(colorsPath, [](const QString &key) {
        return !isMaterialColorKey(key);
    });
    writeVariablesFile(materialPath, [](const QString &key) {
        return isMaterialColorKey(key);
    });
}

QString ChromackPanel::colorsFilePath() const
{
    return expandPath(config_.paths.colorsCss);
}

QString ChromackPanel::materialFilePath() const
{
    return expandPath(config_.paths.materialCss);
}

QString ChromackPanel::stateFilePath() const
{
    return expandPath(config_.paths.stateFile);
}

QString ChromackPanel::recentColorsFilePath() const
{
    return expandPath(config_.paths.recentColorsFile);
}

QString ChromackPanel::expandPath(QString value) const
{
    if (value == QStringLiteral("~")) {
        value = QDir::homePath();
    } else if (value.startsWith(QStringLiteral("~/"))) {
        value = QDir::homePath() + value.mid(1);
    }

    value.replace(QStringLiteral("${XDG_RUNTIME_DIR}"), runtimeDir());
    value.replace(QStringLiteral("$XDG_RUNTIME_DIR"), runtimeDir());
    return value;
}

void ChromackPanel::writeStateFile(const QString &value)
{
    const QString path = stateFilePath();
    const QFileInfo info(path);
    QDir().mkpath(info.absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&file);
    stream << value << '\n';
}
