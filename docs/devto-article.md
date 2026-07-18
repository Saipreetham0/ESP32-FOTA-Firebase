---
title: "ESP32 OTA Firmware Updates Using Firebase (No Custom Server)"
published: false
description: "How to push over-the-air firmware updates to a deployed ESP32 using Firebase Cloud Storage — no update server to run, no port forwarding."
tags: esp32, iot, firebase, embedded
cover_image:
canonical_url: https://github.com/Saipreetham0/ESP32-FOTA-Firebase
---

Once an ESP32 is glued inside an enclosure on a wall, "just re-flash it over USB" stops being an option. You need **O**ver-**T**he-**A**ir (OTA) updates. The usual tutorials point you at running your own update server — but if your device is already talking to Firebase, you don't need one. **Firebase Cloud Storage *is* your update server.**

Here's the whole idea:

```
┌──────────────┐   upload firmware.bin   ┌──────────────────┐
│  You / CI    │ ───────────────────────▶│ Firebase Storage │
└──────────────┘                         └────────┬─────────┘
                                                  │ downloadOTA() at boot
                                                  ▼
                                         ┌──────────────────┐
                                         │      ESP32       │ ── flash → reboot
                                         └──────────────────┘
```

The device checks a fixed path in your bucket at boot. If a `firmware.bin` is there, it downloads it, flashes itself, deletes the file, and reboots into the new firmware.

## Why this beats a custom OTA server

- **Nothing to host** — no VPS, no HTTP server, no port forwarding. The free Firebase tier is plenty.
- **Authenticated** — devices sign in with Firebase email/password auth, so your bucket isn't public.
- **One-shot updates** — the binary is deleted after a successful flash, so devices don't re-flash in a loop.

## The part that does the work

The entire OTA mechanism is one call from the [Firebase-ESP-Client](https://github.com/mobizt/Firebase-ESP-Client) library:

```cpp
Firebase.Storage.downloadOTA(
    &fbdo,
    STORAGE_BUCKET_ID,   // your-project.appspot.com
    "test/firmware.bin", // path in the bucket
    fcsDownloadCallback  // progress callback
);
```

It streams the binary from Storage straight into the ESP32 OTA partition (using `Update.h` under the hood), so the full firmware **never has to fit in RAM**. On success, the sketch deletes the remote file and calls `ESP.restart()`.

A progress callback lets you watch it happen over serial:

```
Checking for new firmware update available...
New update found
Downloading firmware test/firmware.bin (874512 bytes)
Downloaded 100%
Download firmware completed.
Delete file... ok
Restarting...
```

## Setting it up

**1. Firebase project**
- Enable **Email/Password** auth and add a user — this is the *device's* login, e.g. `device1@yourproject.com`.
- Create a **Storage** bucket (note the `xxxx.appspot.com` ID).
- Grab the **Web API Key** from Project Settings.

**2. Keep credentials out of git**

Don't hardcode WiFi and Firebase creds in `main.cpp` (I learned this the hard way). Put them in a gitignored header:

```cpp
// include/secrets.h  — gitignored
#define WIFI_SSID     "your-wifi"
#define WIFI_PASSWORD "your-password"
#define API_KEY       "your-firebase-web-api-key"
#define USER_EMAIL    "device@example.com"
#define USER_PASSWORD "firebase-user-password"
#define STORAGE_BUCKET_ID "your-project.appspot.com"
```

**3. Push an update**
- Build your new firmware → `.pio/build/.../firmware.bin`
- Upload it to your bucket at `test/firmware.bin`
- Power-cycle the device — it finds the file, flashes, deletes it, and reboots.

## Check-at-boot vs. periodic checks

The simple version checks once, in `setup()`. To update a deployed device you upload the binary and power-cycle it. If you'd rather have devices poll on their own, move the `downloadOTA` block into a timer in `loop()` — same call, different trigger.

## Full code

Everything — the sketch, `platformio.ini`, and a `secrets.h.example` template — is on GitHub:

👉 **[github.com/Saipreetham0/ESP32-FOTA-Firebase](https://github.com/Saipreetham0/ESP32-FOTA-Firebase)**

If it saves you from writing an OTA server, a ⭐ on the repo is appreciated — it helps other makers find it.

Questions or improvements? Open an issue or drop a comment below. 👇
