"""Optional foreground-only official-client /tp sweep; no injection or credentials."""
import argparse
import ctypes as C
from ctypes import wintypes as W
from pathlib import Path
import random
import time

MAX_MAP = 0x373 - 1  # Matching GWCA Maps.h: Count is exclusive.

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', type=int)
    parser.add_argument('--start', type=int, default=13)
    parser.add_argument('--end', type=int, default=MAX_MAP)
    parser.add_argument('--sequential', action='store_true', help='ascending order (the default)')
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    if not 1 <= args.start <= args.end <= MAX_MAP:
        parser.error(f'range must be within 1..{MAX_MAP}')
    if args.dry_run:
        print(f'IDs {args.start}..{args.end}, ascending without skips; random delay 3-5 seconds; no input sent.')
        return

    user = C.WinDLL('user32', use_last_error=True)
    kernel = C.WinDLL('kernel32', use_last_error=True)
    user.GetForegroundWindow.restype = W.HWND
    user.GetWindowThreadProcessId.argtypes = [W.HWND, C.POINTER(W.DWORD)]
    user.GetKeyboardLayout.argtypes = [W.DWORD]
    user.GetKeyboardLayout.restype = W.HANDLE
    user.VkKeyScanExW.argtypes = [W.WCHAR, W.HANDLE]
    user.VkKeyScanExW.restype = C.c_short
    class MouseInput(C.Structure):
        _fields_ = [('dx', W.LONG), ('dy', W.LONG), ('mouseData', W.DWORD), ('dwFlags', W.DWORD), ('time', W.DWORD), ('dwExtraInfo', C.c_size_t)]
    class KeyboardInput(C.Structure):
        _fields_ = [('wVk', W.WORD), ('wScan', W.WORD), ('dwFlags', W.DWORD), ('time', W.DWORD), ('dwExtraInfo', C.c_size_t)]
    class InputUnion(C.Union):
        _fields_ = [('mi', MouseInput), ('ki', KeyboardInput)]
    class Input(C.Structure):
        _fields_ = [('type', W.DWORD), ('data', InputUnion)]
    user.SendInput.argtypes = [W.UINT, C.POINTER(Input), C.c_int]
    user.SendInput.restype = W.UINT
    user.MapVirtualKeyExW.argtypes = [W.UINT, W.UINT, W.HANDLE]
    user.MapVirtualKeyExW.restype = W.UINT
    kernel.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
    kernel.OpenProcess.restype = W.HANDLE
    kernel.QueryFullProcessImageNameW.argtypes = [W.HANDLE, W.DWORD, W.LPWSTR, C.POINTER(W.DWORD)]
    kernel.GetExitCodeProcess.argtypes = [W.HANDLE, C.POINTER(W.DWORD)]
    kernel.CloseHandle.argtypes = [W.HANDLE]

    print('Keep chat closed. Focus the official Guild Wars window within 5 seconds.')
    print('Ctrl+C or switching windows stops the sweep.')
    time.sleep(5)
    window = user.GetForegroundWindow()
    owner = W.DWORD()
    thread = user.GetWindowThreadProcessId(window, C.byref(owner))
    if args.pid and owner.value != args.pid:
        raise RuntimeError('The specified client is not foreground; no input sent.')
    pid = owner.value
    process = kernel.OpenProcess(0x1000, False, pid)
    if not process:
        raise RuntimeError('Cannot verify foreground process; no input sent.')
    try:
        path = C.create_unicode_buffer(32768)
        size = W.DWORD(len(path))
        if not kernel.QueryFullProcessImageNameW(process, 0, path, C.byref(size)):
            raise RuntimeError('Cannot verify client path; no input sent.')
        if Path(path.value).name.lower() != 'gw.exe':
            raise RuntimeError('Foreground process is not Gw.exe; no input sent.')
        print(f'Selected PID {pid}: {path.value}', flush=True)
        layout = user.GetKeyboardLayout(thread)

        def check():
            code = W.DWORD()
            if not kernel.GetExitCodeProcess(process, C.byref(code)) or code.value != 259:
                raise RuntimeError('Selected client exited; sweep stopped.')
            current = user.GetForegroundWindow()
            current_owner = W.DWORD()
            user.GetWindowThreadProcessId(current, C.byref(current_owner))
            if current != window or current_owner.value != pid:
                raise RuntimeError('Selected window lost focus; sweep stopped.')

        def raw_key(vk, released=False):
            scan = user.MapVirtualKeyExW(vk, 0, layout)
            if not scan:
                raise RuntimeError(f'Cannot map keyboard key {vk}.')
            event = Input()
            event.type = 1
            event.data.ki.wScan = scan
            event.data.ki.dwFlags = 8 | (2 if released else 0)
            if user.SendInput(1, C.byref(event), C.sizeof(event)) != 1:
                raise RuntimeError('Windows blocked keyboard input. If Guild Wars runs as administrator, run AUTO TP.cmd as administrator too.')

        def key(vk):
            check()
            raw_key(vk)
            try:
                time.sleep(.12 if vk == 0x0D else .06)
            finally:
                raw_key(vk, True)
            time.sleep(.08 if vk == 0x0D else .025)

        def command(map_id):
            key(0x0D)
            time.sleep(.08)
            for char in f'/tp {map_id}':
                check()
                mapped = user.VkKeyScanExW(char, layout)
                if mapped == -1:
                    raise RuntimeError('Keyboard layout cannot type the command.')
                modifiers = [vk for mask, vk in ((1, 0x10), (2, 0x11), (4, 0x12)) if (mapped >> 8) & mask]
                try:
                    for vk in modifiers:
                        raw_key(vk)
                    key(mapped & 255)
                finally:
                    for vk in reversed(modifiers):
                        raw_key(vk, True)
                time.sleep(.015)
            key(0x0D)

        destinations = list(range(args.start, args.end + 1))
        for map_id in destinations:
            command(map_id)
            delay = random.uniform(3, 5)
            print(f'/tp {map_id} submitted; next request in {delay:.1f}s', flush=True)
            until = time.monotonic() + delay
            while time.monotonic() < until:
                check()
                time.sleep(.1)
        print('Sweep completed. Recording remains active.')
    finally:
        kernel.CloseHandle(process)

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print('\nSweep stopped. Recording remains active.')
    except Exception as error:
        print(f'STOPPED: {error}')
        raise SystemExit(1)
