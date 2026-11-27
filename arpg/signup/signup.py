"""Self-service sign-up page for the owner's mates (m8-friends): name + password twice -> a normal player account.

Python stdlib only. Accounts are written like arpg/scripts/new-account.ps1 (sha_pass_hash = UPPER(SHA1(UPPER(name):UPPER(pass))),
v/s empty -> authserver derives them at the first login), never with GM rights and never over an existing account.
Settings: signup.local.json next to this file (see signup.local.json.example); defaults: 127.0.0.1:8090, no invite code, cap 20.
Run: py -I arpg/signup/signup.py [--port N] [--config FILE]   (arpg/scripts/start-signup.ps1 starts it detached)
"""
import argparse
import hashlib
import hmac
import html
import json
import os
import re
import subprocess
import sys
import threading
import time
from datetime import datetime
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
MYSQL = r'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe'
CNF = os.path.join(ROOT, 'arpg', 'scripts', 'mysql.local.cnf')
NAME_RE = re.compile(r'^[A-Za-z0-9]{3,16}$')
PASS_RE = re.compile(r'^[A-Za-z0-9]{4,16}$')
RESERVED = ('RNDBOT', 'ARPG')
TAG = 'signup'                      # auth.account.invited_by of accounts made here (cap counts them)
RATE_WINDOW = 600                   # seconds; attempts per address in it: CFG rate_max

CFG = {'bind': '127.0.0.1', 'port': 8090, 'invite_code': '', 'max_accounts': 20, 'realm_address': '',
       'log': r'D:\WoWMoPProject\ServerData\logs\signup.log', 'title': 'Pandaria ARPG', 'rate_max': 5}
_attempts = {}
_lock = threading.Lock()


def log(addr, name, result):
    line = '%s %s %s %s\n' % (datetime.now().strftime('%Y-%m-%d %H:%M:%S'), addr, name or '-', result)
    try:
        with open(CFG['log'], 'a', encoding='utf-8') as f:
            f.write(line)
    except OSError:
        sys.stderr.write(line)


def sql(statement):
    """Runs SQL on the auth DB. Every value in it has passed NAME_RE / PASS_RE (letters and digits only) or is a hex hash."""
    r = subprocess.run([MYSQL, '--defaults-extra-file=' + CNF, '-D', 'auth', '-B', '-N', '-e', statement],
                       capture_output=True, text=True, timeout=20)
    return r.returncode, r.stdout.strip(), r.stderr.strip()


def rate_limited(addr):
    now = time.time()
    with _lock:
        hits = [t for t in _attempts.get(addr, []) if now - t < RATE_WINDOW]
        hits.append(now)
        _attempts[addr] = hits
        return len(hits) > int(CFG['rate_max'])


def register(name, password, confirm, invite, addr):
    """Returns (ok, message). The message is shown to the person signing up."""
    if rate_limited(addr):
        return False, 'Too many attempts - wait 10 minutes.'
    if CFG['invite_code'] and not hmac.compare_digest(invite.encode(), CFG['invite_code'].encode()):
        return False, 'Wrong invite code.'
    if not NAME_RE.match(name):
        return False, 'Name: 3-16 letters or digits.'
    if not PASS_RE.match(password):
        return False, 'Password: 4-16 letters or digits.'
    if password != confirm:
        return False, 'The two passwords differ.'
    u = name.upper()
    if u.startswith(RESERVED):
        return False, 'That name is reserved - pick another.'
    code, out, _ = sql("SELECT COUNT(*) FROM account WHERE invited_by = '%s'" % TAG)
    if code != 0:
        return False, 'The server database is not reachable right now.'
    if int(out or 0) >= int(CFG['max_accounts']):
        return False, 'No more sign-ups on this server - ask the owner.'
    sha = hashlib.sha1(('%s:%s' % (u, password.upper())).encode()).hexdigest().upper()
    # Plain INSERT: an existing name fails (duplicate key) and is never changed.
    code, _, err = sql("INSERT INTO account (username, sha_pass_hash, email, reg_mail, joindate, expansion, invited_by) "
                       "VALUES ('%s', '%s', '', '', NOW(), 4, '%s'); "
                       "INSERT INTO realmcharacters (realmid, acctid, numchars) SELECT r.id, a.id, 0 FROM realmlist r, account a "
                       "WHERE a.username = '%s' AND NOT EXISTS (SELECT 1 FROM realmcharacters c WHERE c.acctid = a.id AND c.realmid = r.id);"
                       % (u, sha, TAG, u))
    if code != 0:
        if 'Duplicate' in err:
            return False, 'That name is taken.'
        return False, 'Could not create the account (server error).'
    return True, 'Account %s created. Log in with it in the game.' % u


