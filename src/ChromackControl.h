#pragma once

#include <QObject>

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

public slots:
    void Reload();
    void Open();
    void Close();
    void Toggle();
};
