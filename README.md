# TuffCalc
Version 2 (the first one was ass)

![TuffCalc powered on](./the%20thing.jpg)

It's a calculator. It does calculator stuff. It also plays Snake, Tetris, Flappy Bird, Pong, and a tiny Minecraft clone, takes notes, checks your Discord DMs, talks to an AI, and a "few" more things. You know, normal calculator stuff.

Runs on an ESP32 with two SSD1306 OLEDs (one main screen, one info screen) wired up behind a real calculator keypad, so from the outside it just looks like a calculator.

## What it can do

- **Calculator** — the actual point of the thing. Basic arithmetic, history, Ans, exponents, square roots.
- **Games** — Snake, Tetris, Flappy Bird, Pong (vs a bot), and a creative-mode Minecraft thing rendered in raycasted white blocks on a 128x64 screen.
- **Notes** — view the notes you uploaded from your phone organized in folders.
- **Wi-Fi** — scan, connect, save networks.
- **Discord** — lists your DMs and lets you read/send messages, through the companion backend server.
- **AI chat** — ask it stuff, answers come back through the same backend (Groq under the hood).
- **Evil Portal** — don't ask.
- **Clock, battery %, brightness, auto-off, app PIN lock, OTA updates** — the usual quality-of-life stuff.

Menus are PIN-protected if you set one (Settings > App PIN).

## Hardware

- ESP32 dev board
- 2x SSD1306 128x64 OLED, one on `Wire` (pins 21/22), one on `Wire1` (pins 16/17), both at I2C address `0x3C`
- A real calculator keypad wired into a key matrix across pins `0, 2, 4, 5, 13, 14, 15, 18, 19, 25, 26, 32, 33`
- Battery voltage divider into pin 34 for the battery percentage reading

## Setting it up

### 1. Flash the firmware

1. Open `main.ino` in Arduino IDE (or PlatformIO).
2. Install the libraries it needs: `Adafruit_GFX`, `Adafruit_SSD1306`, `ArduinoJson`, plus the ESP32 board package (`WiFi`, `WebServer`, `DNSServer`, `HTTPClient`, `WiFiClientSecure`, `ArduinoOTA`, `LittleFS`, `Preferences` all ship with it).
3. Edit the settings block near the top of the file:
   - `HARD_NETS[]` — your default Wi-Fi SSID/password.
   - `CALC_API_BASE` — the URL where you're running `server.js` (e.g. `http://192.168.1.50:9123`), if you want Discord/AI.
   - `CALC_API_KEY` — a random 32+ character string. Has to match `CALC_API_KEY` on the server side.
   - `OTA_PASSWORD` — change this from `123` to something real if you care about OTA security.
4. Flash it over USB the first time. After that you can push updates over Wi-Fi via Arduino IDE (Port menu > `tuffcalc.local`).

### 2. Run the backend (optional — only needed for Discord/AI)

The backend is a small Node server (`server.js`) that the calculator talks to over HTTP for the Discord and AI features. Everything else (calculator, games, notes, portal) works fully standalone without it.

```bash
npm install
CALC_API_KEY="same-random-string-as-the-firmware" \
GROQ_API_KEY="your-groq-key" \
node server.js
```

Env vars:

| Var | Required | Notes |
|---|---|---|
| `CALC_API_KEY` | yes | Must be 32+ chars and match the firmware's `CALC_API_KEY`. |
| `PORT` | no | Defaults to `9123`. |
| `HOST` | no | Defaults to `127.0.0.1`. |
| `GROQ_API_KEY` | no | Needed for the AI feature. Leave unset and it just won't work. |
| `GROQ_MODEL` | no | Defaults to `openai/gpt-oss-20b`. |

Discord is off by default in this copy (`DISCORD_ENABLED = false`) - CAN GET YOU BANNED, but it works.

### 3. First boot

- The device boots to the calculator screen. Press the MODE key to go to the main menu (Games / Tools / Settings).
- Default PIN for Discord/AI is 0000.

## A couple of notes

- The portal feature logs whatever's typed into the fake page it serves — only point it at networks/devices you own or have permission to test.
- Notes aren't backed up anywhere; "Erase all data" is final.
- Minecraft mode doesn't save your builds. It's just for messing around.
