# Guida Configurazione WiFi - Portale Captive

La Bambola Parlante usa **WiFiManager** per una configurazione WiFi semplice e dinamica, senza bisogno di modificare il codice!

## 🎯 Vantaggi

✅ **Zero configurazione del codice** - Non serve modificare `config.h`
✅ **Portale web automatico** - Si apre automaticamente su smartphone
✅ **Memorizzazione credenziali** - Salva WiFi in **NVS flash** permanentemente
✅ **Facilità d'uso** - Perfetto per musei e installazioni
✅ **Reset semplice** - Basta tenere premuto il bottone all'avvio

**📦 Storage NVS**: Le credenziali WiFi vengono salvate nella partizione **NVS (Non-Volatile Storage)** della flash ESP32, garantendo persistenza permanente anche dopo spegnimento completo e reset. Dettagli tecnici: [NVS_STORAGE.md](NVS_STORAGE.md)

## 📱 Prima Configurazione

### Step 1: Accensione Dispositivo

1. Collega M5Stack Atom Echo via USB
2. Il LED diventa **🔴 ROSSO** (nessun WiFi configurato)
3. Dopo 30 secondi, il LED diventa **🟠 ARANCIONE lampeggiante**
4. Il dispositivo crea un hotspot: **"Bambola_Setup"**

### Step 2: Connessione con Smartphone

1. Apri impostazioni WiFi su smartphone/tablet
2. Cerca rete WiFi: **"Bambola_Setup"**
3. Connettiti (nessuna password richiesta)
4. Il portale web si apre **AUTOMATICAMENTE**

   > Se il portale non si apre, vai a: `http://192.168.4.1`

### Step 3: Configurazione WiFi

Il portale mostra questa schermata:

```
╔════════════════════════════════════╗
║  Bambola Parlante                  ║
║  Configurazione WiFi               ║
╠════════════════════════════════════╣
║                                    ║
║  📡 Seleziona rete WiFi:           ║
║  [ ] MuseoWiFi                     ║
║  [ ] WiFi_Museo_2024              ║
║  [ ] Museo_Guest                   ║
║                                    ║
║  oppure inserisci manualmente:     ║
║                                    ║
║  SSID:     [________________]      ║
║  Password: [________________]      ║
║                                    ║
║  [Scan] [Save] [Exit]             ║
╚════════════════════════════════════╝
```

**Opzione A: Seleziona dalla lista**
- Clicca su una rete WiFi rilevata
- Inserisci password
- Clicca **"Save"**

**Opzione B: Inserimento manuale**
- Scrivi SSID del museo
- Scrivi password
- Clicca **"Save"**

### Step 4: Connessione Automatica

1. Il dispositivo salva le credenziali
2. Si disconnette dall'hotspot
3. Si connette al WiFi del museo
4. LED diventa **🟢 VERDE** (connesso!)
5. Poi **🔵 BLU** (pronto per uso)

**✅ FATTO! La configurazione è permanente.**

## 🔄 Cambiare WiFi / Reset Configurazione

### Metodo 1: Tenere Premuto il Bottone all'Avvio

1. **Spegni** il dispositivo
2. **Tieni premuto il bottone**
3. **Accendi** il dispositivo (mentre tieni premuto)
4. LED lampeggia **🔴 ROSSO veloce**
5. **Continua a tenere premuto per 3 secondi**
6. LED si spegne → Reset completato!
7. Dispositivo si riavvia
8. Ricomincia da "Prima Configurazione" sopra

### Metodo 2: Via Seriale (per tecnici)

Connetti al monitor seriale e invia:
```cpp
wifiManager.resetSettings();
ESP.restart();
```

## 🎨 Guida LED durante Setup

| LED | Significato | Cosa Fare |
|-----|-------------|-----------|
| 🔴 Rosso fisso | Nessun WiFi configurato | Attendi 30 secondi |
| 🟠 Arancione lampeggiante | Modalità configurazione attiva | Connettiti a "Bambola_Setup" |
| ⚪ Bianco | Connessione WiFi in corso | Attendi... |
| 🟢 Verde | WiFi connesso | Setup completato! |
| 🔵 Blu | Dispositivo pronto | Usa normalmente |
| 🔴 Rosso veloce | Reset in corso | Tieni premuto bottone |

## ⚙️ Configurazione Avanzata

### Cambiare Nome Hotspot

Modifica `src/config.h`:

```cpp
#define WIFI_AP_NAME "MuseoBambola_001"  // Nome personalizzato
```

### Aggiungere Password all'Hotspot

```cpp
#define WIFI_AP_PASSWORD "password123"  // Proteggi hotspot
```

### Timeout Portale Configurazione

```cpp
#define WIFI_CONFIG_TIMEOUT 300  // 5 minuti (default: 180s)
```

### WiFi Fallback di Emergenza

Se il portale captive non funziona, puoi configurare un WiFi di backup:

```cpp
#define WIFI_FALLBACK_SSID "WiFiEmergenza"
#define WIFI_FALLBACK_PASSWORD "password123"
```

Il dispositivo userà questo WiFi se la configurazione captive fallisce.

## 🐛 Risoluzione Problemi

### ❌ Portale Non Si Apre Automaticamente

**Soluzione:**
1. Vai manualmente a: `http://192.168.4.1`
2. Oppure: `http://bambola.local` (alcuni sistemi)

**Causa comune:** Alcune versioni Android non aprono portali captive.

