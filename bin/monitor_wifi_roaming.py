#!/usr/bin/env python3
"""
Monitor Wi‑Fi roaming downtime on Linux.

Definition of downtime (default):
- Not associated to any AP (iw reports "Not connected") OR
- No IPv4 address assigned on the Wi‑Fi interface (common indicator of no usable IP)

The script periodically samples link state and IP presence, records downtime
intervals, and prints a summary on exit (Ctrl+C). Optionally logs events to a file.

Requires: iw, ip
"""

from __future__ import annotations

import argparse
import dataclasses
import datetime as dt
import os
import signal
import subprocess
import sys
import time
from typing import Optional, Tuple, List
import csv


def run_cmd(cmd: List[str], timeout: float = 3.0) -> Tuple[int, str, str]:
    try:
        p = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=timeout,
            check=False,
        )
        return p.returncode, p.stdout.strip(), p.stderr.strip()
    except subprocess.TimeoutExpired as e:
        return 124, (e.stdout or "").strip(), (e.stderr or "timeout").strip()
    except FileNotFoundError:
        return 127, "", f"command not found: {cmd[0]}"


def detect_wifi_interface(preferred: Optional[str] = None) -> Optional[str]:
    # If user provided interface, trust it exists (we validate later)
    if preferred:
        return preferred

    code, out, err = run_cmd(["iw", "dev"])
    if code != 0:
        return None

    iface = None
    lines = out.splitlines()
    current = None
    managed_ifaces: List[str] = []
    for line in lines:
        line = line.strip()
        if line.startswith("Interface "):
            current = line.split()[1]
        elif line.startswith("type ") and current:
            typ = line.split()[1]
            if typ == "managed":
                managed_ifaces.append(current)
            current = None

    # Prefer a currently connected iface if any
    for cand in managed_ifaces:
        if get_link_state(cand).connected:
            iface = cand
            break
    if not iface and managed_ifaces:
        iface = managed_ifaces[0]
    return iface


@dataclasses.dataclass
class LinkState:
    connected: bool
    ssid: Optional[str] = None
    bssid: Optional[str] = None


def get_link_state(iface: str) -> LinkState:
    code, out, err = run_cmd(["iw", "dev", iface, "link"])
    if code != 0:
        return LinkState(False)
    if "Not connected" in out:
        return LinkState(False)
    # Parse Connected to <BSSID> and SSID: <name>
    bssid = None
    ssid = None
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("Connected to "):
            parts = line.split()
            if len(parts) >= 3:
                bssid = parts[2]
        elif line.startswith("SSID: "):
            ssid = line.split(": ", 1)[1]
    return LinkState(True, ssid=ssid, bssid=bssid)


def has_ipv4_address(iface: str) -> bool:
    code, out, err = run_cmd(["ip", "-4", "addr", "show", "dev", iface])
    if code != 0:
        return False
    for line in out.splitlines():
        line = line.strip()
        # Typical: "inet 192.168.1.10/24 brd 192.168.1.255 scope global dynamic ..."
        if line.startswith("inet "):
            return True
    return False


def has_global_ipv6_address(iface: str) -> bool:
    code, out, err = run_cmd(["ip", "-6", "addr", "show", "scope", "global", "dev", iface])
    if code != 0:
        return False
    for line in out.splitlines():
        if line.strip().startswith("inet6 "):
            return True
    return False


@dataclasses.dataclass
class Stats:
    start_mono: float
    last_mono: float
    total_downtime: float = 0.0
    events: int = 0
    max_event: float = 0.0


@dataclasses.dataclass
class DowntimeEvent:
    start_wall: dt.datetime
    start_mono: float
    end_wall: Optional[dt.datetime] = None
    end_mono: Optional[float] = None
    last_known_ssid: Optional[str] = None
    last_known_bssid: Optional[str] = None
    started_not_associated: bool = False
    started_no_ip: bool = False

    def duration(self, now_mono: Optional[float] = None) -> float:
        if self.end_mono is not None:
            return max(0.0, self.end_mono - self.start_mono)
        if now_mono is None:
            return 0.0
        return max(0.0, now_mono - self.start_mono)


