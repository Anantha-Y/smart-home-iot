# HiveMQ Cloud MQTT Setup

## 1. Create a free HiveMQ Cloud cluster

1. Go to [https://console.hivemq.cloud](https://console.hivemq.cloud)
2. Sign up → **Create Cluster** → choose the **Free tier** (Serverless)
3. Note your **Cluster URL** (e.g. `abc123.s1.eu.hivemq.cloud`)

## 2. Create credentials

1. In the HiveMQ console → **Access Management** → **Credentials**
2. Add a new user:
   - **Username**: `smarthome_user`
   - **Password**: *choose a strong password*
3. Under **Permissions**, grant publish/subscribe to `home/#`

## 3. Update firmware

Edit `src/esp8266/secrets.h` (copy from `secrets.h.example`):

```c
#define WIFI_SSID     "YourNetworkName"
#define WIFI_PASSWORD "YourWiFiPassword"
#define MQTT_PASSWORD "YourHiveMQPassword"
```

Also update the broker hostname in `smart_home_esp8266.ino`:

```c
#define MQTT_BROKER   "abc123.s1.eu.hivemq.cloud"  // ← your cluster URL
#define MQTT_USER     "smarthome_user"
```

## 4. Topic structure

| Topic | Direction | Payload | Example |
|---|---|---|---|
| `home/cmd` | Subscribe (ESP8266 receives) | JSON | `{"room":"kitchen","state":"ON"}` |
| `home/status` | Publish (ESP8266 sends) | JSON | See below |
| `home/mode` | Subscribe (ESP8266 receives) | String | `AUTO` or `MANUAL` |

### `home/status` payload

```json
{
  "temperature": 30.1,
  "humidity": 62.0,
  "ldr": 95,
  "light": "DARK",
  "mode": "AUTO",
  "relays": {
    "kitchen": true,
    "hall": true,
    "bedroom": false,
    "outdoor": true
  }
}
```

## 5. Test with MQTT Explorer

[MQTT Explorer](https://mqtt-explorer.com/) (free desktop app):

- Host: `abc123.s1.eu.hivemq.cloud`
- Port: `8883`
- TLS: enabled
- Username / Password: as configured above

Subscribe to `home/#` to see all messages.
Publish `{"room":"kitchen","state":"ON"}` to `home/cmd` to test.

## 6. TLS note

The firmware uses `wifiClient.setInsecure()` which skips certificate
verification. This is acceptable for a closed student project but should
be replaced with `wifiClient.setTrustAnchors(&cert)` for production.
HiveMQ's root CA is Let's Encrypt ISRG Root X1.
