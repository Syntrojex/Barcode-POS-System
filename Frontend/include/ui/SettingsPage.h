#pragma once
#include "AppState.h"
#include <QWidget>

class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(QWidget *parent = nullptr);
signals:
    void toast(const QString &message);
};
