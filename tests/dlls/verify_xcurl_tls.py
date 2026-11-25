#!/usr/bin/env python3
"""Isolated Windows CI only: ephemeral loopback TLS CA, trusted and untrusted servers.
Never changes verification flags. Removes the exact temporary root on exit.
"""
import datetime as dt
import hashlib
import ipaddress
import ssl
import subprocess
import sys
import tempfile
import threading
from pathlib import Path
from cryptography import x509
from cryptography.hazmat.primitives import hashes,serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID,ExtendedKeyUsageOID
from verify_xcurl_winhttp import Handler,Server

def pair(root_name):
    now=dt.datetime.now(dt.timezone.utc)
    ca_key=rsa.generate_private_key(public_exponent=65537,key_size=2048)
    name=x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,root_name)])
    ca=(x509.CertificateBuilder().subject_name(name).issuer_name(name).public_key(ca_key.public_key())
        .serial_number(x509.random_serial_number()).not_valid_before(now-dt.timedelta(days=1))
        .not_valid_after(now+dt.timedelta(days=1))
        .add_extension(x509.BasicConstraints(ca=True,path_length=0),critical=True)
        .add_extension(x509.KeyUsage(False,False,False,False,False,True,True,False,False),critical=True)
        .add_extension(x509.SubjectKeyIdentifier.from_public_key(ca_key.public_key()),critical=False)
        .sign(ca_key,hashes.SHA256()))
    key=rsa.generate_private_key(public_exponent=65537,key_size=2048)
    leaf=(x509.CertificateBuilder().subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,"127.0.0.1")]))
        .issuer_name(name).public_key(key.public_key()).serial_number(x509.random_serial_number())
        .not_valid_before(now-dt.timedelta(days=1)).not_valid_after(now+dt.timedelta(days=1))
        .add_extension(x509.BasicConstraints(ca=False,path_length=None),critical=True)
        .add_extension(x509.SubjectAlternativeName([x509.IPAddress(ipaddress.ip_address("127.0.0.1"))]),critical=False)
        .add_extension(x509.ExtendedKeyUsage([ExtendedKeyUsageOID.SERVER_AUTH]),critical=False)
        .add_extension(x509.KeyUsage(True,False,True,False,False,False,False,False,False),critical=True)
        .add_extension(x509.AuthorityKeyIdentifier.from_issuer_public_key(ca_key.public_key()),critical=False)
        .sign(ca_key,hashes.SHA256()))
    return ca,leaf,key

def server(tmp,name,ca,leaf,key):
    cert=tmp/(name+".pem");private=tmp/(name+".key")
    cert.write_bytes(leaf.public_bytes(serialization.Encoding.PEM)+ca.public_bytes(serialization.Encoding.PEM))
    private.write_bytes(key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()))
    s=Server(("127.0.0.1",0),Handler)
    ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ctx.minimum_version=ctx.maximum_version=ssl.TLSVersion.TLSv1_2
    ctx.load_cert_chain(cert,private)
    s.socket=ctx.wrap_socket(s.socket,server_side=True)
    return s

def main():
    assert sys.platform=="win32","Windows runner only"
    assert __import__("os").environ.get("GITHUB_ACTIONS")=="true","Temporary trust store test is CI-only"
    exe=Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory() as d:
        tmp=Path(d);trusted=pair("XCurl ephemeral CI trusted root");untrusted=pair("XCurl ephemeral CI untrusted root")
        root=trusted[0].public_bytes(serialization.Encoding.DER)
        ca_file=tmp/"root.cer";ca_file.write_bytes(root)
        thumb=hashlib.sha1(root).hexdigest()
        servers=[]
        # The disposable hosted runner is elevated. CurrentUser Root can
        # trigger an interactive consent dialog even with certutil -f.
        # Use the runner's LocalMachine Root with exact-thumbprint cleanup.
        try:
            subprocess.run(["certutil","-f","-addstore","Root",str(ca_file)],check=True,timeout=30)
            for name,parts in (("trusted",trusted),("untrusted",untrusted)):
                s=server(tmp,name,*parts);t=threading.Thread(target=s.serve_forever,daemon=True);t.start();servers.append((s,t))
            subprocess.run([str(exe),*[str(s.server_port) for s,_ in servers]],check=True,timeout=90)
        finally:
            for s,t in servers:s.shutdown();s.server_close();t.join()
            subprocess.run(["certutil","-delstore","Root",thumb],check=True,timeout=30)
    print("XCurl isolated TLS trust fixture PASS",flush=True)
if __name__=="__main__":main()
