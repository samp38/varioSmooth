# VarioSmooth v2 — BLE

Notes de conception pour la modernisation du vario (2026-10).

## Objectifs
- MCU **nRF52840** avec BLE : paramétrage depuis le téléphone + envoi de l'altitude baro à **LK8000** (Android).
- Schéma sous **KiCad**, carte redessinée en **PCB souple** pour l'intégration dans le casque.
- Firmware compilé/flashé avec **PlatformIO**.
- **Contrainte n°1 : la consommation.**

## Choix du MCU : module Raytac MDBT50Q (nRF52840)
- **nRF52840 plutôt que nRF52832** (seuls supportés par le core Adafruit) :
  - entrée **VDDH (jusqu'à 5,5 V) + DC/DC intégré** → alimentation directe sur la LiPo, suppression du LDO (ADP160) ;
  - **USB natif** → bootloader UF2, port série de debug (pads D+/D− sur le flex, inutilisés en vol).
- **Module plutôt que chip nu** : antenne, quartz et certification radio inclus, pas de conception RF sur flex.
- **MDBT50Q** = module des Feather nRF52840 → définition de carte et bootloader déjà présents dans le core Adafruit.
- À vérifier dans la datasheet : broches d'alim exposées (VDDH, VBUS, DCCH) et selfs DC/DC intégrées ou non.
- Alternatives : **Insight SiP ISP1807** (~8×8×1 mm, plus cher, LGA, pas de variante Adafruit) ; nRF52840 nu (aQFN73, RF + certification à faire).
- Écarté : nRF54L15 (Zephyr/NCS uniquement, pas de support PlatformIO).

## Framework : PlatformIO + core Arduino Adafruit
- Core Arduino **Adafruit nRF52** : FreeRTOS + SoftDevice S140 (pile BLE Nordic), lib **Bluefruit**, bootloader UF2 + DFU OTA BLE.
- Dans PlatformIO : `platform = nordicnrf52`, `framework = arduino`, `board = adafruit_feather_nrf52840`.
- Autres cores écartés : Arduino Mbed OS (Mbed en fin de vie), forks Seeed.
- **Zephyr écarté** : gain estimé ~10–50 µA en vol sur 0,3–1+ mA (1–5 %), nul à l'arrêt. Le support Zephyr de PlatformIO est en retard (et NCS non supporté).
- Pièges conso sous Arduino :
  - USB CDC actif (garde HFCLK, centaines de µA) ;
  - `loop()` qui tourne sans dormir ;
  - tick FreeRTOS 1024 Hz ;
  - périphériques non coupés ;
  - LED bleue de connexion → `Bluefruit.autoConnLed(false)`.

## Budget conso estimé (en vol, non mesuré)
| Poste | Estimation |
|---|---|
| Capteur MS5611 OSR 4096 @ ~50 Hz (comme la v1) | ~500–700 µA |
| Capteur à fréquence/résolution réduite (ex. 20 Hz) | ~100–300 µA |
| CPU nRF (I2C par DMA, sommeil sinon) | ~20–50 µA |
| BLE connecté (intervalle ~100 ms, trames 4–10 Hz) | ~20–60 µA |
| Buzzer | centaines de µA à quelques mA quand il bipe |
| Plancher repos Zephyr / Arduino | ~2–4 µA / ~5–50 µA |
| Arrêt (System OFF) | ~0,5–1 µA (identique) |

→ Dans la v1, le **capteur** dominait (~0,6 mA sur 1 mA). Les leviers principaux : fréquence/résolution capteur (ou BMP581), buzzer, paramètres BLE.

## Liaison LK8000 (Android)
Vérifié dans le code source de LK8000 (`android/src/org/LK8000/BluetoothGattClientPort.java`, `Common/Source/Android/BluetoothSensor.cpp`) :
- **Port série BLE type HM-10** : service `FFE0`, caractéristique `FFE1` → trames NMEA, ex. **`$LK8EX1`** (pression, altitude, vario, température, batterie). ✅ **Choix retenu** (sert aussi pour les commandes de paramétrage).
- **Environmental Sensing Service** `0x181A` : pression `0x2A6D`, température `0x2A6E` ; + Battery Service `0x180F`. Option possible plus tard.
- ❌ **Nordic UART Service non supporté.**
- Le scan LK8000 filtre sur les UUID annoncés → annoncer `FFE0` (ou `0x181A`) dans l'advertising.

## App de paramétrage
- Page **Web Bluetooth** (Chrome Android/desktop) plutôt qu'une app native : rien à publier ni maintenir.
- Commandes texte via le même canal série BLE (FFE0/FFE1).

## Autres pistes hardware
- **Buzzer** : PWM du nRF avec 2 canaux en opposition = pont en H natif (plus de comparateur ni de MCP4542) ; volume par rapport cyclique.
- **Récupération solaire** : remplacer le LTC4071 (shunt) par un récupérateur MPPT (TI BQ25570/BQ25505, e-peas AEM10941, Iq ~0,5 µA) ; adapter cellules série/parallèle à la tension d'entrée.
- **Batterie** : petite LiPo souple 40–100 mAh au lieu de la LIR2016 (~20 mAh).
- **Capteur** : MS5611 (référence) vs **BMP581** (2×2 mm, très basse conso, FIFO/IIR intégrés, bruit à valider). Accéléromètre en option (fusion Kalman), coûteux en conso.
- **PCB souple** : raidisseurs sous module/capteur ; capteur protégé du vent (mousse) ; antenne orientée vers l'extérieur, zone dégagée respectée.

## Mesure de consommation
### Outil : PPK2 (Nordic Power Profiler Kit II, ~100 €)
Pas indispensable, mais fortement recommandé. Un multimètre lisse mal les pics (µA → 10 mA en quelques µs), et sa chute de tension en calibre µA fait redémarrer le nRF.
- Mesure 200 nA → 1 A, calibre automatique, 100 kéch/s.
- Alimentation réglable 0,8–5 V (mode source) ou ampèremètre en série.
- 8 entrées logiques synchronisées avec la courbe.
- Moyenne sur fenêtre (nRF Connect Power Profiler).
- Alternatives : multimètre (états stables seulement), CurrentRanger (+ oscillo), Joulescope / Otii Arc (5–10× plus chers), décharge batterie (lent).

### Montage sur la Feather nRF52840 Express
| Parasite | Impact | Action |
|---|---|---|
| NeoPixel | ~0,5–1 mA permanent | dessouder / couper l'alim |
| LDO AP2112 | dizaines de µA | **EN → GND** |
| Pont diviseur VBAT (100k/100k) | ~20 µA | pas de batterie |
| Chargeur MCP73831 | négligeable | pas d'USB |
| Flash QSPI | quelques µA | veille profonde (cmd `0xB9`) |
| LED rouge / bleue | si allumées | `Bluefruit.autoConnLed(false)` |

1. PPK2 en mode source **3,3 V sur la broche 3V** + GND, sans USB ni batterie.
2. Entrées logiques du PPK2 sur des GPIO de repère (capteur, I2C, BLE, bip).
3. Vérifier **VBAT ≈ 0 V** (sinon retour de courant par le LDO).
4. **Firmware « plancher »** : flash en veille profonde + `sd_power_system_off()` → attendu ~0,5 µA. Quelques µA = OK (à soustraire) ; des dizaines ou centaines = parasite restant.
- Capteur : sur les cartes du commerce (ex. GY-63), contourner le régulateur et mesurer séparément. Pull-ups I2C 10 kΩ = 330 µA par ligne tant qu'elle est à l'état bas.
- Limites : la Feather alimente le nRF en 3,3 V, alors que la carte finale l'alimentera directement par la LiPo via VDDH (chiffres proches mais pas identiques). Option propre : **nRF52840 DK** (~50 €), qui a un point de mesure prévu pour le PPK2.

## Plan
1. **Prototype** : Feather nRF52840 + carte capteur, PlatformIO/Adafruit, mesures au PPK2, liaison LK8000.
2. **Schéma KiCad** (l'ancien Eagle n'est importé que pour archive).
3. **PCB souple** avec MDBT50Q sur raidisseur, alimenté directement par la LiPo via VDDH, assemblage chez le fabricant.
