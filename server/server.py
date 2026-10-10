"""
IoT Gateway Honeypot log collector and dashboard.
Run only on a trusted lab network. This is a learning prototype, not a production IDS.
"""
from flask import Flask, request, jsonify, render_template_string
import sqlite3
from datetime import datetime, timezone
from pathlib import Path

app = Flask(__name__)
DB_PATH = Path(__file__).with_name("honeypot.db")

DASHBOARD = """
<!doctype html><html><head><meta charset="utf-8"><meta http-equiv="refresh" content="5">
<title>IoT Gateway Honeypot</title>
<style>
body{font-family:Arial,sans-serif;margin:2rem;background:#f4f6f8;color:#172033}
h1{color:#12345a}.cards{display:flex;gap:12px;flex-wrap:wrap}
.card{background:white;padding:16px;border-radius:10px;min-width:150px;box-shadow:0 1px 5px #ddd}
table{border-collapse:collapse;width:100%;background:white;margin-top:20px}
th,td{padding:10px;border-bottom:1px solid #ddd;text-align:left;font-size:14px}
th{background:#12345a;color:white}.high{color:#b42318;font-weight:bold}.normal{color:#067647}
small{color:#667085}
</style></head><body>
<h1>IoT Gateway Honeypot</h1><p>Lab monitoring dashboard · auto-refreshes every 5 seconds</p>
<div class="cards">
<div class="card"><small>Total events</small><h2>{{ total }}</h2></div>
<div class="card"><small>Suspicious events</small><h2>{{ suspicious }}</h2></div>
<div class="card"><small>Unique source IPs</small><h2>{{ unique_ips }}</h2></div>
</div>
<table><tr><th>Time (UTC)</th><th>Device</th><th>Source IP</th><th>Method</th><th>Path</th><th>Classification</th></tr>
{% for e in events %}
<tr><td>{{e[1]}}</td><td>{{e[2]}}</td><td>{{e[3]}}</td><td>{{e[4]}}</td><td>{{e[5]}}</td>
<td class="{{'high' if e[6] != 'NORMAL' else 'normal'}}">{{e[6]}}</td></tr>
{% endfor %}</table>
<p><small>Use only in an authorized, isolated test network. Do not expose this prototype to the public Internet.</small></p>
</body></html>
"""

def connect_db():
    con = sqlite3.connect(DB_PATH)
    con.execute("""CREATE TABLE IF NOT EXISTS events (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        timestamp TEXT NOT NULL,
        device_id TEXT,
        source_ip TEXT,
        method TEXT,
        path TEXT,
        classification TEXT,
        status_code INTEGER
    )""")
    con.commit()
    return con

def classify(method, path):
    suspicious_paths = {"/admin", "/config", "/debug", "/login", "/.env", "/wp-admin"}
    return "SUSPICIOUS" if path in suspicious_paths else "NORMAL"

@app.post("/api/log")
def ingest():
    data = request.get_json(silent=True) or {}
    path = str(data.get("path", "/"))[:200]
    method = str(data.get("method", request.method))[:10]
    source_ip = str(data.get("source_ip") or request.remote_addr or "unknown")[:64]
    device_id = str(data.get("device_id", "GW-001"))[:40]
    classification = classify(method, path)
    timestamp = datetime.now(timezone.utc).isoformat(timespec="seconds")
    con = connect_db()
    con.execute("INSERT INTO events(timestamp,device_id,source_ip,method,path,classification,status_code) VALUES(?,?,?,?,?,?,?)",
                (timestamp, device_id, source_ip, method, path, classification, data.get("status_code")))
    con.commit()
    con.close()
    return jsonify({"ok": True, "classification": classification}), 201

@app.route("/")
def dashboard():
    con = connect_db()
    events = con.execute("SELECT * FROM events ORDER BY id DESC LIMIT 100").fetchall()
    total = con.execute("SELECT COUNT(*) FROM events").fetchone()[0]
    suspicious = con.execute("SELECT COUNT(*) FROM events WHERE classification='SUSPICIOUS'").fetchone()[0]
    unique_ips = con.execute("SELECT COUNT(DISTINCT source_ip) FROM events").fetchone()[0]
    con.close()
    return render_template_string(DASHBOARD, events=events, total=total, suspicious=suspicious, unique_ips=unique_ips)

if __name__ == "__main__":
    connect_db().close()
    # Set host to 0.0.0.0 only for a trusted lab network.
    app.run(host="0.0.0.0", port=5000, debug=False)
