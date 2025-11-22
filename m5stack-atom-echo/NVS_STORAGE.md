# NVS (Non-Volatile Storage) - Guida Tecnica

## 📦 Cos'è NVS?

**NVS (Non-Volatile Storage)** è il sistema di storage permanente dell'ESP32 che salva dati nella **memoria flash** in modo persistente.

### Caratteristiche Chiave

✅ **Persistenza**: Dati sopravvivono a reset, power-off, e riavvii
✅ **Flash-based**: Salvato nella partizione flash interna ESP32
✅ **Key-Value Store**: Organizzato come database chiave-valore
✅ **Namespace**: Organizzazione logica dei dati in namespace separati
✅ **Wear Leveling**: Distribuzione scritture per aumentare vita flash
✅ **Thread-Safe**: Accesso sicuro da multiple task

### NVS vs EEPROM

| Feature | NVS (ESP32) | EEPROM (Arduino) |
|---------|-------------|------------------|
| Storage | Flash interna | Emulazione flash |
| Capacità | ~96KB (dipende da partizione) | 512 bytes - 4KB |
| Organizzazione | Key-Value + Namespace | Array lineare |
| Wear Leveling | ✅ Built-in | ❌ Manuale |
| API | Preferences / NVS API | EEPROM.read/write |
| Performance | Veloce | Lenta (emulazione) |

**⚠️ IMPORTANTE**: Su ESP32, **NON usare EEPROM.h** - usa sempre **Preferences.h** o **nvs.h**!

## 🔧 Come Funziona in Bambola Parlante

### Partizioni Flash ESP32

```
┌─────────────────────────────────────────┐
│         Flash ESP32 (4MB)                │
├─────────────────────────────────────────┤
│  0x1000   - Bootloader                   │
│  0x8000   - Partition Table              │
│  0x9000   - NVS (96KB) ← CREDENZIALI QUI│
│  0x10000  - Firmware App (1.3MB)        │
│  0x150000 - SPIFFS/LittleFS (1.5MB)     │
│  0x3F0000 - OTA Update Space             │
└─────────────────────────────────────────┘
```

### Namespace Usati

#### 1. **`wifi.sta`** (Gestito da ESP32 WiFi Library)

Salva automaticamente credenziali WiFi quando `WiFi.begin()` viene chiamato con successo.

**Chiavi:**
- `ssid`: Nome rete WiFi
- `password`: Password WiFi
- `bssid`: MAC address router (opzionale)

**Location fisica:**
```
Flash → Partizione NVS → Namespace "wifi.sta"
```

#### 2. **`bambola`** (Custom - Bambola Parlante)

Namespace custom per dati specifici dell'applicazione.

**Chiavi esempio:**
- `last_wifi_save`: Timestamp ultimo salvataggio WiFi
- `device_name`: Nome dispositivo (opzionale)
- `boot_count`: Contatore riavvii (opzionale)

## 📖 API e Funzioni

### Funzioni Implementate

#### `checkNVSStatus()`
Verifica stato generale NVS e mostra statistiche.

```cpp
void checkNVSStatus();
```

**Output esempio:**
```
========================================
📦 VERIFICA NVS (Non-Volatile Storage)
========================================
NVS Entries:
  Used:  5
  Free:  249
  Total: 254
  Namespace count: 2

📡 Credenziali WiFi in NVS:
  ✅ SSID salvato: MuseoWiFi
  ✅ Password salvata: *** (nascosta)
  📍 Location: NVS Partition 'nvs' → Namespace 'wifi.sta'
  💾 Tipo storage: FLASH non-volatile (permanente)

💾 Info Flash ESP32:
  Flash size: 4 MB
  Flash speed: 40 MHz
========================================
```

#### `printWiFiCredentials()`
Mostra credenziali WiFi salvate in NVS.

```cpp
void printWiFiCredentials();
```

