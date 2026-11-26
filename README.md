# Bassurda

**Monofoni synth bass plugin za hip-hop, trap, drill i G-funk** (Windows VST3, JUCE, C++17).

Bassurda je napravljen da bas zvuči masivno na klupskom razglasu, a i dalje jasno na zvučniku mobitela:
čisti, fazno poravnati sub ispod, saturacija koja dodaje harmonike iznad, i precizna kontrola udarca i klizanja.

![Bassurda GUI](docs/screenshot.png)

## Značajke

| Sekcija | Što radi | Zašto je bitno za bas |
|---|---|---|
| **Oscilator A** | Morph val: sine → triangle → saw → square (PolyBLEP, bez aliasinga), pulse width, oct/semi/fine | Glavni karakter zvuka |
| **Oscilator B** | Isti morph + **unison do 7 glasova** s detuneom | Reese / "širi" basovi |
| **Sub oscilator** | Sinus ili trokut, oktava 0 / -1 / -2, **CLEAN** prekidač | Čista dubina. CLEAN sub zaobilazi filter i saturaciju, a kasni točno koliko i oversampling pa ostaje u fazi s ostatkom zvuka |
| **Click** | Kratki filtrirani noise burst (level + tone) | Udarac/čujnost na malim zvučnicima |
| **Pitch envelope** | -24 … +48 polutonova, decay 1 ms – 2 s, opcija i za sub | Karakteristični 808 "pad" visine na početku note |
| **Filter** | ZDF state-variable: **LP12, LP24**, BP12, HP12, cutoff, rezonancija, env amount (±), key tracking, drive | LP izlaz ima jedinično pojačanje na DC-u pa rezonancija ne "jede" bas |
| **Filter / Amp ADSR** | Linearni attack (od 0.1 ms), eksponencijalni decay/release kalibrirani da završe točno u zadanom vremenu | Brz, precizan punch; kratak release da se bas ne sudara s kickom |
| **Glide (portamento)** | Off / Legato / Always, Linear (konstantno vrijeme) ili Exp krivulja, 0 – 3 s | Trap/drill slideovi i G-funk portamento |
| **Mono engine** | Note stack (last-note priority, povratak na držanu notu), Retrig/Legato ovojnice, pitch bend do ±24 | Nema preklapanja niskih frekvencija |
| **Phase reset** | Svi oscilatori kreću od iste faze (podesiva početna faza 0–360°), uz de-click kod brzog ponavljanja | Svaki udarac je **identičan** (provjereno testom: razlika 0.0) |
| **Saturacija** | 4× oversampling, Warm (asimetrična, parni harmonici), Tape, Hard, Fold + Tone i Mix. **FULL / HIGHS**: u HIGHS načinu Linkwitz-Riley crossover (40–400 Hz) ostavlja niske frekvencije čistima, a distorzira samo gornji dio | Bas se čuje i na uređajima koji ne reproduciraju fundamental, a sub ostaje čist (test: THD -37 dB umjesto -8 dB) |
| **Sample oscilator** | Učitaj svoj 808 / bass one-shot (WAV, AIFF, FLAC, OGG, MP3) gumbom LOAD ili povlačenjem na ekran. **Root nota se sama prepozna**, uzorak prati glide, pitch bend i LFO, ima Start, Loop i opciju pitch envelopea | Pravi 808 uzorci sa svim slideovima ovog synth-a |
| **Kick Duck** | Bas se utiša kad svira kick: **NOTE** (odabrana MIDI nota, ne svira kao bas), **CH 10** (svaka nota na MIDI kanalu 10) ili **BEAT** (svaki takt hosta). Depth, Attack, Hold, Release; okidanje je usklađeno s latencijom | Kick i 808 se ne sudaraju, bez sidechain routinga u DAW-u |
| **MIDI learn** | Desni klik na bilo koji knob → MIDI Learn, pomakni kontroler. Mapiranje se sprema u projekt | Snimanje automatizacije hardverskim kontrolerom |
| **Vlastiti preseti** | Gumb PRESET → Save preset as / Delete / Open folder. Spremaju se u `Dokumenti/Bassurda/Presets/*.bassurda` (uz putanju učitanog uzorka) | Svoje zvukove imaš u svakom projektu |
| **LFO** | Sine/Tri/Saw/Square/S&H, slobodno ili sync na tempo (1/32 … 4/1, triole, točkaste), na cutoff i pitch | Wobble, vibrato |
| **Analog drift** | Spori nasumični pomak visine po oscilatoru | Manje "digitalan" zvuk |
| **Output** | Gain, Low Cut (20/30/40 Hz), DC blocker, soft clipper | Čist, siguran izlaz |
| **Prikazi** | LCD ekrani: valni oblici, ovojnice uživo, frekvencijski odziv filtera, krivulja saturacije, osciloskop s prikazom note i Hz, metar | Brza vizualna kontrola |

### 30 tvorničkih preseta

