/*
 * FainsGram — Ghost Mode Controller
 * ====================================
 * Suppresses MTProto calls that leak user presence:
 *   • messages.readHistory   → no "read" ticks
 *   • account.updateStatus   → hide online status
 *   • stories.readStories    → view stories anonymously
 *   • messages.setTyping     → hide "typing…" indicator
 *
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#pragma once

#include <QtCore/QObject>

namespace FainsGram {

struct GhostSettings {
    bool enabled           = false;
    bool noReadReceipts    = true;
    bool hideOnline        = true;
    bool ghostStories      = true;
    bool noTypingIndicator = true;
    bool hideLastSeen      = true;
};

class GhostController final : public QObject {
    Q_OBJECT
public:
    explicit GhostController(QObject* parent = nullptr);

    void loadSettings();
    void saveSettings();

    bool isEnabled()         const { return _s.enabled; }
    bool noReadReceipts()    const { return _s.noReadReceipts; }
    bool hideOnline()        const { return _s.hideOnline; }
    bool ghostStories()      const { return _s.ghostStories; }
    bool noTypingIndicator() const { return _s.noTypingIndicator; }
    bool hideLastSeen()      const { return _s.hideLastSeen; }

    void setEnabled(bool on);
    void setNoReadReceipts(bool on);
    void setHideOnline(bool on);
    void setGhostStories(bool on);
    void setNoTypingIndicator(bool on);
    void setHideLastSeen(bool on);

    // MTProto hook queries — return true if the request should be dropped
    bool shouldSuppressReadHistory()  const { return _s.enabled && _s.noReadReceipts; }
    bool shouldSuppressUpdateStatus() const { return _s.enabled && _s.hideOnline; }
    bool shouldSuppressReadStories()  const { return _s.enabled && _s.ghostStories; }
    bool shouldSuppressSetTyping()    const { return _s.enabled && _s.noTypingIndicator; }

Q_SIGNALS:
    void ghostModeChanged(bool enabled);
    void settingsChanged();

private:
    GhostSettings _s;
};

} // namespace FainsGram
