"""ui/ui_motd.cpp's value parsers on the server (the Store build, with the parsing around them:
tests/replay/verify_ui_motd.py)."""
import struct

from test_json import FAKES, OBSERVE, session

SERVER = ["CoDMPServer_PC"]
S_MOTD = 0x078DE1E0  # analysis/CoDMPServer_PC/pdb/statics.tsv

# Values of every kind, each read from its own token (index 2: the value of the object's first key)
VALUES = [b'2', b'3', b'-7', b'65537', b'true', b'false', b'null', b'"text"', b'""', b'"' + b"w" * 1100 + b'"',
          b'{}', b'[1]', b'"t\\"q"', b'0x10']


def inputs():
    out = []
    for v in VALUES:
        js = b'{"k":' + v + b'}'
        for curr in (2, 3):  # the value, and past the end
            run, _ = session(js, curr)

            def setup(w, run=run):
                run(w)
                w.write(S_MOTD, bytes([0xA5]) * 0x6DC)
            out.append({"args": [], "setup": setup})
    return out


PARSERS = ["MOTD_ParseVersion", "MOTD_ParseMOTDVersion", "MOTD_ParseSingleView", "MOTD_ParseExpiration",
           "MOTD_ParseImageCategory", "MOTD_ParseTitle", "MOTD_ParseMessage", "MOTD_ParseImage", "MOTD_ParseAction",
           "MOTD_ParseActionContext", "MOTD_ParseActionString", "MOTD_ParseBannerTitle", "MOTD_ParseBannerMessage",
           "MOTD_ParseBannerImage", "MOTD_ParseBannerAction", "MOTD_ParseBannerActionContext",
           "MOTD_ParseBannerActionString", "MOTD_ParseEliteImage", "MOTD_ParseFounderImage"]

CASES = [{"name": n, "ret": "bool", "builds": SERVER, "fakes": FAKES, "observe": OBSERVE, "inputs": inputs()}
         for n in PARSERS]
