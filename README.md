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

## Frontend GitHub Pages

Frontend portal ada di folder `docs/` supaya bisa langsung dipublish ke GitHub Pages.

Alur deploy yang paling simpel:

1. Buat repository baru di GitHub, lalu push folder ini ke repo itu.
2. Di GitHub buka `Settings` > `Pages`.
3. Pada `Build and deployment`, pilih `Deploy from a branch`.
4. Pilih branch `main` dan folder `/docs`.
5. Tunggu sampai URL Pages muncul, lalu buka portal dari sana.

Portal frontend memakai `Supabase URL` dan `anon key` dari browser, jadi jangan pernah taruh `service_role key` di halaman publik. Supaya portal bisa membaca dan menulis data, nanti perlu policy Supabase yang sesuai untuk tabel `wol_bridges`, `wol_devices`, `wol_commands`, dan `wol_device_status`.

Kalau kamu mau, alur berikutnya biasanya:

1. Rapikan policy Supabase untuk portal GitHub Pages.
2. Sambungkan frontend ini ke data asli.
3. Baru kita lanjut bikin halaman utama portal publik dan dashboard status yang lebih halus.

## Draft Schema Supabase

```sql
create extension if not exists pgcrypto;

create table public.wol_bridges (
  id text primary key,
  name text not null default 'WOL Bridge',
  local_ip inet,
  ap_ip inet,
  wifi_connected boolean not null default false,
  last_seen_at timestamptz,
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now()
);

create table public.wol_devices (
  id uuid primary key default gen_random_uuid(),
  bridge_id text not null references public.wol_bridges(id) on delete cascade,
  name text not null,
  mac_address text not null,
  ip_address inet,
  broadcast_ip inet not null default '255.255.255.255',
  port integer not null default 9,
  enabled boolean not null default true,
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now()
);

create index wol_devices_bridge_enabled_idx on public.wol_devices (bridge_id, enabled);

create table public.wol_commands (
  id uuid primary key default gen_random_uuid(),
  bridge_id text not null references public.wol_bridges(id) on delete cascade,
  device_id uuid references public.wol_devices(id) on delete set null,
  mac_address text not null,
  broadcast_ip inet not null default '255.255.255.255',
  port integer not null default 9,
  status text not null default 'pending',
  message text,
  created_at timestamptz not null default now(),
  processed_at timestamptz
);

create table public.wol_device_status (
  device_id uuid primary key references public.wol_devices(id) on delete cascade,
  bridge_id text not null references public.wol_bridges(id) on delete cascade,
  online boolean not null default false,
  ip_address inet,
  latency_ms integer,
  checked_at timestamptz not null default now()
);
```

Untuk produksi, pakai RLS dan buat policy yang mengikuti cara frontend akan login nanti. Saat ini firmware M5 memakai REST langsung supaya backend dan portal bisa dibangun bertahap, jadi paling aman key M5 tetap diperlakukan sebagai secret device-side.

File SQL siap pakai ada di [supabase/schema.sql](C:/Users/even/Documents/WOLM5/supabase/schema.sql).
