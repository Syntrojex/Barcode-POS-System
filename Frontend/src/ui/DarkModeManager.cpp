#include "DarkModeManager.h"
#include "Theme.h"

#include <QApplication>
#include <QDateTime>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QShortcut>
#include <QWidget>
#include <cmath>

namespace barcodehub::ui {

DarkModeManager::DarkModeManager(QObject* parent)
    : QObject(parent), m_isDark(Theme::isDark())
{
}

DarkModeManager& DarkModeManager::instance()
{
    static DarkModeManager s_instance;
    return s_instance;
}

bool DarkModeManager::isDarkMode() const
{
    return m_isDark;
}

void DarkModeManager::setDarkMode(bool enable)
{
    if (m_isDark == enable && qApp->styleSheet() == getActiveStyleSheet())
        return;

    m_isDark = enable;
    Theme::setDark(enable);
    applyTheme();
    emit themeChanged(m_isDark);
}

void DarkModeManager::toggle()
{
    setDarkMode(!m_isDark);
}

void DarkModeManager::applyTheme()
{
    if (!qApp)
        return;

    qApp->setStyleSheet(getActiveStyleSheet());

    // Repolish all active top-level windows
    for (QWidget* w : QApplication::topLevelWidgets()) {
        Theme::repolish(w);
        for (QWidget* child : w->findChildren<QWidget*>()) {
            Theme::repolish(child);
        }
    }
}

QString DarkModeManager::buildDarkPaletteStyleSheet() const
{
    return Theme::styleSheet();
}

QString DarkModeManager::getActiveStyleSheet() const
{
    QString base = Theme::styleSheet();

    // Append dedicated styles for the Dark Mode top-bar toggle button
    QString toggleStyle;
    if (m_isDark) {
        toggleStyle = QStringLiteral(R"(
            #darkModeToggle {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #233150, stop:1 #172138);
                border: 1px solid #33446c;
                border-bottom: 2px solid #0f1627;
                border-radius: 12px;
                padding: 5px 12px 5px 10px;
                font-weight: 700;
                font-size: 12.5px;
                color: #e0e9ff;
            }
            #darkModeToggle:hover {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2d3e64, stop:1 #1e2a47);
                border-color: #4b629b;
                color: #ffffff;
            }
            #darkModeToggle:pressed {
                border-bottom: 1px solid #0f1627;
                padding-top: 6px;
                padding-bottom: 4px;
                background: #19233a;
            }
        )");
    } else {
        toggleStyle = QStringLiteral(R"(
            #darkModeToggle {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ffffff, stop:1 #eef3ff);
                border: 1px solid #d9e3f8;
                border-bottom: 2px solid #cbd8f2;
                border-radius: 12px;
                padding: 5px 12px 5px 10px;
                font-weight: 700;
                font-size: 12.5px;
                color: #1b2a4d;
            }
            #darkModeToggle:hover {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ffffff, stop:1 #e2ecfd);
                border-color: #b9ccf2;
                color: #0f1b33;
            }
            #darkModeToggle:pressed {
                border-bottom: 1px solid #cbd8f2;
                padding-top: 6px;
                padding-bottom: 4px;
                background: #dce7fb;
            }
        )");
    }

    return Theme::themedSheet(base + "\n" + toggleStyle); // magenta palette for the toggle too
}

QIcon DarkModeManager::createSunIcon(const QColor& color, int size)
{
    const qreal dpr = 2.0;
    QPixmap pm(int(size * dpr), int(size * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(size / 24.0, size / 24.0);

    // Center circular sun disk
    p.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap));
    p.setBrush(QBrush(color));
    p.drawEllipse(QPointF(12.0, 12.0), 4.2, 4.2);

    // 8 radial sun rays
    p.setBrush(Qt::NoBrush);
    const qreal r1 = 6.8;
    const qreal r2 = 9.8;
    for (int i = 0; i < 8; ++i) {
        const qreal angle = i * (3.14159265358979323846 / 4.0);
        const qreal cosA = std::cos(angle);
        const qreal sinA = std::sin(angle);
        p.drawLine(QPointF(12.0 + r1 * cosA, 12.0 + r1 * sinA),
                   QPointF(12.0 + r2 * cosA, 12.0 + r2 * sinA));
    }

    return QIcon(pm);
}

QIcon DarkModeManager::createMoonIcon(const QColor& color, int size)
{
    const qreal dpr = 2.0;
    QPixmap pm(int(size * dpr), int(size * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(size / 24.0, size / 24.0);

    p.setPen(Qt::NoPen);
    p.setBrush(QBrush(color));

    // Smooth crescent moon geometry
    QPainterPath path;
    path.moveTo(17.5, 4.5);
    path.cubicTo(11.0, 4.5, 5.8, 9.7, 5.8, 16.2);
    path.cubicTo(5.8, 20.0, 7.6, 23.3, 10.4, 25.3);
    path.cubicTo(9.0, 23.2, 8.2, 20.6, 8.2, 17.8);
    path.cubicTo(8.2, 11.5, 13.3, 6.4, 19.6, 6.4);
    path.cubicTo(20.3, 6.4, 21.0, 6.5, 21.6, 6.7);
    path.cubicTo(20.4, 5.3, 19.0, 4.5, 17.5, 4.5);
    path.closeSubpath();

    // Adjust scale/offset to fit 24x24 box
    QTransform tr;
    tr.translate(1.0, -1.5);
    tr.scale(0.9, 0.9);
    p.setTransform(tr);

    p.drawPath(path);

    return QIcon(pm);
}

QPushButton* DarkModeManager::createTopBarToggle(QWidget* parent)
{
    auto* btn = new QPushButton(parent);
    btn->setObjectName("darkModeToggle");
    btn->setCursor(Qt::PointingHandCursor);

    auto updateButtonState = [this, btn]() {
        if (m_isDark) {
            btn->setIcon(createSunIcon(Theme::themed("#facc15"), 18)); // Vibrant warm yellow sun
            btn->setText("  Light Mode");
            btn->setToolTip("Switch to Light Theme (Alt+D)");
        } else {
            btn->setIcon(createMoonIcon(Theme::themed("#4338ca"), 18)); // Indigo crescent moon
            btn->setText("  Dark Mode");
            btn->setToolTip("Switch to Dark Theme (Alt+D)");
        }
    };

    updateButtonState();

    connect(btn, &QPushButton::clicked, this, &DarkModeManager::toggle);

    connect(this, &DarkModeManager::themeChanged, btn, [updateButtonState]() {
        updateButtonState();
    });

    // Register Alt+D keyboard shortcut
    auto* shortcut = new QShortcut(QKeySequence("Alt+D"), btn);
    shortcut->setContext(Qt::ApplicationShortcut);
    connect(shortcut, &QShortcut::activated, this, &DarkModeManager::toggle);

    return btn;
}

} // namespace barcodehub::ui
