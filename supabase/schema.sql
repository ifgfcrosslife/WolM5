create extension if not exists pgcrypto;

create table if not exists public.wol_bridges (
  id text primary key,
  name text not null default 'WOL Bridge',
  local_ip inet,
  ap_ip inet,
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

grant select, insert, update, delete on public.wol_bridges to service_role;
grant select, insert, update, delete on public.wol_devices to service_role;
grant select, insert, update, delete on public.wol_commands to service_role;
grant select, insert, update, delete on public.wol_device_status to service_role;