class Logger:
    def __init__(self, path: Optional[str]):
        self.path = path
        self._fp = None
        if self.path:
            os.makedirs(os.path.dirname(os.path.abspath(self.path)), exist_ok=True)
            self._fp = open(self.path, "a", buffering=1)

    def write(self, msg: str) -> None:
        ts = dt.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        line = f"[{ts}] {msg}\n"
        sys.stdout.write(line)
        sys.stdout.flush()
        if self._fp:
            self._fp.write(line)
            self._fp.flush()

    def close(self):
        if self._fp:
            self._fp.close()
            self._fp = None


class CsvLogger:
    def __init__(self, path: Optional[str]):
        self.path = path
        self._fp = None
        self._writer = None
        if self.path:
            os.makedirs(os.path.dirname(os.path.abspath(self.path)), exist_ok=True)
            new_file = not os.path.exists(self.path) or os.path.getsize(self.path) == 0
            self._fp = open(self.path, "a", newline="")
            self._writer = csv.writer(self._fp)
            if new_file:
                self._writer.writerow([
                    "type",               # event | summary
                    "event_index",        # int (for type=event)
                    "start_time_iso",
                    "end_time_iso",
                    "duration_s",         # float with 0.1s resolution
                    "ssid",
                    "bssid",
                    "started_not_associated",
                    "started_no_ip",
                    # Summary-only fields below
                    "runtime_s",
                    "total_downtime_s",
                    "downtime_pct",
                    "events",
                    "avg_downtime_s",
                    "max_downtime_s",
                ])

    def write_event(self, idx: int, ev: DowntimeEvent, duration_s: float):
        if not self._writer:
            return
        start_iso = ev.start_wall.isoformat(timespec="seconds") if ev.start_wall else ""
        end_iso = ev.end_wall.isoformat(timespec="seconds") if ev.end_wall else ""
        self._writer.writerow([
            "event",
            idx,
            start_iso,
            end_iso,
            round(duration_s, 1),
            ev.last_known_ssid or "",
            ev.last_known_bssid or "",
            int(ev.started_not_associated),
            int(ev.started_no_ip),
            "", "", "", "", "", "",
        ])
        self._fp.flush()

    def write_summary(
        self,
        runtime_s: float,
        total_down_s: float,
        pct: float,
        events: int,
        avg_s: float,
        max_s: float,
    ):
        if not self._writer:
            return
        self._writer.writerow([
            "summary",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            round(runtime_s, 1),
            round(total_down_s, 1),
            f"{pct:.2f}",
            events,
            round(avg_s, 1),
            round(max_s, 1),
        ])
        self._fp.flush()

    def close(self):
        if self._fp:
            self._fp.close()
            self._fp = None


