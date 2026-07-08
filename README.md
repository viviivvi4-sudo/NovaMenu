# Floating Menu Mod (Geode / GD 2.2081)

Mod Geode dengan tombol mengambang (bisa di-drag) yang membuka menu popup
berisi 4 tab: **Level**, **Player**, **Cosmetic**, dan **Bot Replay**.
Target: Android32 + Android64 saja.

## Struktur

```
mod-geode/
├── mod.json                 # metadata mod (target GD 2.2081, android32/64)
├── CMakeLists.txt
├── src/
│   ├── main.cpp             # entry point, pasang floating button di semua scene
│   ├── FloatingButton.hpp/.cpp   # tombol draggable
│   ├── ModMenuPopup.hpp/.cpp     # popup 4 tab
│   ├── BotReplay.hpp/.cpp        # inti sistem record/playback macro
│   └── ReplayHooks.cpp           # hook PlayLayer & PlayerObject
└── .github/workflows/build.yml   # CI build android32 + android64
```

## Cara build lokal

1. Install [geode-cli](https://docs.geode-sdk.org/getting-started/) dan pastikan versi SDK yang terpasang **v5.7.1**:
   ```
   geode sdk install v5.7.1
   ```
2. Dari folder project:
   ```
   geode build --platform android32
   geode build --platform android64
   ```
3. File `.geode` hasil build ada di folder `build-*/`.

## Cara build via GitHub Actions

Push repo ini ke GitHub, workflow `.github/workflows/build.yml` otomatis:
- Build target `android32` dan `android64` secara terpisah,
- Menggabungkannya jadi satu file `.geode` universal (job `combine`),
- Artifact bisa didownload dari tab **Actions** tiap run.

Kalau nama action `geode-sdk/geode-build-action` atau `geode-sdk/combine-action`
sudah berubah versi/tag saat kamu pakai, cek referensi terbaru di
https://docs.geode-sdk.org/ dan sesuaikan `uses:` di workflow.

## Cara pakai bot replay in-game

1. Buka level apa pun.
2. Tap tombol mengambang → tab **Bot Replay**.
3. **Record** → mainkan level secara manual (semua input jump/left/right
   direkam per-frame).
4. **Stop** setelah selesai.
5. **Play** → level akan restart otomatis dan input yang direkam diputar
   ulang frame-per-frame (auto-restart tiap kali mati, sampai macro habis).
6. **Reset** untuk menghapus rekaman.

Data rekaman disimpan di memori selama sesi berjalan (belum otomatis
tersimpan ke disk). Fungsi `BotReplay::saveToFile` / `loadFromFile` sudah
disediakan di `BotReplay.hpp` kalau kamu mau menambahkan tombol
"Save"/"Load" ke file `.replay` di tab UI.

## Catatan penting

- Kode ini kerangka kerja fungsional (bukan tempelan), tapi struct/hook GD
  (`PlayerObject`, `PlayLayer`) bisa saja beda member/nama di update GD
  berikutnya — kalau ada error compile setelah update game, cek
  `Geode/binding` terbaru dan sesuaikan nama member.
- Tab **Level**, **Player**, **Cosmetic** baru berisi UI placeholder;
  silakan isi dengan fitur yang kamu mau (noclip, speed, warna trail, dst),
  tinggal tambah tombol di `ModMenuPopup.cpp` seperti pola di tab Bot Replay.
- Fitur macro/replay seperti ini lazim dipakai untuk level yang memang
  didesain untuk bot ("bot level"). Kalau dipakai untuk memalsukan
  completion di level normal / leaderboard, itu melanggar aturan komunitas
  GD — pastikan kamu pakai sesuai konteks yang fair.
