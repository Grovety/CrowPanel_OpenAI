# OpenAI Realtime API Console for ELECROW CrowPanel Advance 5.0-HMI

This application demonstrates OpenAI Realtime API usage on an ESP32-S3 device with a 5-inch HMI LCD panel. It provides a graphical user interface (GUI) for configuring WiFi settings and entering your OpenAI API key, then establishes a WebRTC communication with the OpenAI Realtime API. Audio input is sent to the model, which returns text responses and a transcrioption of the audio.
**Note:** The application supports only English.

## Features

- **Embedded Device Focus:** Designed for ELECROW CrowPanel Advance 5.0-HMI. For detailed device hardware information, see [Device Hardware Documentation](https://www.elecrow.com/pub/wiki/CrowPanel_Advance_5.0-HMI_ESP32_AI_Display.html).
- **Real-time Communication:** Establishes a WebRTC connection with OpenAI Realtime API.
- **Voice Interaction:** Transcribes audio input and displays the model’s text responses.
- **User-friendly GUI:** Built using LVGL 8.4.
- **Session Persistence:** WiFi settings and session configurations are saved in non-volatile storage.
- **Easy Build & Flash:** Build from source using ESP-IDF v5.4 or flash prebuilt images.

## Installation

### Building from Source

1. Install ESP-IDF framework v5.4.
2. Clone the repository.
3. Dependencies are installed via the framework component manager (see `idf_component.yml`).
4. Build and flash using the following commands:
   ```bash
   idf.py build
   idf.py -p PORT flash
   ```

### Flashing Prebuilt Images

- Use `flash_tool.exe` to flash the prebuilt images.

## Usage

1. **WiFi Setup:** Navigate to the *WiFi* tab and enter your SSID and password.
2. **Authentication:** Go to the *Auth* tab and input your OpenAI API key (non-free tier account required).
3. **Realtime Communication:** On a successful WebRTC connection, a terminal window will display your audio transcriptions and the corresponding model responses.
4. **Session Controls:** Use the terminal to clear the screen or disconnect and stop communication.

## Dependencies

- **ESP-IDF Components:** All dependencies are listed in the `idf_component.yml` file and are downloaded automatically.
- **LVGL 8.4:** Used for the user interface.
- **ESP WebRTC Examples:** Heavily inspired by [Espressif's WebRTC Solution](https://github.com/espressif/esp-webrtc-solution).
