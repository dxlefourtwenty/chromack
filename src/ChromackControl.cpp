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
