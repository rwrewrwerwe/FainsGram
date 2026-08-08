/*
 * FainsGram — Local Premium (Implementation)
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#include "local_premium.h"
#include "fainsgram_controller.h"
#include "main/main_session.h"
#include "data/data_user.h"
#include "data/data_changes.h"
#include "core/application.h"
#include "main/main_account.h"
#include "main/main_domain.h"

namespace FainsGram {

LocalPremium::LocalPremium(QObject* parent) : QObject(parent) {}

void LocalPremium::loadSettings() {
    auto& s = FG().settings();
    s.beginGroup("LocalPremium");
    _enabled     = s.value("enabled",     true).toBool();
    _noAds       = s.value("noAds",       true).toBool();
    _customPhone = s.value("customPhone", "").toString();
    s.endGroup();
}

void LocalPremium::saveSettings() {
    auto& s = FG().settings();
    s.beginGroup("LocalPremium");
    s.setValue("enabled",     _enabled);
    s.setValue("noAds",       _noAds);
    s.setValue("customPhone", _customPhone);
    s.endGroup();
    s.sync();
}

void LocalPremium::apply() {
    if (_enabled) patchUserPremium(true);
}

void LocalPremium::revoke() {
    patchUserPremium(false);
}

void LocalPremium::setEnabled(bool on) {
    if (_enabled == on) return;
    _enabled = on;
    patchUserPremium(on);
    saveSettings();
    Q_EMIT premiumChanged(on);
}

void LocalPremium::setNoAds(bool on) {
    if (_noAds == on) return;
    _noAds = on;
    saveSettings();
    Q_EMIT noAdsChanged(on);
}

void LocalPremium::setCustomPhone(const QString& phone) {
    _customPhone = phone;
    saveSettings();
    Q_EMIT customPhoneChanged(phone);
}

void LocalPremium::clearCustomPhone() {
    setCustomPhone(QString());
}

void LocalPremium::patchUserPremium(bool on) {
    if (Core::App().someSessionExists()) {
        if (const auto session = Core::App().domain().active().maybeSession()) {
            const auto self = session->user();
            if (on) {
                self->addFlags(UserDataFlag::Premium);
            } else {
                self->removeFlags(UserDataFlag::Premium);
            }
            session->changes().peerUpdated(
                self,
                Data::PeerUpdate::Flag::FullInfo
                | Data::PeerUpdate::Flag::Name
            );
        }
    }
}

} // namespace FainsGram
