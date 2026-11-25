#pragma once
#include "UserRole.h"
#include <QWidget>

class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(QWidget *parent = nullptr, UserRole role = UserRole::Cashier);
signals:
    void toast(const QString &message);
};
