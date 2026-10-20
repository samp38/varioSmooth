# VarioSmooth v2 — BLE

Notes de conception pour la modernisation du vario (2026-10).

## Objectifs
- MCU **nRF52840** avec BLE : paramétrage depuis le téléphone + envoi de la pression et du vario à **XCTrack** (puis LK8000) sur Android.
- **Vario sans GPS** : l'application utilise le GPS du téléphone.
- Schéma sous **KiCad**, carte redessinée en **PCB souple** pour l'intégration dans le casque.
- Firmware compilé/flashé avec **PlatformIO**.
- **Contrainte n°1 : la consommation.**

## Réorientation (2026-10-20) : deux appareils
Le vario intégré au casque cumule des contraintes contradictoires : BLE et antenne, petite batterie, fils vers les cellules et le bouton. La v1 marche, mais elle est pénible à installer : les fils passés par l'aération cassent, la carte est trop épaisse pour la doublure, et le flashage au PICkit est laborieux. Un GPS aurait aggravé tout cela. Le projet est donc scindé, sur le modèle de la gamme Stodeus (miniBip → UltraBip, tous dans un boîtier de 58 × 36 × 14 mm) :

| | **Extension** (cockpit) — en premier | **Vario casque** — ensuite |
|---|---|---|
| Rôle | Extension du téléphone : pression, vario et `$LK8EX1` vers XCTrack, **log GPS IGC de secours**, bip si le téléphone plante | Bip seul, comme la v1 |
| Fixation | Cockpit ; peut être accroché verticalement (un GNSS moderne y perd quelques dB, sans gêne en vol) | Petit boîtier sur une sangle du casque, laissé à demeure (on ne l'oublie pas) |
| Contenu | Schéma KiCad actuel (MDBT50Q, MS5611, piezo, LTC4071, USB-C) + GNSS (u-blox MAX-M10S + patch ~15 × 15 mm, UART, alimentation coupable) + flash QSPI (16 Mo) + cellules plus grandes + LiPo ≥ 500 mAh | Capteur, piezo, cellules et bouton **sur le boîtier** : plus aucun fil. USB-C pour charger et flasher |
| BLE | Désactivable | Aucun, ou seulement au sol pour les réglages |

### Extension : comportement visé
- Téléphone connecté : trames `$LK8EX1`, bip coupé (XCTrack bipe).
- Connexion perdue depuis un certain délai : bip automatique.
- Log IGC permanent à 1 Hz (~125 Ko/h, ~130 h sur 16 Mo), récupérable en **USB Mass Storage** (Adafruit_SPIFlash + FatFs + TinyUSB) : le boîtier apparaît comme une clé USB.
- Flash QSPI sur les **mêmes broches que la Feather Express** (P0.17, P0.19–P0.23) : le firmware la gère sans changement, et le logger se prototype sur la Feather (GD25Q16 2 Mo intégrée) avec une carte d'évaluation MAX-M10S.
- Le GNSS consomme 5–30 mW selon la puce (MAX-M10S < 25 mW en suivi) : c'est lui qui dimensionne la batterie. Garder une alimentation de sauvegarde (V_BCKP) pour démarrer à chaud.

### Vario casque : à trancher
- **Même MDBT50Q que l'extension**, radio coupée : un seul firmware (deux environnements PlatformIO), même flashage UF2, schéma commun. Le capteur domine la consommation (~1 mA à 50 Hz), donc un MCU plus sobre (SAMD21, STM32 L0/U0) n'apporterait que peu, pour un surcoût du module d'environ 10 € et 15 × 10 mm.
- Bouton du boîtier, ou accéléromètre (taper sur le casque, ex. LIS2DH12 ~2 µA) à prototyper.

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

## Liaison BLE avec XCTrack et LK8000 (Android)
Trame retenue : **`$LK8EX1,<pression Pa>,99999,<vario cm/s>,<température °C>,<batterie>,*<checksum>`**. L'altitude vaut 99999 quand la pression est fournie (l'application la calcule avec son QNH), batterie 999 = non disponible, sinon tension (< 1000) ou 1000 + pourcentage.

| | XCTrack | LK8000 |
|---|---|---|
| **Nordic UART Service** `6E400001-…` (TX `6E400003-…`) | ✅ | ❌ |
| Port série type HM-10 : service `FFE0`, caractéristique `FFE1` | ❌ | ✅ |
| Environmental Sensing `0x181A` (pression `0x2A6D`, température `0x2A6E`) + Battery `0x180F` | ✅ | ✅ |
| Trame `$LK8EX1` | ✅ | ✅ |

