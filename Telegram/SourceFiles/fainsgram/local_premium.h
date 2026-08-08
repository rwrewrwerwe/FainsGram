/*
 * FainsGram — Local Premium + No Ads
 * =====================================
 * Client-side premium unlock without real Telegram Premium subscription.
 *
 * Unlocked (client-side only):
 *   ✓ isPremium flag on local user object
 *   ✓ Extended reactions (all emoji sets)
 *   ✓ Animated avatars / emoji status
 *   ✓ Coloured/gradient usernames
 *   ✓ Premium sticker packs shown as unlocked
 *   ✓ Voice transcription UI enabled
 *   ✓ No sponsored messages (ads → empty list)
 *   ✓ Visual fake phone number in profile
 *   ✓ 4GB upload limit indicator (server-side still limits)
 *
 * Copyright (c) 2024 FainsGram. MIT License.
 */

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

namespace FainsGram {

class LocalPremium final : public QObject {
    Q_OBJECT
public:
    explicit LocalPremium(QObject* parent = nullptr);

    void loadSettings();
    void saveSettings();

    // Apply patches to current session user object
    void apply();
    // Restore to real server state
    void revoke();

    bool isEnabled()  const { return _enabled; }
    bool noAdsEnabled() const { return _noAds; }
    void setEnabled(bool on);
    void setNoAds(bool on);

    // Visual-only custom phone number (local display only)
    QString customPhone() const { return _customPhone; }
    bool    hasCustomPhone() const { return !_customPhone.isEmpty(); }
    void    setCustomPhone(const QString& phone);
    void    clearCustomPhone();

Q_SIGNALS:
    void premiumChanged(bool on);
    void noAdsChanged(bool on);
    void customPhoneChanged(const QString& phone);

private:
    void patchUserPremium(bool on);
    bool  _enabled     = false;
    bool  _noAds       = true;
    QString _customPhone;
};

} // namespace FainsGram
