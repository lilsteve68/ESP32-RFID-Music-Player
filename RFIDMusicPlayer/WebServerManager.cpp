#include "WebServerManager.h"
#include "RFID.h"
#include "MusicPlayer.h"
#include "TrackDatabase.h"
#include "Pins.h"
#include <SD.h>

WebServerManager webServerManager;

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>RFID Music Linker</title>
    <style>
        body { font-family: Arial, sans-serif; background: #121212; color: #fff; text-align: center; padding: 15px; margin: 0; }
        .card { background: #1e1e1e; max-width: 450px; margin: 15px auto; padding: 20px; border-radius: 12px; box-shadow: 0 4px 12px rgba(0,0,0,0.5); }
        h2 { color: #1db954; margin-top: 0; }
        .val { font-weight: bold; color: #00e676; font-size: 1.1em; }
        select, input[type="text"] { width: 90%; padding: 10px; margin: 8px 0; border-radius: 6px; border: none; background: #2a2a2a; color: #fff; box-sizing: border-box; }
        .slider { width: 85%; margin: 15px 0; accent-color: #1db954; cursor: pointer; }
        .btn { background: #1db954; border: none; color: white; padding: 10px 18px; margin: 5px; border-radius: 20px; cursor: pointer; font-weight: bold; }
        .btn-del { background: #e53935; padding: 4px 10px; border-radius: 6px; }
        .btn-stop { background: #e53935; width: 90%; }
        table { width: 100%; margin-top: 10px; border-collapse: collapse; }
        td, th { padding: 8px; border-bottom: 1px solid #333; text-align: left; font-size: 0.9em; }
    </style>
</head>
<body>
    <h1>🎵 RFID Music Linker</h1>

    <div class="card">
        <h2>Live Reader</h2>
        <p>Scanned Tag UID: <span id="uid" class="val">None</span></p>
        <p>Player Status: <span id="status" class="val">Idle</span></p>
        
        <p>🔊 Volume: <span id="volLabel" class="val">100%</span></p>
        <input type="range" min="0" max="100" value="100" class="slider" id="volRange" onchange="setVolume(this.value)" oninput="document.getElementById('volLabel').innerText = this.value + '%'">
        <br><br>
        <button class="btn btn-stop" onclick="sendCommand('stop')">⏹ Stop Playback</button>
    </div>

    <div class="card">
        <h2>Link Tag to Song</h2>
        <input type="text" id="tagInput" placeholder="Tag UID (Scan tag or type manually)">
        <select id="trackSelect"><option>Loading SD tracks...</option></select>
        <br>
        <button class="btn" onclick="saveMapping()">💾 Save Link</button>
    </div>

    <div class="card">
        <h2>Active Mappings</h2>
        <table>
            <thead><tr><th>Tag UID</th><th>Song Path</th><th>Action</th></tr></thead>
            <tbody id="mappingsTable"></tbody>
        </table>
    </div>

    <script>
        let isDraggingVolume = false;

        document.getElementById('volRange').addEventListener('mousedown', () => isDraggingVolume = true);
        document.getElementById('volRange').addEventListener('mouseup', () => isDraggingVolume = false);
        document.getElementById('volRange').addEventListener('touchstart', () => isDraggingVolume = true);
        document.getElementById('volRange').addEventListener('touchend', () => isDraggingVolume = false);

        async function updateStatus() {
            try {
                let res = await fetch('/api/status');
                let data = await res.json();
                document.getElementById('uid').innerText = data.uid || 'None';
                document.getElementById('status').innerText = data.playing ? 'Playing 🎶' : 'Idle 💤';
                
                if (!isDraggingVolume) {
                    document.getElementById('volRange').value = data.volume;
                    document.getElementById('volLabel').innerText = data.volume + '%';
                }

                if (data.uid && data.uid !== 'None' && !document.getElementById('tagInput').value) {
                    document.getElementById('tagInput').value = data.uid;
                }
            } catch(e) {}
        }

        async function setVolume(val) {
            await fetch('/api/volume?level=' + val, { method: 'POST' });
        }

        async function loadTracks() {
            try {
                let res = await fetch('/api/tracks');
                let tracks = await res.json();
                let select = document.getElementById('trackSelect');
                select.innerHTML = '';
                tracks.forEach(t => {
                    let opt = document.createElement('option');
                    opt.value = t;
                    opt.innerText = t;
                    select.appendChild(opt);
                });
            } catch(e) {}
        }

        async function loadMappings() {
            try {
                let res = await fetch('/api/mappings');
                let maps = await res.json();
                let tbody = document.getElementById('mappingsTable');
                tbody.innerHTML = '';
                for (let [uid, file] of Object.entries(maps)) {
                    let tr = document.createElement('tr');
                    tr.innerHTML = `<td><b>${uid}</b></td><td>${file}</td><td><button class="btn btn-del" onclick="deleteMapping('${uid}')">X</button></td>`;
                    tbody.appendChild(tr);
                }
            } catch(e) {}
        }

        async function saveMapping() {
            let uid = document.getElementById('tagInput').value.trim();
            let file = document.getElementById('trackSelect').value;
            if(!uid || !file) return alert('Select a track and scan a tag!');
            
            await fetch(`/api/map?uid=${encodeURIComponent(uid)}&file=${encodeURIComponent(file)}`, { method: 'POST' });
            loadMappings();
        }

        async function deleteMapping(uid) {
            await fetch(`/api/delete?uid=${encodeURIComponent(uid)}`, { method: 'POST' });
            loadMappings();
        }

        async function sendCommand(cmd) {
            await fetch('/api/control?cmd=' + cmd, { method: 'POST' });
            updateStatus();
        }

        setInterval(updateStatus, 1000);
        updateStatus();
        loadTracks();
        loadMappings();
    </script>
</body>
</html>
)rawliteral";

void WebServerManager::begin(const char* ssid, const char* password)
{
    WiFi.softAP(ssid, password);
    Serial.print("[WIFI] AP Started: ");
    Serial.println(ssid);
    Serial.print("[WIFI] Dashboard: http://");
    Serial.println(WiFi.softAPIP());

    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/api/tracks", HTTP_GET, [this]() { handleTracks(); });
    server.on("/api/mappings", HTTP_GET, [this]() { handleMappings(); });
    server.on("/api/map", HTTP_POST, [this]() { handleSaveMapping(); });
    server.on("/api/delete", HTTP_POST, [this]() { handleDeleteMapping(); });
    server.on("/api/control", HTTP_POST, [this]() { handleControl(); });
    server.on("/api/volume", HTTP_POST, [this]() { handleVolume(); });

    server.begin();
    Serial.println("[WEB] Server Running");
}

void WebServerManager::update()
{
    server.handleClient();
}

void WebServerManager::setLastUID(const String& uid)
{
    lastUID = uid;
}

void WebServerManager::handleRoot()
{
    server.send(200, "text/html; charset=utf-8", INDEX_HTML);
}

void WebServerManager::handleStatus()
{
    int volPercent = (int)(music.getVolume() * 100.0f);
    String json = "{\"uid\":\"" + lastUID + "\",\"playing\":" + String(music.isPlaying() ? "true" : "false") + ",\"volume\":" + String(volPercent) + "}";
    server.send(200, "application/json", json);
}

void WebServerManager::handleTracks()
{
    digitalWrite(RFID_CS, HIGH);
    digitalWrite(SD_CS, LOW);

    File musicDir = SD.open("/music");
    String json = "[";

    if (musicDir && musicDir.isDirectory())
    {
        File file = musicDir.openNextFile();
        bool first = true;
        while (file)
        {
            String name = String(file.name());
            if (name.endsWith(".mp3") || name.endsWith(".MP3"))
            {
                if (!first) json += ",";
                json += "\"/music/" + name + "\"";
                first = false;
            }
            file = musicDir.openNextFile();
        }
    }
    json += "]";

    digitalWrite(SD_CS, HIGH);
    server.send(200, "application/json", json);
}

void WebServerManager::handleMappings()
{
    server.send(200, "application/json", TrackDatabase::getMappingsJson());
}

void WebServerManager::handleSaveMapping()
{
    if (server.hasArg("uid") && server.hasArg("file"))
    {
        TrackDatabase::setMapping(server.arg("uid"), server.arg("file"));
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    }
    else
    {
        server.send(400, "application/json", "{\"error\":\"missing args\"}");
    }
}

void WebServerManager::handleDeleteMapping()
{
    if (server.hasArg("uid"))
    {
        TrackDatabase::removeMapping(server.arg("uid"));
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    }
    else
    {
        server.send(400, "application/json", "{\"error\":\"missing args\"}");
    }
}

void WebServerManager::handleControl()
{
    if (server.hasArg("cmd") && server.arg("cmd") == "stop")
    {
        music.reset();
    }
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handleVolume()
{
    if (server.hasArg("level"))
    {
        int level = server.arg("level").toInt();
        float vol = level / 100.0f;
        music.setVolume(vol);
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    }
    else
    {
        server.send(400, "application/json", "{\"error\":\"missing level\"}");
    }
}