# ESP32 FOTA via Firebase Storage

<p align="left">
  <a href="https://github.com/Saipreetham0/ESP32-FOTA-Firebase/stargazers"><img alt="Stars" src="https://img.shields.io/github/stars/Saipreetham0/ESP32-FOTA-Firebase?style=social"></a>
  <a href="https://github.com/Saipreetham0/ESP32-FOTA-Firebase/network/members"><img alt="Forks" src="https://img.shields.io/github/forks/Saipreetham0/ESP32-FOTA-Firebase?style=social"></a>
  <img alt="License" src="https://img.shields.io/github/license/Saipreetham0/ESP32-FOTA-Firebase">
</p>

> ⭐ **Found this useful? Star the repo** — it helps other makers find it.

## 📸 Demo

<!-- Drop a wiring photo and a short demo GIF into docs/ then uncomment:
![Demo](docs/demo.gif)
![Wiring](docs/wiring.jpg)
-->
_Demo GIF and wiring photo coming soon._


Firmware **O**ver-**T**he-**A**ir updates for ESP32 using **Firebase Cloud Storage** — no custom update server needed. The device checks a fixed path in your storage bucket at boot; if a `firmware.bin` is there, it downloads it, flashes itself, deletes the file, and reboots into the new firmware.

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

## Why this approach

- **No server to run** — Firebase Storage is the update server (free tier is plenty)
- **Authenticated** — devices sign in with Firebase email/password auth; your bucket isn't public
- **One-shot updates** — the binary is deleted after a successful flash, so devices don't re-flash in a loop

## Hardware

| Part | Notes |
|---|---|
| ESP32 DevKit v1 | any ESP32 dev board works (adjust `board` in `platformio.ini`) |
| 20×4 I²C LCD *(optional)* | address `0x27`, SDA → GPIO 21, SCL → GPIO 22 — shows WiFi status |
| Onboard LED | GPIO 2, blinks in the main loop as a version heartbeat |

## Setup

### 1. Firebase project

1. Create a project at [console.firebase.google.com](https://console.firebase.google.com)
2. **Authentication → Sign-in method** → enable **Email/Password**, then add a user (this is the *device's* login, e.g. `device1@yourproject.com`)
3. **Storage** → create a bucket (note the `xxxx.appspot.com` ID)
4. **Project settings → General** → copy the **Web API Key**

### 2. Credentials

```bash
cp include/secrets.h.example include/secrets.h
# edit include/secrets.h with your WiFi + Firebase values
```

`include/secrets.h` is gitignored — real credentials never get committed.

### 3. Build & flash (first time over USB)

```bash
pio run -t upload && pio device monitor
```

## Pushing an OTA update

1. Bump `currentVersion` in `src/main.cpp`, make your changes
2. Build: `pio run` → binary is at `.pio/build/esp32doit-devkit-v1/firmware.bin`
3. Upload it to your bucket at `test/firmware.bin` (Firebase console → Storage, or `gsutil cp`)
4. Reboot/power-cycle the device — it finds the file, flashes, deletes it, restarts

Serial output during an update:

```
Checking for new firmware update available...
New update found
Downloading firmware test/firmware.bin (874512 bytes)
Downloaded 100%
Donwload firmware completed.
Delete file... ok
Restarting...
```

## How it works

The interesting part is one call from [Firebase-ESP-Client](https://github.com/mobizt/Firebase-ESP-Client):

```cpp
Firebase.Storage.downloadOTA(&fbdo, STORAGE_BUCKET_ID, FIRMWARE_PATH, fcsDownloadCallback);
```

It streams the binary from Storage straight into the ESP32 OTA partition (`Update.h` under the hood), so the full firmware never needs to fit in RAM. On success the sketch deletes the remote file and calls `ESP.restart()`.

**Check-at-boot** keeps it simple: to update a deployed device, upload the binary and power-cycle it. If you want periodic checks instead, move the `downloadOTA` block from `setup()` into a timer in `loop()`.

## Troubleshooting

- **`firmware is not for running progress` / download error at boot** — no file at `FIRMWARE_PATH`; that's normal when no update is pending
- **Auth errors** — Email/Password sign-in not enabled, or wrong Web API key
- **Download OK but boot loops** — binary built for a different board/partition scheme; rebuild with the same `platformio.ini`

## License

MIT