def format_duration(seconds: float) -> str:
    # Human readable with 0.1s precision for seconds
    if seconds < 60:
        return f"{round(seconds, 1)}s"
    h = int(seconds // 3600)
    m = int((seconds % 3600) // 60)
    s = seconds % 60
    if h:
        return f"{h}h {m}m {int(round(s))}s"
    return f"{m}m {int(round(s))}s"


def main():
    parser = argparse.ArgumentParser(description="Monitor Wi‑Fi roaming downtime (association/IP loss)")
    parser.add_argument("--iface", help="Wi‑Fi interface to monitor (auto-detect if omitted)")
    parser.add_argument("--interval", type=float, default=0.5, help="Sampling interval in seconds (default: 0.5)")
    parser.add_argument("--log", help="Optional log file to append events and summary")
    parser.add_argument("--csv", help="Optional CSV file to append per-event and summary data")
    # By default, IPv4 presence is required for uptime.
    parser.add_argument(
        "--allow-ipv6-global",
        action="store_true",
        help="Also accept presence of a global IPv6 address as having IP",
    )

    args = parser.parse_args()

    iface = detect_wifi_interface(args.iface)
    if not iface:
        sys.stderr.write("Could not detect a Wi‑Fi interface. Provide --iface.\n")
        sys.exit(2)

    logger = Logger(args.log)
    csv_logger = CsvLogger(args.csv)
    logger.write(f"Monitoring interface: {iface}; interval={args.interval}s")

    stats = Stats(start_mono=time.monotonic(), last_mono=time.monotonic())
    current_event: Optional[DowntimeEvent] = None

    running = True

    def handle_sigint(signum, frame):
        nonlocal running
        running = False

    signal.signal(signal.SIGINT, handle_sigint)
    signal.signal(signal.SIGTERM, handle_sigint)

    last_connected_ssid = None
    last_connected_bssid = None

    try:
        while running:
            t0 = time.monotonic()
            link = get_link_state(iface)

            # If iface disappeared, re-detect once in a while
            if link.connected is False and "No such device" in run_cmd(["iw", "dev", iface, "info"])[2]:
                new_iface = detect_wifi_interface(None)
                if new_iface and new_iface != iface:
                    logger.write(f"Interface {iface} disappeared; switching to {new_iface}")
                    iface = new_iface
                    link = get_link_state(iface)

            has_ip = has_ipv4_address(iface)
            if not has_ip and args.allow_ipv6_global:
                has_ip = has_global_ipv6_address(iface)

            uptime_condition = link.connected and has_ip

            # Log connect transitions (optional, informational)
            if link.connected and (link.ssid != last_connected_ssid or link.bssid != last_connected_bssid):
                if link.ssid or link.bssid:
                    logger.write(
                        f"Connected to SSID='{link.ssid or '?'}' BSSID={link.bssid or '?'}"
                    )
                last_connected_ssid, last_connected_bssid = link.ssid, link.bssid

            if uptime_condition:
                # We are UP now
                if current_event is not None:
                    current_event.end_mono = t0
                    current_event.end_wall = dt.datetime.now()
                    dur = current_event.duration()
                    stats.total_downtime += dur
                    stats.events += 1
                    stats.max_event = max(stats.max_event, dur)
                    logger.write(
                        f"Downtime ended after {format_duration(dur)} (from {current_event.start_wall.strftime('%H:%M:%S')} to {current_event.end_wall.strftime('%H:%M:%S')})"
                    )
                    csv_logger.write_event(stats.events, current_event, dur)
                    current_event = None
            else:
                # We are DOWN now
                if current_event is None:
                    current_event = DowntimeEvent(
                        start_wall=dt.datetime.now(),
                        start_mono=t0,
                        last_known_ssid=link.ssid,
                        last_known_bssid=link.bssid,
                        started_not_associated=(not link.connected),
                        started_no_ip=(not has_ip),
                    )
                    logger.write("Downtime started (not associated and/or no IP)")

            # Pace the loop to desired interval
            elapsed = time.monotonic() - t0
            to_sleep = max(0.0, args.interval - elapsed)
            time.sleep(to_sleep)

            stats.last_mono = time.monotonic()

    finally:
        # On exit, if in the middle of a downtime, account it up to now
        if current_event is not None:
            dur = current_event.duration(now_mono=stats.last_mono)
            stats.total_downtime += dur
            stats.events += 1
            stats.max_event = max(stats.max_event, dur)
            logger.write(
                f"Downtime (open) accounted as {format_duration(dur)} up to exit"
            )
            # Close the event for CSV with an end time now
            current_event.end_mono = stats.last_mono
            current_event.end_wall = dt.datetime.now()
            csv_logger.write_event(stats.events, current_event, dur)

        total_runtime = stats.last_mono - stats.start_mono
        pct = (stats.total_downtime / total_runtime * 100.0) if total_runtime > 0 else 0.0
        avg = (stats.total_downtime / stats.events) if stats.events else 0.0

        logger.write("----- SUMMARY -----")
        logger.write(f"Runtime: {format_duration(total_runtime)}")
        logger.write(
            f"Total downtime: {format_duration(stats.total_downtime)} ({pct:.2f}%) in {stats.events} event(s)"
        )
        if stats.events:
            logger.write(
                f"Avg downtime: {format_duration(avg)} | Max downtime: {format_duration(stats.max_event)}"
            )
        logger.write("-------------------")
        logger.close()
        csv_logger.write_summary(total_runtime, stats.total_downtime, pct, stats.events, avg, stats.max_event)
        csv_logger.close()


if __name__ == "__main__":
    main()
