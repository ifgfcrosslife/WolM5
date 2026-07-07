# WOL M5 Atom S3 Bridge

Project ini menjadikan M5 Atom S3 sebagai bridge Wake-on-LAN lokal. Frontend GitHub Pages dan Supabase akan menjadi portal dari luar jaringan, sedangkan M5 hanya perlu koneksi keluar ke Supabase dan akses broadcast/ping ke LAN.

## Alur Sistem

1. M5 membuat AP setup `WOLM5-xxxxxx`.
2. User buka web config M5 dari AP atau IP lokal.
3. User isi WiFi lokal, centang `Hidden SSID` kalau jaringan disembunyikan, lalu isi Supabase URL, Supabase API key, dan `bridge_id`.
4. M5 connect ke WiFi lokal.
5. M5 polling tabel `wol_commands` di Supabase.
6. Saat ada command `pending`, M5 mengirim magic packet ke PC lokal.
7. M5 update command menjadi `done` atau `failed`.
8. M5 polling daftar PC aktif dari `wol_devices`, ping berkala, lalu upsert status ke `wol_device_status`.

## Struktur Firmware

- `src/main.cpp`: lifecycle utama, setup WiFi/AP/web portal, polling command, status monitor.
- `src/ConfigStore.cpp`: simpan dan baca setting dari flash ESP32.
- `src/WifiManager.cpp`: AP setup dan koneksi station ke WiFi lokal.
- `src/WebPortal.cpp`: webapp konfigurasi lokal di port 80.
- `src/SupabaseClient.cpp`: akses REST API Supabase.
- `src/WolSender.cpp`: pembuatan dan pengiriman WOL magic packet.
- `src/StatusMonitor.cpp`: ping PC berkala dan update status ke Supabase.
- `include/*.h`: kontrak tiap modul agar fungsi mudah dibaca dan di-debug.

## Build

```powershell
pio run
pio run --target upload
pio device monitor
```

Board di `platformio.ini` memakai `m5stack-atoms3`. Jika PlatformIO lokal belum mengenali board ini, update platform Espressif 32 atau ganti sementara ke board ESP32-S3 yang cocok dengan Atom S3.

AtomS3 punya layar 0.85" IPS/LCD bawaan, jadi firmware sekarang juga menyalakan layar untuk status boot, koneksi WiFi, mode setup, dan bridge siap.

Kalau `deviceName` diisi `wolm5`, kamu bisa buka `http://wolm5.local` di perangkat yang satu jaringan. Di Windows, nama NetBIOS juga akan aktif dengan nama itu.

Setup pertama:

1. Flash firmware ke M5 Atom S3.
2. Connect ke WiFi AP `WOLM5-xxxxxx` dengan password default `wolm5setup`.
3. Buka `http://192.168.4.1`.
4. Isi WiFi lokal, Supabase URL, Supabase key, dan `bridge_id`.
5. Save, lalu M5 restart dan mulai polling Supabase.

## Draft Schema Supabase

```sql
create table public.wol_devices (
  id uuid primary key default gen_random_uuid(),
  bridge_id text not null,
  name text not null,
  mac_address text not null,
  ip_address text,
  broadcast_ip text default '255.255.255.255',
  wol_port integer default 9,
  enabled boolean default true,
  created_at timestamptz default now()
);

create table public.wol_commands (
  id uuid primary key default gen_random_uuid(),
  bridge_id text not null,
  device_id uuid references public.wol_devices(id) on delete set null,
  mac_address text not null,
  broadcast_ip text default '255.255.255.255',
  port integer default 9,
  status text not null default 'pending',
  message text,
  created_at timestamptz default now(),
  processed_at timestamptz
);

create table public.wol_device_status (
  device_id uuid primary key references public.wol_devices(id) on delete cascade,
  bridge_id text not null,
  online boolean not null default false,
  ip_address text,
  latency_ms integer,
  checked_at timestamptz default now()
);
```

Untuk produksi, pakai RLS: frontend hanya boleh insert command dan membaca device/status miliknya, sedangkan M5 bridge diberi key atau mekanisme auth khusus. Pada tahap awal firmware ini memakai Supabase REST langsung agar backend dan frontend bisa dibangun bertahap.
