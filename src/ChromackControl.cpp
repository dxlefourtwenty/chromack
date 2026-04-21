#include "ChromackControl.h"

ChromackControl::ChromackControl(QObject *parent)
    : QObject(parent)
{
}

void ChromackControl::Reload()
{
    emit reloadRequested();
}

void ChromackControl::Open()
{
    emit openRequested();
}

void ChromackControl::Close()
{
    emit closeRequested();
}

void ChromackControl::Toggle()
{
    emit toggleRequested();
}

void ChromackControl::SetColor(const QString &value)
{
    emit setColorRequested(value);
}

QString ChromackControl::ActiveColor() const
{
    return activeColor_;
}

void ChromackControl::UpdateActiveColor(const QString &value)
{
    activeColor_ = value.trimmed();
}