PAGE = '''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title} - sign up</title><style>
:root {{ --bg:#f4f1ea; --fg:#222; --card:#fff; --accent:#8a5a14; --bad:#a3261b; --good:#2f6b2f; }}
@media (prefers-color-scheme: dark) {{ :root {{ --bg:#1b1a17; --fg:#e8e4da; --card:#262420; --accent:#d9a54a; --bad:#ef6f62; --good:#7fc57f; }} }}
body {{ background:var(--bg); color:var(--fg); font:16px/1.45 system-ui, sans-serif; margin:0; padding:24px 16px; }}
main {{ max-width:420px; margin:auto; background:var(--card); padding:24px; border-radius:10px; }}
h1 {{ font-size:1.4em; margin-top:0; color:var(--accent); }} label {{ display:block; margin-top:12px; font-weight:600; }}
input {{ width:100%; box-sizing:border-box; padding:8px; font-size:1em; margin-top:4px; }}
button {{ margin-top:18px; width:100%; padding:10px; font-size:1em; background:var(--accent); color:#fff; border:0; border-radius:6px; }}
.msg {{ padding:10px; border-radius:6px; margin-bottom:12px; }} .bad {{ color:var(--bad); }} .good {{ color:var(--good); }}
small {{ display:block; margin-top:16px; opacity:.8; }}
</style></head><body><main><h1>{title}</h1>{message}
<form method="post" action="/register">
<label>Account name<input name="name" maxlength="16" pattern="[A-Za-z0-9]{{3,16}}" required autocomplete="username"></label>
<label>Password<input name="password" type="password" maxlength="16" pattern="[A-Za-z0-9]{{4,16}}" required autocomplete="new-password"></label>
<label>Password again<input name="confirm" type="password" maxlength="16" required autocomplete="new-password"></label>
{invite}<button type="submit">Create account</button></form>
<small>Letters and digits only; name 3-16, password 4-16 characters. {realm}</small></main></body></html>'''


def render(message='', good=False):
    msg = '<div class="msg %s">%s</div>' % ('good' if good else 'bad', html.escape(message)) if message else ''
    inv = '<label>Invite code<input name="invite" maxlength="64" required></label>' if CFG['invite_code'] else ''
    realm = ('Realm address: <b>%s</b>' % html.escape(CFG['realm_address'])) if CFG['realm_address'] else ''
    return PAGE.format(title=html.escape(CFG['title']), message=msg, invite=inv, realm=realm).encode()


class Handler(BaseHTTPRequestHandler):
    server_version = 'signup'

    def send_page(self, body, status=200):
        self.send_response(status)
        self.send_header('Content-Type', 'text/html; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.send_header('X-Frame-Options', 'DENY')
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path not in ('/', '/register'):
            return self.send_page(b'not found', 404)
        self.send_page(render())

    def do_POST(self):
        if self.path != '/register':
            return self.send_page(b'not found', 404)
        n = int(self.headers.get('Content-Length') or 0)
        if n > 4096:
            return self.send_page(b'too large', 413)
        form = parse_qs(self.rfile.read(n).decode('utf-8', 'replace'))
        get = lambda k: (form.get(k) or [''])[0].strip()
        name = get('name')
        addr = self.client_address[0]
        ok, message = register(name, get('password'), get('confirm'), get('invite'), addr)
        log(addr, name if NAME_RE.match(name) else '(bad name)', 'ok' if ok else message)
        self.send_page(render(message, ok), 200 if ok else 400)

    def log_message(self, fmt, *args):   # keep the console quiet; signup.log has the outcomes
        pass


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--config', default=os.path.join(HERE, 'signup.local.json'))
    ap.add_argument('--port', type=int)
    ap.add_argument('--bind')
    a = ap.parse_args()
    if os.path.exists(a.config):
        with open(a.config, encoding='utf-8') as f:
            CFG.update(json.load(f))
    if a.port:
        CFG['port'] = a.port
    if a.bind:
        CFG['bind'] = a.bind
    srv = ThreadingHTTPServer((CFG['bind'], int(CFG['port'])), Handler)
    print('signup listening on http://%s:%d/ (invite code: %s, cap %s)' % (CFG['bind'], CFG['port'],
          'yes' if CFG['invite_code'] else 'no', CFG['max_accounts']), flush=True)
    srv.serve_forever()


if __name__ == '__main__':
    main()
