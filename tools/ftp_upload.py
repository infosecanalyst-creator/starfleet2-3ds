#!/usr/bin/env python3
"""Upload a local SD-card tree to the 3DS's ftpd and verify each file's size.

usage: ftp_upload.py IP PORT LOCAL_SD_ROOT   (LOCAL_SD_ROOT/3ds/... -> sdmc:/3ds/...)
Skips files whose size on the 3DS already matches unless --all is given.
"""
import ftplib
import os
import sys
import time

ip, port, root = sys.argv[1], int(sys.argv[2]), sys.argv[3]
upload_all = '--all' in sys.argv
ftp = None


def connect():
    """(Re)connect: ftpd on the 3DS sometimes drops or stalls a session, so every failure
    starts a fresh one."""
    global ftp
    if ftp is not None:
        try:
            ftp.close()
        except Exception:
            pass
    for attempt in range(8):
        try:
            ftp = ftplib.FTP()
            ftp.connect(ip, port, timeout=60)
            ftp.login()
            ftp.voidcmd('TYPE I')
            return
        except (OSError, ftplib.Error) as e:
            print(f'connect failed ({e}); retrying...')
            time.sleep(4)
    sys.exit('could not reach ftpd on the 3DS')


connect()


def ensure_dir(path):
    parts = path.strip('/').split('/')
    cur = ''
    for p in parts:
        cur += '/' + p
        try:
            ftp.mkd(cur)
        except ftplib.error_perm:
            pass


def remote_size(path):
    for attempt in range(3):
        try:
            return ftp.size(path)
        except ftplib.error_perm:
            return None
        except (OSError, ftplib.Error):
            connect()
    return None


ok = bad = skipped = 0
todo = []
for dirpath, _, files in sorted(os.walk(root)):
    for name in sorted(files):
        todo.append((dirpath, name))
todo.sort(key=lambda t: t[1].endswith('.3dsx'))       # the app last: game files first
for dirpath, name in todo:
        local = os.path.join(dirpath, name)
        rel = os.path.relpath(local, root).replace(os.sep, '/')
        remote = '/' + rel
        size = os.path.getsize(local)
        if not upload_all and remote_size(remote) == size and not name.endswith('.3dsx'):
            skipped += 1
            continue
        ensure_dir(os.path.dirname(remote))
        for attempt in range(3):
            try:
                with open(local, 'rb') as f:
                    ftp.storbinary('STOR ' + remote, f, 16384)
                break
            except (OSError, ftplib.Error) as e:
                print(f'  retry {remote}: {e}')
                time.sleep(2)
                connect()
        got = remote_size(remote)
        if got == size:
            ok += 1
            print(f'ok    {remote} ({size} bytes)')
        else:
            bad += 1
            print(f'FAIL  {remote} (local {size}, 3DS {got})')
try:
    ftp.quit()
except Exception:
    pass
print(f'uploaded {ok}, unchanged {skipped}, failed {bad}')
sys.exit(1 if bad else 0)
