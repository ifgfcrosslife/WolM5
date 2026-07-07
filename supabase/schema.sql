create extension if not exists pgcrypto;

create table if not exists public.wol_bridges (
  id text primary key,
  name text not null default 'WOL Bridge',
  local_ip inet,
  ap_ip inet,
  bridge_secret text not null default gen_random_uuid()::text,
  wifi_connected boolean not null default false,
  last_seen_at timestamptz,
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now()
);

create table if not exists public.wol_devices (
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

create index if not exists wol_devices_bridge_enabled_idx
  on public.wol_devices (bridge_id, enabled);

create table if not exists public.wol_commands (
  id uuid primary key default gen_random_uuid(),
  bridge_id text not null references public.wol_bridges(id) on delete cascade,
  device_id uuid references public.wol_devices(id) on delete set null,
  mac_address text not null,
  broadcast_ip inet not null default '255.255.255.255',
  port integer not null default 9,
  status text not null default 'pending'
    check (status in ('pending', 'processing', 'done', 'failed', 'cancelled')),
  message text,
  created_at timestamptz not null default now(),
  processed_at timestamptz
);

create index if not exists wol_commands_bridge_status_idx
  on public.wol_commands (bridge_id, status, created_at);

create table if not exists public.wol_device_status (
  device_id uuid primary key references public.wol_devices(id) on delete cascade,
  bridge_id text not null references public.wol_bridges(id) on delete cascade,
  online boolean not null default false,
  ip_address inet,
  latency_ms integer,
  checked_at timestamptz not null default now()
);

create index if not exists wol_device_status_bridge_checked_idx
  on public.wol_device_status (bridge_id, checked_at desc);

create or replace function public.set_updated_at()
returns trigger
language plpgsql
as $$
begin
  new.updated_at = now();
  return new;
end;
$$;

drop trigger if exists wol_bridges_set_updated_at on public.wol_bridges;
create trigger wol_bridges_set_updated_at
before update on public.wol_bridges
for each row
execute function public.set_updated_at();

drop trigger if exists wol_devices_set_updated_at on public.wol_devices;
create trigger wol_devices_set_updated_at
before update on public.wol_devices
for each row
execute function public.set_updated_at();

alter table public.wol_bridges enable row level security;
alter table public.wol_devices enable row level security;
alter table public.wol_commands enable row level security;
alter table public.wol_device_status enable row level security;

alter table if exists public.wol_bridges add column if not exists bridge_secret text;
update public.wol_bridges
set bridge_secret = coalesce(bridge_secret, gen_random_uuid()::text)
where bridge_secret is null;
alter table public.wol_bridges alter column bridge_secret set default gen_random_uuid()::text;
alter table public.wol_bridges alter column bridge_secret set not null;
create unique index if not exists wol_bridges_bridge_secret_idx on public.wol_bridges (bridge_secret);

create or replace function public.request_headers_json()
returns jsonb
language sql
stable
as $$
  select coalesce(nullif(current_setting('request.headers', true), '')::jsonb, '{}'::jsonb);
$$;

create or replace function public.request_bridge_secret()
returns text
language sql
stable
as $$
  select coalesce(public.request_headers_json() ->> 'x-bridge-secret', '');
$$;

drop policy if exists "bridge owners read bridges" on public.wol_bridges;
drop policy if exists "bridge owners write bridges" on public.wol_bridges;
drop policy if exists "bridge owners read devices" on public.wol_devices;
drop policy if exists "bridge owners write devices" on public.wol_devices;
drop policy if exists "bridge owners read commands" on public.wol_commands;
drop policy if exists "bridge owners write commands" on public.wol_commands;
drop policy if exists "bridge owners read statuses" on public.wol_device_status;
drop policy if exists "bridge owners write statuses" on public.wol_device_status;

create policy "bridge owners read bridges"
on public.wol_bridges
for select
to anon, authenticated, service_role
using (bridge_secret = public.request_bridge_secret());

create policy "bridge owners write bridges"
on public.wol_bridges
for all
to anon, authenticated, service_role
using (bridge_secret = public.request_bridge_secret())
with check (bridge_secret = public.request_bridge_secret());

create policy "bridge owners read devices"
on public.wol_devices
for select
to anon, authenticated, service_role
using (
  exists (
    select 1
    from public.wol_bridges b
    where b.id = bridge_id
      and b.bridge_secret = public.request_bridge_secret()
  )
);

create policy "bridge owners write devices"
on public.wol_devices
for all
to anon, authenticated, service_role
using (
  exists (
    select 1
    from public.wol_bridges b
    where b.id = bridge_id
      and b.bridge_secret = public.request_bridge_secret()
  )
)
with check (
  exists (
    select 1
    from public.wol_bridges b
    where b.id = bridge_id
      and b.bridge_secret = public.request_bridge_secret()
  )
);

create policy "bridge owners read commands"
on public.wol_commands
for select
to anon, authenticated, service_role
using (
  exists (
    select 1
    from public.wol_bridges b
    where b.id = bridge_id
      and b.bridge_secret = public.request_bridge_secret()
  )
);

create policy "bridge owners write commands"
on public.wol_commands
for all
to anon, authenticated, service_role
using (
  exists (
    select 1
    from public.wol_bridges b
    where b.id = bridge_id
      and b.bridge_secret = public.request_bridge_secret()
  )
)
with check (
  exists (
    select 1
    from public.wol_bridges b
    where b.id = bridge_id
      and b.bridge_secret = public.request_bridge_secret()
  )
);

create policy "bridge owners read statuses"
on public.wol_device_status
for select
to anon, authenticated, service_role
using (
  exists (
    select 1
    from public.wol_bridges b
    where b.id = bridge_id
      and b.bridge_secret = public.request_bridge_secret()
  )
);

create policy "bridge owners write statuses"
on public.wol_device_status
for all
to anon, authenticated, service_role
using (
  exists (
    select 1
    from public.wol_bridges b
    where b.id = bridge_id
      and b.bridge_secret = public.request_bridge_secret()
  )
)
with check (
  exists (
    select 1
    from public.wol_bridges b
    where b.id = bridge_id
      and b.bridge_secret = public.request_bridge_secret()
  )
);

grant select, insert, update, delete on public.wol_bridges to anon, authenticated, service_role;
grant select, insert, update, delete on public.wol_devices to anon, authenticated, service_role;
grant select, insert, update, delete on public.wol_commands to anon, authenticated, service_role;
grant select, insert, update, delete on public.wol_device_status to anon, authenticated, service_role;

create table if not exists public.wol_portal_config (
  id text primary key default 'default',
  portal_password_hash text not null,
  supabase_url text not null,
  supabase_key text not null,
  bridge_id text not null default 'm5-atom-s3',
  bridge_secret text not null,
  bridge_name text not null default 'Atom S3 Bridge',
  default_broadcast text,
  default_port integer not null default 9,
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now()
);

alter table if exists public.wol_portal_config enable row level security;

drop policy if exists "portal config deny select" on public.wol_portal_config;
drop function if exists public.portal_login(text);
drop function if exists public.save_portal_config(text, text, text, text, text, text, integer);

create function public.portal_login(password text)
returns jsonb
language plpgsql
security definer
set search_path = public, extensions
as $$
declare
  cfg public.wol_portal_config%rowtype;
begin
  select *
  into cfg
  from public.wol_portal_config
  where id = 'default'
  limit 1;

  if not found then
    raise exception 'Portal config default belum diisi' using errcode = 'P0002';
  end if;

  if cfg.portal_password_hash is null
     or cfg.portal_password_hash <> extensions.crypt(password, cfg.portal_password_hash) then
    raise exception 'Invalid portal password' using errcode = '28P01';
  end if;

  return jsonb_build_object(
    'supabaseUrl', cfg.supabase_url,
    'supabaseKey', cfg.supabase_key,
    'bridgeId', cfg.bridge_id,
    'bridgeSecret', cfg.bridge_secret,
    'bridgeName', cfg.bridge_name,
    'defaultBroadcast', coalesce(cfg.default_broadcast, ''),
    'defaultPort', cfg.default_port
  );
end;
$$;

revoke all on function public.portal_login(text) from public;
grant execute on function public.portal_login(text) to anon, authenticated, service_role;

create function public.save_portal_config(
  p_supabase_url text,
  p_supabase_key text,
  p_bridge_id text,
  p_bridge_secret text,
  p_bridge_name text,
  p_default_broadcast text,
  p_default_port integer
)
returns jsonb
language plpgsql
security definer
set search_path = public, extensions
as $$
declare
  cfg public.wol_portal_config%rowtype;
  caller_secret text := public.request_bridge_secret();
begin
  if caller_secret = '' then
    raise exception 'Missing bridge secret' using errcode = '28000';
  end if;

  update public.wol_portal_config
  set
    supabase_url = p_supabase_url,
    supabase_key = p_supabase_key,
    bridge_id = p_bridge_id,
    bridge_secret = p_bridge_secret,
    bridge_name = p_bridge_name,
    default_broadcast = nullif(p_default_broadcast, ''),
    default_port = coalesce(p_default_port, 9),
    updated_at = now()
  where id = 'default'
    and bridge_secret = caller_secret
  returning * into cfg;

  if not found then
    raise exception 'Portal config not found or bridge secret mismatch' using errcode = '28P01';
  end if;

  return jsonb_build_object(
    'supabaseUrl', cfg.supabase_url,
    'supabaseKey', cfg.supabase_key,
    'bridgeId', cfg.bridge_id,
    'bridgeSecret', cfg.bridge_secret,
    'bridgeName', cfg.bridge_name,
    'defaultBroadcast', coalesce(cfg.default_broadcast, ''),
    'defaultPort', cfg.default_port
  );
end;
$$;

revoke all on function public.save_portal_config(text, text, text, text, text, text, integer) from public;
grant execute on function public.save_portal_config(text, text, text, text, text, text, integer) to anon, authenticated, service_role;
