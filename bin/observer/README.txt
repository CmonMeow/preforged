Separate native official-client observer state recorder

Start bin/observer/START OBSERVER CAPTURE.cmd and enter the official client's PID.
Keep the console open. Ctrl+C stops only recording; Guild Wars stays open.
Logs are written beside observer_capture.exe. Select matches normally with B.
The recorder follows map changes for up to one hour. Start it again for more.

No injection, hooks, remote calls, memory writes or automatic match selection.
Only supported official paths and the exact audited executable hash are accepted.
State samples aim at a 50ms cadence; actual timestamps and read spans are logged.
All living actors visible in the client are captured, including pets and NPCs.
Roster/map changes are not deaths. Gaps and unavailable actors are recorded.

This initial recorder captures positions, velocity, facing, floor, animation,
skill/state IDs and health fractions. It does NOT capture incoming combat packets
or original movement inputs. Other players' maximum HP/energy/armor are not proven
by these records. Damage-source and armor analysis require a later event recorder.
Observer catalog and observer flag native layouts remain unverified; failed catalog
validation is reported. char_230_raw/char_238_raw must not be interpreted as verified
observer fields. Earlier initial-trial logs called them observe_map_raw/type_raw.

Build: cmake -S tools/observer -B build/observer -A Win32
       cmake --build build/observer --config Release
It reuses the existing skills tool's audited external image/TLS read guards;
the skills tool and stable world logger are not changed.