### ❌ Non Vedo Hotspot "Bambola_Setup"

**Verifica:**
- LED è arancione lampeggiante?
- Attendi 30 secondi dopo accensione
- Fai scan WiFi manuale su smartphone
- Prova reset: tieni premuto bottone all'avvio

### ❌ Configurazione Salvata Ma Non Si Connette

**Soluzioni:**
1. Verifica password WiFi corretta
2. Controlla che WiFi museo sia 2.4GHz (non 5GHz)
3. Resetta e riprova configurazione
4. Controlla log seriale per dettagli errore

### ❌ Dispositivo Non Ricorda WiFi Dopo Riavvio

**Causa:** NVS corrotto o partizione non inizializzata

**Soluzione:**
1. Verifica log seriale - dovrebbe mostrare info NVS all'avvio
2. Controlla partizione NVS nel partition table
3. Se NVS è corrotto, re-flash completo del firmware:
   ```bash
   esptool.py erase_flash
   pio run --target upload
   ```

### ❌ WiFi Museo Ha Portale Captive

Alcuni WiFi pubblici richiedono login aggiuntivo (portale captive del museo).

**Soluzioni:**
1. Usa WiFi senza portale captive
2. Configura IP statico con credenziali museo
3. Contatta IT museo per whitelist MAC address dispositivo

## 📊 Monitor Seriale - Output di Esempio

### Prima Configurazione (Nessun WiFi)

```
========================================
🎭 Bambola Parlante - M5Stack Atom Echo
========================================
Versione: 1.1 (WiFi Manager Edition)
========================================

💡 SUGGERIMENTO: Tieni premuto il bottone per 3 secondi
   per resettare la configurazione WiFi

🎤 Inizializzazione I2S...
🔊 Inizializzazione codec Opus...

========================================
📡 Inizializzazione WiFi Manager
========================================
Tentativo di connessione WiFi...

⚠️  Nessuna credenziale WiFi salvata
Avvio modalità configurazione...

========================================
🔧 MODALITÀ CONFIGURAZIONE ATTIVA
========================================
Hotspot: Bambola_Setup
IP: 192.168.4.1

📱 ISTRUZIONI:
1. Connetti smartphone a WiFi: Bambola_Setup
2. Apri browser (portale si apre automaticamente)
3. Inserisci SSID e password del museo
4. Clicca 'Save'
========================================
```

### Configurazione Salvata e Connessa

```
✅ Configurazione WiFi salvata!

========================================
✅ WiFi CONNESSO!
========================================
SSID: MuseoWiFi
IP: 192.168.1.142
Signal: -45 dBm
========================================

WebSocket Connected to: your-vm.lightning.ai
Session updated successfully
```

### Reset Configurazione

```
========================================
🔄 RESET CONFIGURAZIONE WIFI
========================================
Rilascia il bottone per confermare reset...
✅ Reset confermato! Cancellazione credenziali...
🔄 Riavvio...
```

## 🏢 Setup Multi-Dispositivo per Musei

### Scenario: 10 Bambole in Museo

**Opzione 1: Configurazione Individuale**
- Personalizza nome hotspot per ogni dispositivo:
  ```cpp
  #define WIFI_AP_NAME "Bambola_Sala_A"
  #define WIFI_AP_NAME "Bambola_Sala_B"
  // etc.
  ```

**Opzione 2: Configurazione di Massa via NVS**
1. Configura prima bambola via portale
2. Estrai partizione NVS e clonala su altre bambole:
   ```bash
   # Leggi NVS da prima bambola
   esptool.py --port /dev/ttyUSB0 read_flash 0x9000 0x6000 nvs_template.bin

   # Scrivi su altre bambole
   esptool.py --port /dev/ttyUSB1 write_flash 0x9000 nvs_template.bin
   ```

**Opzione 3: WiFi Fallback**
- Configura stesso fallback WiFi su tutte:
  ```cpp
  #define WIFI_FALLBACK_SSID "MuseoWiFi_Bambole"
  #define WIFI_FALLBACK_PASSWORD "MuseoPassword2024"
  ```

## 📱 Screenshot Portale Web

Il portale mostra:

1. **Pagina principale:**
   - Lista reti WiFi rilevate con segnale
   - Pulsante "Scan" per aggiornare lista
   - Campi per SSID e password manuali

2. **Opzioni avanzate:**
   - IP statico (opzionale)
   - Gateway
   - DNS

3. **Info dispositivo:**
   - MAC Address
   - Versione firmware
   - Tempo di uptime

## 🔐 Sicurezza

### Proteggere Hotspot Setup

Per ambienti pubblici, aggiungi password:

```cpp
#define WIFI_AP_PASSWORD "SetupMuseo2024"
```

Comunica password solo al personale museo.

### Timeout Automatico

Il portale si chiude automaticamente dopo 3 minuti di inattività:

```cpp
#define WIFI_CONFIG_TIMEOUT 180  // secondi
```

Previene accessi non autorizzati.

## 📞 Supporto

**Problemi comuni:**
- ✅ 90% problemi risolti con reset (tieni premuto bottone)
- ✅ Verifica sempre WiFi sia 2.4GHz
- ✅ Controlla log seriale per dettagli

**Contatto tecnico:**
- Apri issue su GitHub repository
- Allega log seriale completo
- Specifica modello router museo

---

**✨ Con WiFiManager, setup WiFi richiede solo 2 minuti tramite smartphone!**
