# VarioSmooth v2 — BLE

Notes de conception pour la modernisation du vario (2026-10).

## Objectifs
- MCU **nRF52840** avec BLE : paramétrage depuis le téléphone + envoi de la pression et du vario à **XCTrack** (puis LK8000) sur Android.
- **Vario sans GPS** : l'application utilise le GPS du téléphone.
- Schéma sous **KiCad**, carte redessinée pour l'intégration dans le casque.
- Firmware compilé/flashé avec **PlatformIO**.
- **Contrainte n°1 : la consommation.**

## Réorientation : deux appareils (2026-10-20, redéfinie le 2026-10-22)
La v1 marche, mais elle est pénible à installer : les fils passés par l'aération cassent, la carte est trop épaisse pour la doublure, et le flashage au PICkit est laborieux. Le projet est scindé en deux appareils :

| | **Vario casque** | **Extension** (cockpit) |
|---|---|---|
| Rôle | Bip + pression et vario (`$LK8EX1`) vers XCTrack : le baro du vario donne une finesse calculée correcte | **Batterie externe** du téléphone + vario (bip activable) + **logger GPS IGC** + BLE |
| Intégration | Dans le casque, cellules solaires sur le dessus, comme la v1 | Boîtier dans le cockpit ou la sellette |
| MCU | nRF52840 (MDBT50Q, ou ISP1807 pour gagner de la place) | nRF52840 |
| USB | **Aucun** : SWD une seule fois, puis OTA BLE | USB-C : recharge, flash, récupération du log |
| Énergie | LiPo + 2 × SP3-12 + LTC4071, comme la v1 | Li-ion **18650** + panneau solaire (prolonge un peu l'autonomie) |
| Interface | Un bouton | Boutons **+ / − / push**, petit écran, menu |

### Vario casque
- **Pas de connecteur USB** (il serait inaccessible) : J1, U1 (protection ESD), R2, R3, R4, D2 et C4 disparaissent. Carte routée avec le MDBT50Q : **20 × 25,25 mm** (~15 × 18 mm estimés avec un ISP1807).
- **Module : à trancher** entre le MDBT50Q et l'**Insight SiP ISP1807** (même nRF52840, donc même firmware) :

  | | MDBT50Q-1MV2 | ISP1807 |
  |---|---|---|
  | Taille | 10,5 × 15,5 × 2,05 mm + zone d'antenne | 8 × 8 × 1 mm, antenne intégrée |
  | Quartz | 32 MHz intégré, 32,768 kHz externe (Y1, C5, C6) | 32 MHz et 32,768 kHz a priori intégrés (à vérifier) |
  | Prix | ~10 € | ~15–20 € (stock à vérifier) |
  | Soudure | Pads accessibles, air chaud possible | LGA à pas fin : pochoir + four |
  | Bootloader | Déjà présent sur un module récupéré d'une Feather | Rien : sonde SWD obligatoire |
  | Carte Adafruit | Définition Feather nRF52840 utilisable telle quelle | Pas de variante : définition de carte à adapter |

  Dans les deux cas, l'antenne doit être au bord de la carte, avec une zone sans cuivre.
- **Flash** : bootloader Adafruit flashé une fois par SWD (sonde CMSIS-DAP), puis mises à jour en **OTA BLE** (nRF Connect ou Bluefruit). Le bouton reste sur **P1.02** : maintenu à la mise sous tension, il lance le bootloader en mode DFU, ce qui permet de récupérer un firmware planté sans ouvrir le casque.
- **Pastilles de test** au dos (Ø 1,5 mm, pas de 2,54 mm, pour pogo pins) : 3V3, GND, RESET, SWDIO, SWDCLK.
- **Reset sans bouton** : la batterie est soudée, on ne peut pas la débrancher pour relancer le nRF. Les pastilles RESET et GND sont voisines : les toucher ensemble avec une pince fait un reset. Un bouton, même petit, dépasserait et risquerait des resets intempestifs dans la doublure. Reset avec P1.02 maintenu (ou double reset) → DFU, pour reflasher une appli plantée ou une OTA ratée. Un bootloader corrompu ne se répare que par SWD.
- **Buzzer sur la carte** : à tester avant de l'adopter. Un piézo CMS (~9 × 9 mm, résonance ~4 kHz) est bien plus faible qu'un KPEG006 aux fréquences du vario (400–1500 Hz) ; il ne convient que si la carte est près de l'oreille. Essai : Feather + piézo CMS contre KPEG006, casque sur la tête, avec un ventilateur.
- **Fils vers les cellules** : point faible de la v1, prévoir un passage propre et un serre-câble collé sur la carte.
- La version avec USB-C (20 × 32,75 mm, commit `d064723`) reste disponible dans l'historique git comme **prototype de développement**.

### Extension : batterie externe, vario et logger
- **Batterie** : 18650 en **1S** (1 ou 2 cellules en parallèle, ~3000–3500 mAh chacune) : pas d'équilibrage, chargeurs et élévateurs courants. 2 cellules : ~37 × 65 × 19 mm, ~95 g. Cellules de marque (Samsung 35E, Molicel, LG).
- **Alimentation** : chargeur type **TI BQ25895** (le LTC4071 ne convient plus) :
  - power path (l'appareil fonctionne pendant la charge) ;
  - sortie 5 V élevée (OTG, ~2–3 A) pour le téléphone ;
  - NTC avec seuils JEITA : pas de charge sous 0 °C ;
  - régulation de la tension d'entrée (VINDPM), qui fait office de MPPT simplifié pour un panneau de 6 V nominal ;
  - suivi par I²C depuis le nRF (niveau, courants, état).
  - Alternative écartée : IP5328P (charge rapide PD/QC intégrée, mais peu documenté et peu pilotable).
- **Connectique** : USB-C en entrée ; sortie USB-C ou USB-A en 5 V seul (le téléphone charge à ~1,5–2 A, suffisant pour XCTrack en vol). La charge rapide PD demanderait un contrôleur USB-PD en plus.
- **Sécurité** : protection de cellule (cellules protégées ou DW01 + MOSFET), fusible, thermistance sur les cellules, boîtier résistant aux chocs.
- **Solaire** : ~1 W en plein soleil → ~150–200 mA dans la batterie, contre 300–600 mA pour un téléphone avec XCTrack. Le panneau alimente l'extension elle-même et ralentit la décharge du téléphone.
- **Écran** : un **Sharp Memory LCD** (ex. LS013B7DH03, 1,28″) plutôt qu'un OLED. Il reste lisible en plein soleil et consomme quelques µA, contre 10–20 mA pour un OLED peu lisible au soleil.
- **Baro** : avec les deux appareils, n'envoyer la pression à XCTrack que depuis l'un des deux (baro désactivable sur l'extension, comme le bip).

### Extension : comportement visé
- Téléphone connecté : trames `$LK8EX1`, bip coupé (XCTrack bipe), sauf si le bip est activé dans le menu.
- Connexion perdue depuis un certain délai : bip automatique.
- Log IGC permanent à 1 Hz (~125 Ko/h, ~130 h sur 16 Mo), récupérable en **USB Mass Storage** (Adafruit_SPIFlash + FatFs + TinyUSB) : le boîtier apparaît comme une clé USB.
- Flash QSPI sur les **mêmes broches que la Feather Express** (P0.17, P0.19–P0.23) : le firmware la gère sans changement, et le logger se prototype sur la Feather (GD25Q16 2 Mo intégrée) avec une carte d'évaluation MAX-M10S.
- Le GNSS consomme 5–30 mW selon la puce (MAX-M10S < 25 mW en suivi), négligeable devant le téléphone. Garder une alimentation de sauvegarde (V_BCKP) pour démarrer à chaud.

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

Sortie de debug choisie à la compilation par `DEBUG_OUTPUT` (`src/debug_out.h`) : 0 = aucune (mesures de consommation, vol), 1 = USB, 2 = **SEGGER RTT** par le SWD (carte casque sans USB). L'environnement `rtt` compile avec RTT ; lecture avec une sonde CMSIS-DAP : `probe-rs attach --chip nRF52840_xxAA .pio/build/rtt/firmware.elf`. Le tampon RTT ne bloque pas : sans sonde, les messages en trop sont perdus et le firmware continue. Une sonde branchée consomme ~1 mA.

| Fichier | Rôle |
|---|---|
| `src/ms5611.*` | Pilote du capteur : PROM + CRC4, compensation de température du 2e ordre (absentes en v1). Pression en OSR 4096, température en OSR 1024 (512 en v1). |
| `src/vario.*` | Algorithme de la v1 : filtre passe-bas d'ordre 2 sur la pression, dérivée sur une fenêtre de 10 à 50 échantillons selon la vitesse, filtre IIR sur la vitesse. Calcul en flottant et sur l'altitude (cm/s) plutôt que sur la pression. |
| `src/ble_link.*` | Nordic UART + trames `$LK8EX1`. |
| `src/debug_out.*` | Sortie de debug `dbg` (USB ou RTT), voir `DEBUG_OUTPUT`. |
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

## Bloc d'alimentation du vario casque (config v1, retenue le 2026-10-21)

```
SP3-12 ─ SP3-12 ── D1 (Schottky) ── RIN (200 Ω) ──┬── VCC ── VDDH (MDBT50Q) ── REG0 DC/DC ── VDD 3,0 V ── MS5611, buzzer
   (série, côte à côte sur le dessus)               │
                                      LTC4071 (shunt)── BAT ── LiPo (hors carte, pastilles BT1)
```

**Retenu le 2026-10-21 : la configuration de la v1**, qui a toujours fonctionné. Elle remplace les réglages étudiés le 2026-10-17 (RIN 0 Ω, LBSEL à VCC, NTC sur la batterie), conservés ci-dessous pour mémoire :
- **RIN = 200 Ω** ;
- **ADJ en l'air** (fin de charge 4,1 V) ;
- **NTC relié à VCC** : pas de réduction de la tension de fin de charge à chaud ;
- **LBSEL à GND** : coupure à 3,2 V, meilleure conservation pendant les longs mois sans vol.

**Froid** : d'après la datasheet, le NTC du LTC4071 ne fait que baisser la tension de fin de charge au-dessus de 40 °C. Il n'y a **aucune coupure de charge par temps froid**. Le courant de charge est minuscule, mais charger une LiPo sous 0 °C reste un risque. Parade possible : un P-MOSFET entre les cellules et le LTC4071, coupé par le nRF sous 0 °C (température du MS5611).

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
  - VBUS (32) relié à GND, D+/D− (35/34) non connectés : pas d'USB (flash par SWD puis OTA BLE).
  - P0.18 en broche de reset (pastille RESET), configuré par le bootloader Adafruit.
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
- **Batterie** (vario casque) : petite LiPo souple 40–100 mAh au lieu de la LIR2016 (~20 mAh), hors carte, reliée aux pastilles BT1. Modèle à choisir.
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
2. **Vario casque** :
   - carte compacte `pcb/kicad/varioSmooth_v2` routée (sans USB, 20 × 25,25 mm), à fabriquer et valider : flash SWD, puis OTA BLE ;
   - test du buzzer CMS.
3. **Extension** :
   - logger IGC prototypé sur la Feather (flash QSPI intégrée + carte d'évaluation MAX-M10S) ;
   - nouveau schéma : 18650 + BQ25895, GNSS, flash, écran, 3 boutons ;
   - boîtier pour le cockpit, PCB rigide.
