-- Portal bootstrap untuk Supabase.
-- Jalankan di SQL Editor setelah schema utama dibuat.
--
-- Isi bagian placeholder kalau kamu mau ganti nilai default.
-- Password portal di-hash ulang pakai bcrypt lewat pgcrypto.

insert into public.wol_portal_config (
  id,
  portal_password_hash,
  supabase_url,
  supabase_key,
  bridge_id,
  bridge_secret,
  bridge_name,
  default_broadcast,
  default_port
)
values (
  'default',
  extensions.crypt('1f6f', extensions.gen_salt('bf')),
  'https://kmhvcefbpkhsfahgrgpo.supabase.co',
  'PASTE_ANON_KEY_DI_SINI',
  'm5-atom-s3',
  'PASTE_BRIDGE_SECRET_DI_SINI',
  'Atom S3 Bridge',
  '',
  9
)
on conflict (id) do update set
  portal_password_hash = excluded.portal_password_hash,
  supabase_url = excluded.supabase_url,
  supabase_key = excluded.supabase_key,
  bridge_id = excluded.bridge_id,
  bridge_secret = excluded.bridge_secret,
  bridge_name = excluded.bridge_name,
  default_broadcast = excluded.default_broadcast,
  default_port = excluded.default_port,
  updated_at = now();

select public.portal_login('1f6f');

notify pgrst, 'reload schema';
