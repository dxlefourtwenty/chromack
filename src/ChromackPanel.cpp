#include "ChromackPanel.h"

#include <QApplication>
#include <QCursor>
#include <QDir>
#include <QEasingCurve>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <QVariantAnimation>
#include <QVBoxLayout>
#include <QWindow>
#include <unistd.h>
#if CHROMACK_HAS_LAYERSHELLQT
#include <LayerShellQt/window.h>
#endif

namespace {

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
    QColor color(value.trimmed());
    if (color.isValid()) {
        return color;
    }
    return QColor();
}

QString toHex(const QColor &color)
{
    if (!color.isValid()) {
        return QStringLiteral("#000000");
    }
    return color.name(QColor::HexRgb).toLower();
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

    titleLabel_ = new QLabel(headerBar_);
    titleLabel_->setObjectName(QStringLiteral("headerTitle"));

    closeButton_ = new QPushButton(QStringLiteral("Close"), headerBar_);
    closeButton_->setObjectName(QStringLiteral("closeButton"));

    headerLayout_->addWidget(titleLabel_);
    headerLayout_->addStretch(1);
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

    scrollArea_->setWidget(pickerContainer_);
    scrollArea_->viewport()->setObjectName(QStringLiteral("pickerViewport"));

    panelLayout_->addWidget(scrollArea_, 1);

    footerBar_ = new QFrame(panelRoot_);
    footerBar_->setObjectName(QStringLiteral("footerBar"));
    footerLayout_ = new QHBoxLayout(footerBar_);
    footerLayout_->setContentsMargins(10, 8, 10, 8);
    footerLayout_->setSpacing(8);

    footerLabel_ = new QLabel(footerBar_);
    footerLabel_->setObjectName(QStringLiteral("footerLabel"));

    applyButton_ = new QPushButton(QStringLiteral("Apply"), footerBar_);
    applyButton_->setObjectName(QStringLiteral("applyButton"));

    footerLayout_->addWidget(footerLabel_);
    footerLayout_->addStretch(1);
    footerLayout_->addWidget(applyButton_);

    panelLayout_->addWidget(footerBar_);

    connect(closeButton_, &QPushButton::clicked, this, &ChromackPanel::closePanel);
    connect(applyButton_, &QPushButton::clicked, this, [this]() {
        writeColors();
        reloadConfiguration();
    });
}

void ChromackPanel::buildColorRows()
{
    while (QLayoutItem *item = pickerLayout_->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    colorRows_.clear();

    QStringList orderedKeys = {
        QStringLiteral("--color-primary"),
        QStringLiteral("--color-secondary"),
        QStringLiteral("--color-accent"),
        QStringLiteral("--color-surface"),
        QStringLiteral("--color-surface-alt"),
        QStringLiteral("--color-border"),
        QStringLiteral("--color-text"),
        QStringLiteral("--color-muted"),
        QStringLiteral("--color-success"),
        QStringLiteral("--color-warning"),
        QStringLiteral("--color-danger")
    };

    for (auto it = styleVariables_.cbegin(); it != styleVariables_.cend(); ++it) {
        if (it.key().startsWith(QStringLiteral("--color-")) && !orderedKeys.contains(it.key())) {
            orderedKeys.append(it.key());
        }
    }

    for (const QString &key : orderedKeys) {
        auto row = ColorRow {};
        row.key = key;

        auto *rowWidget = new QFrame(pickerContainer_);
        auto *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(8, 6, 8, 6);
        rowLayout->setSpacing(8);

        row.swatch = new QFrame(rowWidget);
        row.swatch->setObjectName(QStringLiteral("previewSwatch"));
        row.swatch->setFixedSize(18, 18);

        row.nameLabel = new QLabel(displayNameForKey(key), rowWidget);
        row.nameLabel->setObjectName(QStringLiteral("colorNameLabel"));

        row.valueLabel = new QLabel(rowWidget);
        row.valueLabel->setObjectName(QStringLiteral("colorValueLabel"));

        row.input = new QLineEdit(rowWidget);
        row.input->setObjectName(QStringLiteral("colorInput"));
        row.input->setMaxLength(32);
        row.input->setFixedHeight(30);
        row.input->setFixedWidth(130);

        rowLayout->addWidget(row.swatch);
        rowLayout->addWidget(row.nameLabel);
        rowLayout->addStretch(1);
        rowLayout->addWidget(row.valueLabel);
        rowLayout->addWidget(row.input);

        const QColor color = colorForKey(key);
        updateColorRow(&row, color);

        connect(row.input, &QLineEdit::textChanged, this, [this, key](const QString &value) {
            const QColor color = parseColorValue(value);
            for (ColorRow &target : colorRows_) {
                if (target.key == key) {
                    if (color.isValid()) {
                        updateColorRow(&target, color);
                        styleVariables_.insert(key, toHex(color));
                    }
                    break;
                }
            }
        });

        pickerLayout_->addWidget(rowWidget);
        colorRows_.append(row);
    }

    pickerLayout_->addStretch(1);
}

void ChromackPanel::updateColorRow(ColorRow *row, const QColor &color)
{
    const QString hex = toHex(color);

    row->swatch->setStyleSheet(QStringLiteral("background:%1;").arg(hex));
    row->valueLabel->setText(hex);

    if (row->input->text().trimmed().isEmpty() || parseColorValue(row->input->text()).isValid()) {
        row->input->setText(hex);
    }
}

QColor ChromackPanel::colorForKey(const QString &key) const
{
    const QColor color = parseColorValue(styleVariables_.value(key));
    if (color.isValid()) {
        return color;
    }
    return QColor(QStringLiteral("#000000"));
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
    footerLabel_->setText(config_.panel.footerText);
    headerBar_->setVisible(config_.panel.showHeader);
    footerBar_->setVisible(config_.panel.showFooter);

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
    const int rightMarginPx = 20;
    const int x = geo.right() - rightMarginPx - width() + 1;
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
    const int panelWidth = qMax(280, geo.width() / 5);
    const int panelHeight = qMax(220, geo.height() / 2);

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
    QFile file(colorsFilePath());
    const QFileInfo info(file.fileName());
    QDir().mkpath(info.absolutePath());

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&file);
    stream << ":root {\n";

    for (const ColorRow &row : colorRows_) {
        const QColor color = parseColorValue(row.input->text());
        stream << "    " << row.key << ": " << toHex(color) << ";\n";
    }

    stream << "}\n";
}

QString ChromackPanel::colorsFilePath() const
{
    return expandPath(config_.paths.colorsCss);
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