**Output esempio:**
```
========================================
📡 CREDENZIALI WIFI SALVATE IN NVS
========================================
SSID:     MuseoWiFi
Password: **********

✅ Credenziali trovate in NVS!
📍 Namespace: 'wifi.sta'
📍 Partizione: 'nvs' (flash non-volatile)
💾 Persistenza: PERMANENTE (sopravvive a reset/power-off)
========================================
```

#### `saveToNVS(key, value)`
Salva dato custom in namespace "bambola".

```cpp
void saveToNVS(const char* key, const char* value);

// Esempio:
saveToNVS("device_name", "Bambola_01");
```

#### `loadFromNVS(key, default)`
Carica dato custom da namespace "bambola".

```cpp
String loadFromNVS(const char* key, const char* defaultValue = "");

// Esempio:
String deviceName = loadFromNVS("device_name", "Bambola_Unknown");
```

#### `eraseNVS()` ⚠️ PERICOLOSO!
Cancella **TUTTO** il NVS (incluse credenziali WiFi).

```cpp
void eraseNVS();  // ⚠️ Usare SOLO per debug!
```

## 🔄 Ciclo di Vita Credenziali WiFi

### 1. Prima Configurazione

```
User → Smartphone → Portale Captive → WiFiManager
                                          ↓
                        WiFi.begin(ssid, password)
                                          ↓
                           ESP32 WiFi Library
                                          ↓
                        Salva in NVS "wifi.sta"
                                          ↓
                           FLASH ESP32 💾
```

### 2. Riavvio Dispositivo

```
Power On → ESP32 Boot → WiFiManager.autoConnect()
                                  ↓
                     Legge NVS "wifi.sta"
                                  ↓
                       Trova credenziali?
                            ↓          ↓
                          SÌ          NO
                            ↓          ↓
                   WiFi.begin()   Crea Hotspot
                            ↓
                      Connessione
```

### 3. Reset WiFi

```
User → Bottone 3s → checkWiFiReset()
                          ↓
              wifiManager.resetSettings()
                          ↓
               Cancella "wifi.sta" da NVS
                          ↓
                     ESP.restart()
                          ↓
                    Modalità Config
```

## 💾 Dettagli Tecnici Storage

### Struttura NVS Entry

Ogni entry in NVS contiene:
- **Namespace**: 15 chars max (es: "wifi.sta", "bambola")
- **Key**: 15 chars max (es: "ssid", "password")
- **Type**: uint8, int32, string, blob, etc.
- **Value**: Fino a 4000 bytes per entry

### Capacità Storage

**Partizione NVS Default:**
- Dimensione: 20KB - 96KB (dipende da partition table)
- Entries: ~254 entries totali
- Overhead: ~20 bytes per entry

**Credenziali WiFi:**
- SSID: ~32 bytes
- Password: ~64 bytes
- Totale: ~96 bytes + overhead

### Wear Leveling

ESP32 implementa wear leveling automatico:
- **Cicli scrittura flash**: ~100,000 cicli per settore
- **Distribuzione**: Scritture distribuite su settori diversi
- **Vita stimata**: 10+ anni con configurazioni normali

### Performance

| Operazione | Tempo |
|------------|-------|
| Lettura entry | ~1-2 ms |
| Scrittura entry | ~10-20 ms |
| Cancellazione namespace | ~50-100 ms |
| Erase completo NVS | ~200-500 ms |

## 🛠️ Debugging NVS

### Monitor Seriale

Quando dispositivo si avvia, vedrai:

```
========================================
📦 VERIFICA NVS (Non-Volatile Storage)
========================================
NVS Entries:
  Used:  5
  Free:  249
  Total: 254
```

**Interpretazione:**
- **Used**: Numero di entry attualmente salvate
- **Free**: Entry disponibili
- **Total**: Capacità totale

### Comandi Debug Seriale

Durante sviluppo, puoi aggiungere comandi seriali per debug:

```cpp
void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');

        if (cmd == "nvs_info") {
            checkNVSStatus();
        }
        else if (cmd == "wifi_info") {
            printWiFiCredentials();
        }
        else if (cmd == "nvs_erase") {
            eraseNVS();
            ESP.restart();
        }
    }
}
```

