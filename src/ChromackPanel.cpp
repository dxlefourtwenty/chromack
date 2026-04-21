#include "ChromackPanel.h"

#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QDir>
#include <QEasingCurve>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
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
#include <QTextStream>
#include <QTimer>
#include <QVariantAnimation>
#include <QVBoxLayout>
#include <QWindow>
#include <algorithm>
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
        painter.setRenderHint(QPainter::Antialiasing, true);

        QRect area = rect().adjusted(1, 1, -1, -1);
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
        const QRect area = rect().adjusted(1, 1, -1, -1);
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
        QStringLiteral(R"(^rgba\s*\(\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*([0-9]*\.?[0-9]+)\s*\)$)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = rgbaPattern.match(trimmed);
    if (!match.hasMatch()) {
        return QColor();
    }

    bool okR = false;
    bool okG = false;
    bool okB = false;
    bool okA = false;

    const int r = match.captured(1).toInt(&okR);
    const int g = match.captured(2).toInt(&okG);
    const int b = match.captured(3).toInt(&okB);
    const qreal a = match.captured(4).toDouble(&okA);
    if (!okR || !okG || !okB || !okA) {
        return QColor();
    }

    QColor parsed(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255));
    parsed.setAlphaF(qBound(0.0, a, 1.0));
    return parsed;
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

QIcon copyIconFor(QWidget *widget)
{
    const QIcon themed = QIcon::fromTheme(QStringLiteral("edit-copy"));
    if (!themed.isNull()) {
        return themed;
    }

    if (widget && widget->style()) {
        return widget->style()->standardIcon(QStyle::SP_FileDialogDetailedView);
    }

    return QIcon();
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
    panelLayout_->setContentsMargins(14, 14, 14, 14);
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

    headerLayout_->addWidget(titleWrap, 1);
    headerLayout_->addWidget(closeButton_);

    panelLayout_->addWidget(headerBar_);

    scrollArea_ = new QScrollArea(panelRoot_);
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

    hueSlider_ = new QSlider(Qt::Vertical, pickerTopFrame_);
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

    opacitySlider_ = new QSlider(Qt::Horizontal, opacityRow_);
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
    copyHexButton_->setIcon(copyIconFor(copyHexButton_));
    copyHexButton_->setIconSize(QSize(16, 16));
    copyHexButton_->setText(QString());
    copyHexButton_->setToolTip(QStringLiteral("Copy hex"));

    rgbaLabel_ = new QLabel(QStringLiteral("RGBA"), hexRow_);
    rgbaLabel_->setObjectName(QStringLiteral("rowLabel"));

    rgbaInput_ = new QLineEdit(hexRow_);
    rgbaInput_->setObjectName(QStringLiteral("rgbaInput"));
    rgbaInput_->setReadOnly(true);

    copyRgbaButton_ = new QPushButton(hexRow_);
    copyRgbaButton_->setObjectName(QStringLiteral("inlineCopyButton"));
    copyRgbaButton_->setIcon(copyIconFor(copyRgbaButton_));
    copyRgbaButton_->setIconSize(QSize(16, 16));
    copyRgbaButton_->setText(QString());
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

    panelLayout_->addWidget(scrollArea_, 1);

    connect(closeButton_, &QPushButton::clicked, this, [this]() {
        writeColors();
        reloadConfiguration();
        closePanel();
    });
    connect(copyHexButton_, &QPushButton::clicked, this, &ChromackPanel::copyHexValue);
    connect(copyRgbaButton_, &QPushButton::clicked, this, &ChromackPanel::copyRgbaValue);

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

    syncingUi_ = false;

    updateColorPreview();
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

    syncingUi_ = false;

    updateColorPreview();
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

    if (copyToWaylandClipboard(value)) {
        return;
    }

    QApplication::clipboard()->setText(value);
}

void ChromackPanel::applyConfig(const ChromackConfig &config)
{
    config_ = config;

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
    if (scrollArea_) {
        if (config_.panel.scrollbar == QStringLiteral("none")) {
            scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        } else if (config_.panel.scrollbar == QStringLiteral("always")) {
            scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        } else {
            scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
    }

    updatePanelGeometry(false);
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
