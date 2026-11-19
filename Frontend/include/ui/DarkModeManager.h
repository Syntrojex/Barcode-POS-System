#pragma once

/**
 * @file DarkModeManager.h
 * @brief Self-contained Dark Mode subsystem and top-bar toggle component for BarcodeHub.
 * @author Ali Abdullah (25L-3022)
 *
 * Implements a standalone theme management module adhering to Separation of Concerns:
 * - Manages application-wide theme switching (Light <-> Dark mode) dynamically at runtime.
 * - Generates high-contrast, polished dark stylesheets tailored for Point of Sale terminals.
 * - Persists theme state across sessions via QSettings.
 * - Provides a ready-to-mount, interactive toggle button widget for the application top bar.
 */

#include <QObject>
#include <QPushButton>
#include <QIcon>
#include <QPixmap>
#include <QString>

class QWidget;

namespace barcodehub::ui {

/**
 * @class DarkModeManager
 * @brief Central controller managing application theme state, stylesheet injection, and persistence.
 */
class DarkModeManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Accesses the singleton instance of the DarkModeManager.
     * @return Reference to the DarkModeManager instance.
     */
    static DarkModeManager& instance();

    /**
     * @brief Checks if Dark Mode is currently active.
     * @return True if dark mode is active, false if light mode.
     */
    bool isDarkMode() const;

    /**
     * @brief Sets the application theme mode and immediately reapplies the global stylesheet.
     * @param enable True to enable dark mode, false for light mode.
     */
    void setDarkMode(bool enable);

    /**
     * @brief Toggles between light and dark modes.
     */
    void toggle();

    /**
     * @brief Generates the comprehensive QSS stylesheet for the current active theme.
     * @return QString containing complete Qt stylesheet rules.
     */
    QString getActiveStyleSheet() const;

    /**
     * @brief Factory creating a styled, pre-wired toggle button for the application top bar.
     *
     * The button features dynamic icons (Sun for Light mode, Moon for Dark mode),
     * a pill-shaped modern layout, and tooltips. Clicking automatically calls toggle().
     *
     * @param parent Optional parent widget.
     * @return Pointer to the configured QPushButton.
     */
    QPushButton* createTopBarToggle(QWidget* parent = nullptr);

    /**
     * @brief Renders a vector sun icon using QPainter.
     * @param color Icon color.
     * @param size Pixel dimensions.
     * @return QIcon rendered with crisp antialiasing.
     */
    static QIcon createSunIcon(const QColor& color, int size = 20);

    /**
     * @brief Renders a vector moon icon using QPainter.
     * @param color Icon color.
     * @param size Pixel dimensions.
     * @return QIcon rendered with crisp antialiasing.
     */
    static QIcon createMoonIcon(const QColor& color, int size = 20);

signals:
    /**
     * @brief Emitted whenever the theme changes.
     * @param isDark True if switched to dark mode, false if light mode.
     */
    void themeChanged(bool isDark);

private:
    explicit DarkModeManager(QObject* parent = nullptr);
    ~DarkModeManager() override = default;
    DarkModeManager(const DarkModeManager&) = delete;
    DarkModeManager& operator=(const DarkModeManager&) = delete;

    void applyTheme();
    QString buildDarkPaletteStyleSheet() const;

    bool m_isDark{false};
};

} // namespace barcodehub::ui