- **Choix pour le prototype : Nordic UART + `$LK8EX1`** pour XCTrack. Le core Adafruit fournit le service (`BLEUart`). La trame transporte le vario calculé par notre filtre ; avec la pression seule (ESS), l'application devrait recalculer le vario, avec plus de retard.
- **Vérifié le 2026-10-18** sur un Galaxy S9 : XCTrack se connecte, et il **affiche le vario de la trame**, alors qu'il ne liste que la pression et la température dans les données du capteur. Test fait avec un vario fixe de +3 m/s, carte immobile (environnement PlatformIO `fake_climb`).
- **LK8000 ensuite** : ajouter le service `FFE0`/`FFE1` en parallèle, avec la même trame. Les deux UUID tiennent dans le paquet d'annonce (le nom va dans la réponse au scan).
- Sources : [page External Devices de XCTrack](https://xctrack.org/External_Devices.html) ; code source de LK8000 (`android/src/org/LK8000/BluetoothGattClientPort.java`, `Common/Source/Android/BluetoothSensor.cpp`), dont le scan filtre sur les UUID annoncés.
- Dans XCTrack : choisir le capteur Bluetooth LE dans les réglages des capteurs externes ; le journal (widget log) montre les trames reçues. Pour un premier test, l'application **nRF Connect** suffit à voir les trames.

## App de paramétrage
- Page **Web Bluetooth** (Chrome Android/desktop) plutôt qu'une app native : rien à publier ni maintenir.
- Commandes texte via le canal série BLE (Nordic UART).

## Firmware (`firmware/`)
Projet PlatformIO, environnement `feather_nrf52840` : `pio run -t upload`, puis `pio device monitor` (sortie de debug sur l'USB).

| Fichier | Rôle |
|---|---|
| `src/ms5611.*` | Pilote du capteur : PROM + CRC4, compensation de température du 2e ordre (absentes en v1). Pression en OSR 4096, température en OSR 1024 (512 en v1). |
| `src/vario.*` | Algorithme de la v1 : filtre passe-bas d'ordre 2 sur la pression, dérivée sur une fenêtre de 10 à 50 échantillons selon la vitesse, filtre IIR sur la vitesse. Calcul en flottant et sur l'altitude (cm/s) plutôt que sur la pression. |
| `src/ble_link.*` | Nordic UART + trames `$LK8EX1`. |
| `src/audio.*` | Bips de la v1 (tables `beepLen` / `audioFreq`, ton à 2/3, silence en descente) et mélodies (démarrage, volume, arrêt). 3 volumes comme en v1 : boost = deux broches en opposition de phase (le comparateur de la v1 fournissait le signal inversé), high = une seule broche, medium = une 3ᵉ broche à travers une résistance (le potentiomètre numérique de la v1). Réduire le rapport cyclique ne marche pas : le son vient surtout des harmoniques, et chaque front fait sonner le piézo pareil ; il faut arrondir les fronts (RC avec la capacité du piézo). |
| `src/button.*` | Bouton scruté à chaque échantillon (anti-rebond par la période de 20 ms). Appui court : volume suivant (enregistré en flash à l'extinction seulement : une écriture bloque ~0,5 s, le capteur refroidit pendant la pause et le vario voit une fausse montée). Appui long (> 0,5 s) : arrêt en System OFF, le bouton rallume la carte. |
| `src/main.cpp` | Boucle à 50 Hz, même séquence que la v1 : lecture de la pression, conversion courte de la température, lancement de la pression suivante. Trame BLE à 10 Hz. La température doit être fraîche à chaque échantillon : la pression brute dérive d'environ 2 mbar/°C (coefficients typiques), et une température en retard se voit comme une fausse vitesse verticale. |

À venir : arrêt automatique, mesure batterie, réglages de consommation.

### Câblage du prototype (Feather nRF52840 Express)
| Breakout MS5611 (GY-63…) | Feather |
|---|---|
| VCC | 3V |
| GND | GND |
| SCL / SDA | SCL / SDA |
| PS | VCC (mode I2C, souvent déjà fait sur la carte) |
| CSB | GND → adresse 0x77 (VCC → 0x76, détectée aussi) |

Piézo (KPEG006, comme en v1), polarité indifférente :
```
A0 ──[ 220 Ω ]──┬── KPEG006 ── A1
A2 ──[ ~1 kΩ ]──┘   (volume medium, valeur à ajuster à l'oreille)
```
La 220 Ω limite les pics de courant à chaque front (bruit sur le 3,3 V).

## Bloc d'alimentation (figé le 2026-10-17)

```
SP3-12 ─ SP3-12 ── D1 (Schottky) ── RIN (0 Ω) ──┬── VCC ── VDDH (MDBT50Q) ── REG0 DC/DC ── VDD 3,0 V ── MS5611, buzzer
   (série, côte à côte sur le dessus)             │
                                    LTC4071 (shunt)── BAT ── LiPo   (NTC 10k collée à la batterie)
```

### Chargeur : LTC4071 (shunt, MSOP8 ou DFN 2 × 3 mm), comme la v1
Le **BQ25505** (MPPT) a été envisagé puis écarté. Les cellules sont côte à côte sur le dessus du casque, donc à l'ombre ensemble : la série ne pénalise pas. Et avec RIN ≈ 0, le LTC4071 récupère déjà ~75 % de la puissance des cellules, contre ~80–85 % pour le BQ25505 (estimations, non mesurées). Ce faible gain ne justifiait pas :
- l'abandon de la **coupure batterie basse intégrée** (le BQ25505 descend à 1,95 V et impose une batterie avec PCM) ;
- l'abandon de la **réduction de la tension de fin de charge à chaud** (NTC) ;
- l'ajout d'une self, d'un convertisseur à découpage à côté du capteur et de la radio, et de 8 résistances de 1 à 14 MΩ.

Principe : les cellules débitent dans VCC (la charge système). Le LTC4071 dérive l'excédent (jusqu'à 50 mA) pour maintenir VCC à la tension de fin de charge, et relie la batterie à VCC par un interrupteur interne (4–6 Ω). Iq : 550 nA typ. (1,2 µA max). Fuite batterie une fois déconnectée : < 0,1 nA typ.

| Réglage | Choix | Effet |
|---|---|---|
| **RIN** | **0 Ω** (empreinte gardée) | La datasheet n'exige RIN que si la source peut dépasser 50 mA de shunt. Isc des SP3-12 : 10,7 mA. En v1, 220 Ω coûtaient ~15 % de la récolte en plein soleil (~64 % contre ~75 %), et presque rien à faible éclairage (pertes en I²). Shunt max avec RIN = 0 : 4,1 V × 11 mA ≈ 45 mW, sans souci. |
| **ADJ** | en l'air | Fin de charge **4,1 V** (4,0 V à GND, 4,2 V à VCC). |
| **NTC / NTCBIAS** | thermistance **10 kΩ « curve 2 »** (B25/85 = 3490, ex. Vishay NTHS0402N02N1002F) de NTC à GND, **collée à la batterie** ; RNOM 10 kΩ de NTCBIAS à NTC | Avec ADJ en l'air : 4,1 V < 40 °C, puis 4,025 / 3,95 / 3,875 / 3,8 V par paliers de 10 °C. Sur un casque au soleil, c'est le principal levier de durée de vie de la LiPo. Polarisation pulsée : ~50 pA en moyenne. |
| **LBSEL** | **à VCC** | Coupure batterie à **2,7 V**, reconnexion quand VCC atteint 3,6 V. Voir la remarque ci-dessous. |
| **HBO** | non connecté | Sortie CMOS au niveau de VCC (jusqu'à 4,1 V), au-delà de ce que tolère une GPIO du nRF (3,3 V). La batterie se mesure par le SAADC. |
| **D1** | Schottky à **faible courant inverse** | Anti-retour : sans elle, les cellules déchargent la batterie la nuit (recommandé par la datasheet). Choisir sur le courant inverse sous 4 V (il augmente fortement avec la température), plus que sur le Vf. |
| **CVCC** | 10 µF (+ les 10 µF de VDDH) | Absorbe les pics d'émission BLE à travers les 4–6 Ω de l'interrupteur batterie. |

**Pourquoi LBSEL à VCC (2,7 V) et pas à GND (3,2 V) comme en v1** : après une coupure, la batterie n'est reconnectée que quand VCC atteint VLBC_VCC. Avec LBSEL à GND, ce seuil vaut **4,19 V typ.** (à 1 mA), soit **au-dessus** de la tension de fin de charge de 4,1 V (ADJ en l'air) à laquelle le shunt plafonne VCC. La reconnexion ne se fait alors qu'à faible courant de charge, où le seuil baisse : c'est **limite**. Avec LBSEL à VCC, le seuil tombe à 3,6 V, sans ambiguïté. La coupure à 2,7 V est profonde pour une LiPo : c'est un **dernier recours**, et le firmware s'arrête bien avant (voir « Gestion de l'énergie »).

### Cellules solaires
- 2 × SP3-12 **en série**, comme en v1 : Vmp ~6 V, Voc ~9 V (jusqu'à ~10 V à froid, sans conséquence pour un shunt), Isc ~10,7 mA, ~50 mW en plein soleil.
- La charge démarre dès que la tension de la chaîne dépasse Vbat + Vf de D1 (~4,2–4,5 V). Le Voc du silicium amorphe chute peu à faible éclairage, donc la charge fonctionne même par temps couvert, avec un courant proportionnel à la lumière.

### MDBT50Q (nRF52840), alimentation
- Dimensions : 10,5 × 15,5 × 2,05 mm. Le quartz 32 MHz et la self du DC/DC REG1 sont **intégrés**.
- Schéma de référence « Reg0 DC/DC Enabled » (VDDH ≥ 3,6 V) :
  - VDDH (broche 30) ← VCC du LTC4071, 10 µF (VDDH accepte 2,5–5,5 V, VCC ne dépasse pas 4,1 V) ;
  - DCCH (31) → **L 10 µH 0603 (≥ 80 mA)** → VDD (28), 10 µF ;
  - VBUS (32) : 10 µF, et D+/D− (35/34) en pads pour le bootloader UF2 et le debug. Pas de charge par USB pour l'instant (à revoir avec le choix de la batterie).
- **VDD = REGOUT0 réglé à 3,0 V** (registre UICR) plutôt que 3,3 V : le régulateur reste en régulation jusqu'à une batterie à ~3,3 V (le seuil d'arrêt du firmware), et le MS5611 accepte 1,8–3,6 V. À prévoir dans la définition de carte et le bootloader (la valeur par défaut est 1,8 V).
- **Quartz 32,768 kHz** (optionnel selon Raytac) : **retenu**. Il évite l'oscillateur RC, qui se recalibre périodiquement et impose des fenêtres de réception BLE plus larges (500 ppm), donc plus de temps radio.
- Mesure de la batterie : canal **VDDHDIV5** interne du SAADC → pas de pont diviseur externe (trame `$LK8EX1` + service Battery).

### Gestion de l'énergie
- Le firmware mesure la batterie par le SAADC (VDDHDIV5). Sous **~3,4 V**, il passe en **veille « batterie faible »** : tout est coupé (capteur, buzzer, BLE), et le nRF reste en System ON avec un réveil RTC toutes les minutes environ pour remesurer. Il redémarre au-dessus de **~3,7 V**, après une recharge solaire.
- Pas de System OFF dans ce cas : aucun signal externe ne pourrait réveiller le nRF quand la batterie remonte, puisque HBO ne monte qu'à pleine charge et dépasse le niveau toléré par les GPIO.
- Consommation estimée en veille batterie faible : nRF en System ON + RTC ~1,5–3 µA + LTC4071 0,55 µA + MS5611 en veille < 0,15 µA → **~2–4 µA**. Avec une LiPo de 40 mAh arrêtée à 3,4 V (~10 % restants), il reste plusieurs semaines avant la coupure matérielle.
- Dernier recours : le LTC4071 déconnecte la batterie à 2,7 V (fuite < 0,1 nA ensuite) et la reconnecte dès que les cellules ramènent VCC à 3,6 V. Une PCM dans la batterie n'est pas indispensable, mais elle ne gêne pas.

## Autres pistes hardware
- **Buzzer** : PWM du nRF avec 2 canaux en opposition = pont en H natif (plus de comparateur ni de MCP4542) ; volume par rapport cyclique.
- **Récupération solaire** : LTC4071 conservé, voir « Bloc d'alimentation ».
  - Cellules conservées : **2 × PowerFilm SP3-12** (silicium amorphe souple, 12,7 × 64 mm), côte à côte sur le dessus du casque. Par cellule, en plein soleil : Vmp 3,0 V / Imp 8,5 mA (~25 mW), Voc 4,5 V, Isc 10,7 mA.
  - Bilan grossier : 50 mW en plein soleil, ~5–15 mW réalistes sur le casque → ~1–3 mA de charge, contre 0,3–1 mA consommés en vol.
  - Option écartée : récupérateur MPPT (TI BQ25505, e-peas AEM10941) avec les cellules en parallèle. Il ne vaut le coup que si les cellules sont souvent ombrées différemment. L'AEM10941 est de toute façon limité à un Voc de 5,0 V, trop juste pour 2 SP3-12 en parallèle par temps froid.
- **Batterie** : petite LiPo souple 40–100 mAh au lieu de la LIR2016 (~20 mAh). Choix reporté.
- **Capteur** : **MS5611 retenu** (connu, même algorithme que la v1). Levier conso : OSR et fréquence d'échantillonnage. BMP581 écarté pour l'instant. Accéléromètre en option (fusion Kalman), coûteux en conso.
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
1. ~~**Prototype** : Feather nRF52840 + carte capteur, PlatformIO/Adafruit, liaison XCTrack~~ (fait) ; LK8000 et mesures au PPK2 à venir.
2. **Extension** :
   - logger IGC prototypé sur la Feather (flash QSPI intégrée + carte d'évaluation MAX-M10S) ;
   - schéma KiCad (`pcb/kicad/varioSmooth_v2`, base déjà dessinée) complété du GNSS et de la flash ;
   - boîtier pour le cockpit, PCB rigide.
3. **Vario casque** : petit boîtier sur une sangle, firmware commun.
