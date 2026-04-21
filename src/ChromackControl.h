#pragma once

#include <QObject>
#include <QString>

class ChromackControl : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.dxle.chromack.Control")

public:
    explicit ChromackControl(QObject *parent = nullptr);

signals:
    void reloadRequested();
    void openRequested();
    void closeRequested();
    void toggleRequested();
    void setColorRequested(const QString &value);

public slots:
    void Reload();
    void Open();
    void Close();
    void Toggle();
    void SetColor(const QString &value);
};