| Stil | Preseti |
|---|---|
| **Trap 808** | 808 Boom · Trap Slide 808 · Distorted 808 · Long 808 Tail · Short 808 Knock · Hard 808 Grit · Fuzzy Fold 808 · Rage 808 · Memphis Phonk 808 · Plugg Sine · Trap Triangle Sub |
| **Drill** | Drill Glide · UK Drill Slide · NY Drill Bounce · Dark Drill Reese |
| **Sub / mobitel** | Clean Sub · Phone Speaker Grit |
| **Boom bap / lo-fi** | Lo-Fi Boom Bap · Boom Bap Pluck |
| **West Coast** | G-Funk Moog · West Coast Glide |
| **Reese / grime** | Reese Pressure · Grime Square |
| **Ostalo** | Init Bass · Punch Square · Rubber Pluck · Acid Squelch · Wobble Lane · Synthwave Pulse · Deep House Pluck |

Factory preseti ne diraju Kick Duck postavke (one ovise o miksu) ni ugađanje učitanog uzorka;
sample sloj isključuju (uključiš ga gumbom ON).

## Tok signala

```
Osc A ──┐
Osc B ──┤
Sample ─┼─► Drive ─► Filter (LP12/LP24/BP/HP) ─► + Click ─► Amp ADSR ─► Saturacija (4× OS, FULL ili HIGHS) ─┐
Sub ────┘ (ako CLEAN nije uključen)                                                                       ├─► DC/Low Cut ─► Kick Duck ─► Gain ─► Soft Clip ─► Out
Sub ─────────────────────► Amp ADSR ─► kašnjenje = latencija oversamplinga (ako je CLEAN) ────────────────┘
        ▲ pitch: glide + pitch bend + pitch env + LFO + drift
```

Nazivi nota koriste konvenciju **C3 = srednji C (MIDI 60)**, kao Ableton i Logic.

## Skidanje

**[⬇ Bassurda-Windows-VST3.zip (najnovija verzija)](https://github.com/voyagersquare/Bassurda/releases/latest/download/Bassurda-Windows-VST3.zip)**

Raspakiraj zip i kopiraj mapu `Bassurda.vst3` u `C:\Program Files\Common Files\VST3`, pa u DAW-u ponovno skeniraj pluginove.

Nova verzija se objavljuje tagom: `git tag v0.2.0` pa `git push origin v0.2.0`. GitHub Actions tada builda plugin,
pokrene testove i objavi Release; gornji link uvijek vodi na najnoviju verziju.

## Build na svom računalu

Treba: Windows, [Visual Studio 2022](https://visualstudio.microsoft.com/) (radno opterećenje "Desktop development with C++")
i CMake ≥ 3.22. JUCE 7 se skida automatski.

```bash
cmake -B build
```

```bash
cmake --build build --config Release --target Bassurda_VST3
```

Rezultat: `build\Bassurda_artefacts\Release\VST3\Bassurda.vst3` → kopiraj u `C:\Program Files\Common Files\VST3`.

Lokalni JUCE checkout se može koristiti s `-DJUCE_DIR=C:/put/do/JUCE`.

## Testovi

`BassurdaTests` renderira note kroz cijeli procesor i provjerava ono što je bitno za bas:

- svi preseti renderiraju bez NaN-a i bez clippinga
- phase reset: uzastopni udarci su bit-identični
- glide: na pola vremena glida visina je točno na pola, i završava točno na ciljnoj noti
- clean sub ima harmoničku distorziju ispod -80 dB čak i uz punu saturaciju; kroz saturaciju dobiva harmonike
- pitch envelope stvarno spušta visinu
- latencija oversamplinga se javlja hostu
- kick duck: trigger nota utiša bas (−100 dB tijekom holda), sama ne svira, kanal 10 radi
- HIGHS saturacija ostavlja 55 Hz sub čistim
- sample oscilator: prepoznaje root notu i transponira uzorak točno na sviranu notu
- vlastiti preseti, MIDI learn i spremanje/učitavanje stanja projekta

```bash
./build/BassurdaTests_artefacts/Release/BassurdaTests --render demos --snapshot gui.png
```

`--render` sprema demo WAV za svaki preset (kratka trap linija sa slideovima), `--snapshot` sprema sliku GUI-ja.

## Struktura

```
Source/
  Parameters.*        svi parametri (ID-evi, rasponi, prikaz vrijednosti)
  Presets.*           tvornički preseti
  PluginProcessor.*   MIDI, oversampling saturacija, kompenzacija latencije, izlaz
  DSP/                BassEngine (glas), Envelope, Saturator, DspUtils (oscilatori, SVF)
  GUI/                LookAndFeel, LCD prikazi, moduli
  PluginEditor.*      raspored i skaliranje (prozor je resizable)
Tests/TestMain.cpp    offline testovi + render demo zvukova
```

## Licenca

GPLv3 (vidi `LICENSE`). JUCE 7 se koristi pod GPLv3.
