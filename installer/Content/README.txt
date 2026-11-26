Sadržaj koji installer kopira kod korisnika
==========================================

Presets\  -> Dokumenti\Bassurda\Presets   (*.bassurda)
Samples\  -> Dokumenti\Bassurda\Samples   (*.wav, *.aif, *.aiff, *.flac, *.ogg, *.mp3)

Kako dodati svoje presete u installer:
1. U pluginu spremi preset (PRESET -> Save preset as...).
2. Kopiraj ga iz Dokumenti\Bassurda\Presets u installer\Content\Presets.
3. Ako koristi sample, kopiraj i taj sample iz Dokumenti\Bassurda\Samples u installer\Content\Samples.
4. Commitaj i pushaj: GitHub napravi novi installer s tim sadržajem.

Postojeće datoteke kod korisnika se nikad ne prepisuju, a deinstalacija ih ne briše.
Preset pronalazi svoj sample po imenu datoteke, pa radi i na drugom računalu.
