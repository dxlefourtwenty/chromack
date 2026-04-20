#pragma once

#include <QColor>
#include <QHash>
#include <QPointer>
#include <QRect>
#include <QWidget>

#include "ChromackConfig.h"

class QFrame;
class QGridLayout;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QPropertyAnimation;
class QScreen;
class QScrollArea;
class QSlider;
class QVBoxLayout;
class QVariantAnimation;
class SaturationValuePicker;
#if CHROMACK_HAS_LAYERSHELLQT
namespace LayerShellQt
{
class Window;
}
#endif

class ChromackPanel : public QWidget {
    Q_OBJECT

public:
    explicit ChromackPanel(ChromackConfigLoader *configLoader, QWidget *parent = nullptr);
    ~ChromackPanel() override;

public slots:
    void reloadConfiguration();
    void openPanel();
    void closePanel();
    void togglePanel();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void applyConfig(const ChromackConfig &config);
    void applyStyle(const QString &styleSheet, const QHash<QString, QString> &styleVariables);

private:
    struct ColorRow {
        QString key;
        QPushButton *materialButton = nullptr;
    };

    void buildUi();
    void buildColorRows();
    void refreshMaterialButtons();
    void refreshRecentButtons();
    void setActiveColorKey(const QString &key);
    QColor colorForKey(const QString &key) const;
    QColor activeColor() const;
    void setActiveColor(const QColor &color, bool pushRecent);
    void updateColorPreview();
    void copyCurrentColor();
    void copyHexValue();
    void copyRgbaValue();
    void copyTextValue(const QString &value);
    void updatePanelGeometry(bool animated);
    QPoint openPosition() const;
    QPoint closedPosition() const;
    int closedPanelOffset() const;
    int panelOffsetForState(bool open) const;
    void applyPanelOffset(int offset);
    void animatePanelOffsetTo(int offset);
    void animateToPosition(const QPoint &position);
    QScreen *targetScreen() const;
    bool slideFromRight() const;
    bool usesLayerShellPlacement() const;
    void configureLayerShell(QScreen *screen, int panelHeight);
    void applyLayerShellPlacement(const QPoint &position, const QRect &availableGeometry);
    void setPanelOpen(bool open, bool animated, bool writeState);
    void writeColors();
    QString colorsFilePath() const;
    QString stateFilePath() const;
    QString expandPath(QString value) const;
    void writeStateFile(const QString &value);

    ChromackConfigLoader *configLoader_ = nullptr;
    ChromackConfig config_;
    QString styleSheet_;
    QHash<QString, QString> styleVariables_;

    QFrame *panelRoot_ = nullptr;
    QVBoxLayout *panelLayout_ = nullptr;

    QFrame *headerBar_ = nullptr;
    QHBoxLayout *headerLayout_ = nullptr;
    QLabel *titleLabel_ = nullptr;
    QLabel *subtitleLabel_ = nullptr;
    QPushButton *closeButton_ = nullptr;

    QScrollArea *scrollArea_ = nullptr;
    QWidget *pickerContainer_ = nullptr;
    QVBoxLayout *pickerLayout_ = nullptr;

    QFrame *pickerTopFrame_ = nullptr;
    QHBoxLayout *pickerTopLayout_ = nullptr;
    SaturationValuePicker *svPicker_ = nullptr;
    QSlider *hueSlider_ = nullptr;

    QLabel *materialLabel_ = nullptr;
    QFrame *materialGridFrame_ = nullptr;
    QGridLayout *materialGridLayout_ = nullptr;

    QLabel *recentLabel_ = nullptr;
    QFrame *recentRowFrame_ = nullptr;
    QHBoxLayout *recentRowLayout_ = nullptr;

    QFrame *opacityRow_ = nullptr;
    QHBoxLayout *opacityLayout_ = nullptr;
    QLabel *opacityLabel_ = nullptr;
    QSlider *opacitySlider_ = nullptr;
    QFrame *opacityPreview_ = nullptr;

    QFrame *hexRow_ = nullptr;
    QHBoxLayout *hexLayout_ = nullptr;
    QLabel *hexLabel_ = nullptr;
    QLineEdit *hexInput_ = nullptr;
    QLabel *rgbaLabel_ = nullptr;
    QLineEdit *rgbaInput_ = nullptr;
    QPushButton *copyHexButton_ = nullptr;
    QPushButton *copyRgbaButton_ = nullptr;

    QFrame *footerBar_ = nullptr;
    QHBoxLayout *footerLayout_ = nullptr;
    QLabel *footerLabel_ = nullptr;
    QPushButton *cancelButton_ = nullptr;
    QPushButton *applyButton_ = nullptr;
    QPushButton *copyButton_ = nullptr;

    QList<ColorRow> colorRows_;
    QList<QPushButton *> recentButtons_;
    QList<QColor> recentColors_;

    QString activeColorKey_;
    QPointer<QVariantAnimation> slideAnimation_;
    QPointer<QPropertyAnimation> fadeAnimation_;
    int panelOffsetX_ = 0;
    bool syncingUi_ = false;

    bool open_ = false;
#if CHROMACK_HAS_LAYERSHELLQT
    LayerShellQt::Window *layerShellWindow_ = nullptr;
    QPoint layerShellPosition_;
    QRect layerShellPlacementGeometryCache_;
#endif
};