### Esportare NVS da Flash

Per backup/analisi, puoi estrarre partizione NVS:

```bash
# Con esptool.py
esptool.py --port /dev/ttyUSB0 read_flash 0x9000 0x6000 nvs_dump.bin

# Analizza con nvs_partition_parser
python nvs_partition_parser.py nvs_dump.bin
```

## 🔐 Sicurezza

### Protezione Credenziali

**⚠️ Le credenziali WiFi sono in chiaro nella flash!**

**Mitigazioni:**
1. **Flash Encryption** (opzionale):
   ```cpp
   // In menuconfig:
   Security features → Enable flash encryption on boot
   ```

2. **NVS Encryption** (opzionale):
   ```cpp
   // Usa nvs_flash_secure_init() invece di nvs_flash_init()
   ```

3. **Protezione fisica**:
   - Non lasciare dispositivi incustoditi
   - Considerare coating per impedire lettura flash

### Password WiFi Visibili?

**SÌ**, chiunque con accesso fisico al dispositivo può:
1. Collegare via USB
2. Leggere flash con esptool
3. Estrarre credenziali WiFi

**Soluzione**: Usa flash encryption per produzione.

## 📋 Best Practices

### ✅ Fare

1. **Usa Preferences API** (non EEPROM)
   ```cpp
   preferences.begin("myapp", false);
   preferences.putString("key", "value");
   preferences.end();
   ```

2. **Chiudi sempre namespace**
   ```cpp
   preferences.begin("myapp", false);
   // ... operazioni ...
   preferences.end();  // ← IMPORTANTE!
   ```

3. **Usa namespace descrittivi**
   ```cpp
   preferences.begin("config", false);    // ✅ Buono
   preferences.begin("c", false);         // ❌ Cattivo
   ```

4. **Limita scritture**
   - Salva solo quando necessario
   - Raggruppa scritture multiple
   - Non scrivere in loop ad alta frequenza

### ❌ Evitare

1. **Non usare EEPROM.h su ESP32**
   ```cpp
   #include <EEPROM.h>  // ❌ NO su ESP32!
   ```

2. **Non scrivere troppo spesso**
   ```cpp
   // ❌ Cattivo - scrive ogni secondo
   void loop() {
       preferences.putInt("counter", counter++);
       delay(1000);
   }
   ```

3. **Non dimenticare di chiudere**
   ```cpp
   preferences.begin("myapp", false);
   preferences.putString("key", "value");
   // ❌ Manca preferences.end()!
   ```

## 🔍 FAQ

### Q: Le credenziali WiFi vengono cancellate al riavvio?
**A:** NO. NVS è permanente, credenziali sopravvivono a power-off e reset.

### Q: Cosa succede se flash si riempie?
**A:** NVS restituisce errore `ESP_ERR_NVS_NOT_ENOUGH_SPACE`. Cancella entry non necessarie.

### Q: Posso salvare immagini/file grandi?
**A:** NO. NVS è per piccoli dati (<4KB per entry). Per file grandi usa SPIFFS/LittleFS.

### Q: Reset WiFi cancella anche dati custom?
**A:** NO. `wifiManager.resetSettings()` cancella solo namespace `wifi.sta`. Dati in namespace `bambola` restano.

### Q: Quante volte posso scrivere?
**A:** ~100,000 cicli per settore grazie a wear leveling. Sufficiente per anni.

### Q: NVS funziona con OTA update?
**A:** SÌ. NVS è separato da firmware, credenziali sopravvivono agli update OTA.

## 📚 Riferimenti

- [ESP-IDF NVS Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html)
- [Arduino ESP32 Preferences](https://github.com/espressif/arduino-esp32/tree/master/libraries/Preferences)
- [ESP32 Partition Tables](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html)

---

**💾 Con NVS, le tue credenziali WiFi sono al sicuro nella flash ESP32!**
