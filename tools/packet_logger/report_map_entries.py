"""Read capture schemas and report correlated instance info/file observations."""
import argparse
import json
import struct
from pathlib import Path

# Exact live descriptors, rather than potentially stale GWCA opcode numbers.
INFO = [80, 516, 260, 1028, 260, 260, 260]
FILE = [1028, 18, 516, 260, 260, 2053]


def report(path):
    schemas = {}
    pending = None
    entries = []
    skipped = 0
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        try:
            row = json.loads(line)
        except ValueError:
            skipped += 1
            continue
        opcode = row.get("opcode")
        if row.get("event") == "schema":
            fields = row.get("fields", [])
            if fields and fields[0] == opcode:
                schemas[opcode] = fields[1:]
            continue
        if row.get("event") != "packet" or row.get("direction") != "StoC":
            continue
        layout = schemas.get(opcode)
        if layout not in (INFO, FILE) or row.get("complete") is not True:
            continue
        blob = bytes.fromhex(row["hex"])
        expected = 32 if layout == INFO else 36
        if len(blob) != expected or row.get("bytes") != expected or struct.unpack_from("<I", blob)[0] != opcode:
            raise ValueError(f"invalid instance packet at line {number}")
        if layout == INFO:
            values = struct.unpack("<8I", blob)
            pending = dict(map=values[2], info_tick=row["tick_ms"], info_line=number,
                           info_opcode=opcode, info_hex=row["hex"])
        elif pending is not None:
            delay = row["tick_ms"] - pending["info_tick"]
            if 0 <= delay <= 10000:
                file_id, x, y, plane = struct.unpack_from("<IffH", blob, 4)
                entries.append(dict(**pending, file_id=file_id, x=x, y=y, plane=plane,
                                    file_tick=row["tick_ms"], file_line=number,
                                    file_opcode=opcode, file_hex=row["hex"]))
            pending = None
    return dict(source=str(path.resolve()), skipped_lines=skipped, entries=entries,
                limitation="Observed current-client mapping only; validate against the target client's DAT before installing a binding.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    result = json.dumps(report(args.capture), indent=2)
    if args.output:
        args.output.write_text(result + "\n", encoding="utf-8")
    print(result)
