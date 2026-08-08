/*
 * FainsGram — Custom Badge (Implementation)
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#include "custom_badge.h"
#include "fainsgram_controller.h"

namespace FainsGram {

CustomBadge::CustomBadge(QObject* parent) : QObject(parent) {}

void CustomBadge::loadSettings() {
    auto& s = FG().settings();
    s.beginGroup("Badge");
    _enabled  = s.value("enabled",   true).toBool();
    _badge    = s.value("emoji",     "⚡").toString();
    _marker   = s.value("marker",    "#FG").toString();
    _selfMark = s.value("selfMark",  true).toBool();
    s.endGroup();
}

void CustomBadge::saveSettings() {
    auto& s = FG().settings();
    s.beginGroup("Badge");
    s.setValue("enabled",  _enabled);
    s.setValue("emoji",    _badge);
    s.setValue("marker",   _marker);
    s.setValue("selfMark", _selfMark);
    s.endGroup();
    s.sync();
}

void CustomBadge::setBadgesEnabled(bool on) {
    _enabled = on; saveSettings(); Q_EMIT badgeSettingsChanged();
}
void CustomBadge::setBadgeEmoji(const QString& e) {
    _badge = e; saveSettings(); Q_EMIT badgeSettingsChanged();
}
void CustomBadge::setBioMarker(const QString& m) {
    _marker = m; saveSettings(); Q_EMIT badgeSettingsChanged();
}
void CustomBadge::setSelfMarkEnabled(bool on) {
    _selfMark = on; saveSettings(); Q_EMIT badgeSettingsChanged();
}

bool CustomBadge::isFainsGramUser(const QString& bio) const {
    return _enabled && bio.contains(_marker);
}

void CustomBadge::markUser(qint64 userId)   { _knownUsers.insert(userId); }
bool CustomBadge::isKnownUser(qint64 userId) const { return _knownUsers.contains(userId); }
void CustomBadge::forgetUser(qint64 userId) { _knownUsers.remove(userId); }

} // namespace FainsGram
