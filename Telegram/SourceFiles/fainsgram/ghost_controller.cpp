/*
 * FainsGram — Ghost Mode Controller (Implementation)
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#include "ghost_controller.h"
#include "fainsgram_controller.h"

namespace FainsGram {

GhostController::GhostController(QObject* parent) : QObject(parent) {}

void GhostController::loadSettings() {
    auto& s = FG().settings();
    s.beginGroup("GhostMode");
    _s.enabled           = s.value("enabled",           true).toBool();
    _s.noReadReceipts    = s.value("noReadReceipts",    true).toBool();
    _s.hideOnline        = s.value("hideOnline",        true).toBool();
    _s.ghostStories      = s.value("ghostStories",      true).toBool();
    _s.noTypingIndicator = s.value("noTypingIndicator", true).toBool();
    _s.hideLastSeen      = s.value("hideLastSeen",      true).toBool();
    s.endGroup();
}

void GhostController::saveSettings() {
    auto& s = FG().settings();
    s.beginGroup("GhostMode");
    s.setValue("enabled",           _s.enabled);
    s.setValue("noReadReceipts",    _s.noReadReceipts);
    s.setValue("hideOnline",        _s.hideOnline);
    s.setValue("ghostStories",      _s.ghostStories);
    s.setValue("noTypingIndicator", _s.noTypingIndicator);
    s.setValue("hideLastSeen",      _s.hideLastSeen);
    s.endGroup();
    s.sync();
}

#define SET_AND_SAVE(field, on) \
    if (_s.field == on) return; \
    _s.field = on; saveSettings(); Q_EMIT settingsChanged();

void GhostController::setEnabled(bool on) {
    if (_s.enabled == on) return;
    _s.enabled = on; saveSettings();
    Q_EMIT ghostModeChanged(on);
    Q_EMIT settingsChanged();
}
void GhostController::setNoReadReceipts(bool on)    { SET_AND_SAVE(noReadReceipts, on) }
void GhostController::setHideOnline(bool on)        { SET_AND_SAVE(hideOnline, on) }
void GhostController::setGhostStories(bool on)      { SET_AND_SAVE(ghostStories, on) }
void GhostController::setNoTypingIndicator(bool on) { SET_AND_SAVE(noTypingIndicator, on) }
void GhostController::setHideLastSeen(bool on)      { SET_AND_SAVE(hideLastSeen, on) }

} // namespace FainsGram
