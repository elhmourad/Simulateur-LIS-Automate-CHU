#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

ESP8266WebServer server(80);

String state     = "EN ATTENTE";
String echantillon = "--";
String analyse   = "--";
String resultat  = "--";
int    progress  = 0;
int    etape     = 1;
bool   nouvelleCommande = false;

String cmdId   = "";
String cmdType = "";
String cmdVal  = ""; // <-- المتغير الجديد اللي غيخزن النتيجة اللي كتبتي فالتليفون

// =========================================================================
const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>LIS — Automate ASTM</title>
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
    :root {
      --navy:    #0b1622; --panel:   #111d2c; --card:    #162032;
      --border:  #1e3a5f; --accent:  #1a8cff; --accent2: #0d6ecc;
      --green:   #00c896; --amber:   #f5a623; --red:     #e84040;
      --text1:   #e8f0fe; --text2:   #7a9bbf; --text3:   #3d5c7a;
      --mono:    'Courier New', 'Lucida Console', monospace;
      --sans:    system-ui, -apple-system, 'Segoe UI', sans-serif;
    }
    body { font-family: var(--sans); background: var(--navy); color: var(--text1); min-height: 100vh; display: flex; flex-direction: column; }
    header { background: var(--panel); border-bottom: 1px solid var(--border); padding: 0 15px; height: 60px; display: flex; align-items: center; justify-content: space-between; position: sticky; top: 0; z-index: 10; }
    .header-left { display: flex; align-items: center; gap: 10px; }
    .logo-mark { width: 34px; height: 34px; background: var(--accent); border-radius: 8px; display: flex; align-items: center; justify-content: center; font-family: var(--mono); font-weight: 700; font-size: 13px; color: #fff; }
    .app-title { font-size: 14px; font-weight: 600; color: var(--text1); }
    .app-sub { font-size: 10px; color: var(--text2); text-transform: uppercase; }
    main { flex: 1; padding: 15px; max-width: 1000px; width: 100%; margin: 0 auto; display: flex; flex-direction: column; gap: 15px; }
    
    .two-col { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; }
    @media (max-width: 768px) { .two-col { grid-template-columns: 1fr; } } /* 100% Responsive for Mobile */
    
    .panel { background: var(--panel); border: 1px solid var(--border); border-radius: 12px; padding: 15px; display: flex; flex-direction: column; gap: 12px; }
    .panel-head { display: flex; align-items: center; gap: 10px; padding-bottom: 10px; border-bottom: 1px solid var(--border); }
    .panel-title { font-size: 14px; font-weight: 600; }
    
    .field-group { display: flex; flex-direction: column; gap: 4px; }
    .field-label { font-size: 11px; text-transform: uppercase; color: var(--text2); }
    .field-input { background: var(--card); border: 1px solid var(--border); border-radius: 8px; color: var(--text1); font-size: 14px; padding: 12px; width: 100%; outline: none; }
    .field-input:focus { border-color: var(--accent); }
    .btn-send { margin-top: 10px; padding: 14px; background: var(--accent); border: none; border-radius: 8px; color: #fff; font-size: 15px; font-weight: bold; cursor: pointer; width: 100%; transition: 0.2s; }
    .btn-send:active { transform: scale(0.98); }
    
    .metrics { display: grid; grid-template-columns: repeat(auto-fit, minmax(80px, 1fr)); gap: 10px; }
    .metric { background: var(--card); border: 1px solid var(--border); border-radius: 8px; padding: 10px; text-align: center; }
    .metric-label { font-size: 10px; color: var(--text2); text-transform: uppercase; margin-bottom: 5px; }
    .metric-value { font-size: 16px; font-weight: bold; color: var(--text1); }
    .metric-value.result { color: var(--green); }
    
    .prog-wrap { background: var(--card); border: 1px solid var(--border); border-radius: 8px; padding: 12px; }
    .prog-header { display: flex; justify-content: space-between; margin-bottom: 8px; font-size: 11px; color: var(--text2); }
    .prog-track { height: 8px; background: var(--border); border-radius: 4px; overflow: hidden; }
    .prog-fill { height: 100%; width: 0%; background: linear-gradient(90deg, var(--accent), var(--green)); transition: 0.5s; }
    
    .timeline { display: flex; justify-content: space-between; position: relative; margin-top: 10px; overflow-x: auto; padding-bottom: 10px;}
    .tl-track { position: absolute; top: 15px; left: 15px; right: 15px; height: 2px; background: var(--border); }
    .tl-fill { height: 100%; background: var(--green); width: 0%; transition: 0.6s; }
    .step { position: relative; z-index: 2; display: flex; flex-direction: column; align-items: center; gap: 5px; flex: 1; min-width: 60px; }
    .step-circle { width: 30px; height: 30px; border-radius: 50%; background: var(--card); border: 2px solid var(--border); display: flex; align-items: center; justify-content: center; font-size: 12px; font-weight: bold; color: var(--text2); transition: 0.3s; }
    .step-label { font-size: 9px; text-align: center; color: var(--text2); }
    .step.done .step-circle { background: rgba(0,200,150,0.15); border-color: var(--green); color: var(--green); }
    .step.active .step-circle { background: rgba(245,166,35,0.15); border-color: var(--amber); color: var(--amber); box-shadow: 0 0 10px rgba(245,166,35,0.3); }
  </style>
</head>
<body>
  <header>
    <div class="header-left">
      <div class="logo-mark">LIS</div>
      <div>
        <div class="app-title">Laboratoire SI</div>
        <div class="app-sub">Supervision Automate</div>
      </div>
    </div>
  </header>
  <main>
    <div class="two-col">
      <!-- PANEL SAISIE -->
      <div class="panel">
        <div class="panel-head"><div><div class="panel-title">Saisie LIS / Simulateur</div></div></div>
        
        <div class="field-group">
          <div class="field-label">N° Échantillon</div>
          <input class="field-input" type="text" id="inputId" value="456">
        </div>
        
        <div class="field-group">
          <div class="field-label">Type d'analyse</div>
          <select class="field-input" id="inputType">
            <option value="GLUCOSE">Glucose</option>
            <option value="CHOLESTEROL">Cholestérol</option>
            <option value="UREE">Urée</option>
            <option value="CREATININE">Créatinine</option>
            <option value="TRIGLYCERIDES">Triglycérides</option>
            <option value="HEMOGLOBINE">Hémoglobine</option>
          </select>
        </div>

        <div class="field-group">
          <div class="field-label">Valeur à simuler (ex: 1.88)</div>
          <input class="field-input" type="number" step="0.01" id="inputResult" value="1.88">
        </div>

        <button class="btn-send" id="btnSend" onclick="envoyerPrescription()">▶ Lancer l'Analyse</button>
      </div>

      <!-- PANEL RESULTAT -->
      <div class="panel">
        <div class="panel-head"><div><div class="panel-title">État de l'Automate : <span id="stateBadge" style="color:var(--amber)">EN ATTENTE</span></div></div></div>
        <div class="metrics">
          <div class="metric"><div class="metric-label">Échantillon</div><div class="metric-value" id="echID">--</div></div>
          <div class="metric"><div class="metric-label">Analyse</div><div class="metric-value" id="anaType">--</div></div>
          <div class="metric"><div class="metric-label">Résultat</div><div class="metric-value result" id="resultVal">--</div></div>
        </div>
        <div class="prog-wrap">
          <div class="prog-header"><span>Progression</span><span id="progPct" style="color:var(--green)">0 %</span></div>
          <div class="prog-track"><div class="prog-fill" id="progressBar"></div></div>
        </div>
      </div>
    </div>

    <!-- TIMELINE -->
    <div class="panel" style="overflow:hidden;">
      <div class="timeline">
        <div class="tl-track"><div class="tl-fill" id="tlFill"></div></div>
        <div class="step" id="s1"><div class="step-circle">1</div><div class="step-label">Prescription</div></div>
        <div class="step" id="s2"><div class="step-circle">2</div><div class="step-label">Reçu</div></div>
        <div class="step" id="s3"><div class="step-circle">3</div><div class="step-label">Analyse</div></div>
        <div class="step" id="s4"><div class="step-circle">4</div><div class="step-label">Résultat</div></div>
        <div class="step" id="s5"><div class="step-circle">5</div><div class="step-label">Validation</div></div>
      </div>
    </div>
  </main>

  <script>
    function envoyerPrescription() {
      var id   = document.getElementById('inputId').value;
      var type = document.getElementById('inputType').value;
      var val  = document.getElementById('inputResult').value; // Récupère la valeur tapée
      
      if (!id || !val) { alert("Veuillez remplir l'ID et la valeur."); return; }
      
      var btn = document.getElementById('btnSend');
      btn.textContent = '↻ Envoi...';
      
      fetch('/api/order?id=' + encodeURIComponent(id) + '&type=' + type + '&val=' + encodeURIComponent(val))
        .then(() => setTimeout(() => btn.textContent = '▶ Lancer l\'Analyse', 1500));
    }

    function fetchData() {
      fetch('/api/data')
        .then(r => r.json())
        .then(d => {
          document.getElementById('echID').textContent    = d.echantillon;
          document.getElementById('anaType').textContent  = d.analyse;
          document.getElementById('resultVal').textContent= d.resultat;
          document.getElementById('progressBar').style.width = d.progress + '%';
          document.getElementById('progPct').textContent  = d.progress + ' %';
          document.getElementById('stateBadge').textContent = d.state;

          for (var i = 1; i <= 5; i++) {
            var s = document.getElementById('s' + i);
            s.className = 'step' + (i < d.etape ? ' done' : (i == d.etape ? ' active' : ''));
          }
          document.getElementById('tlFill').style.width = ((d.etape - 1) / 4 * 100) + '%';
        });
    }
    setInterval(fetchData, 1000);
  </script>
</body>
</html>
)=====";

// =========================================================================

void handleRoot()  { server.send(200, "text/html", MAIN_page); }

void handleData() {
  String json = "{\"state\":\"" + state + "\",\"echantillon\":\"" + echantillon + "\",\"analyse\":\"" + analyse + "\",\"resultat\":\"" + resultat + "\",\"progress\":" + String(progress) + ",\"etape\":" + String(etape) + "}";
  server.send(200, "application/json", json);
}

void handleOrder() {
  cmdId   = server.arg("id");
  cmdType = server.arg("type");
  cmdVal  = server.arg("val"); // القيمة اللي دخل اليوزر
  nouvelleCommande = true;
  server.send(200, "text/plain", "OK");
}

void updateOLED() {
  display.clearDisplay();
  display.fillRect(0, 0, 128, 12, WHITE);
  display.setTextColor(BLACK, WHITE);
  display.setCursor(2, 2);
  display.print("IP: "); display.print(WiFi.softAPIP().toString());
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0, 16);
  display.print("Etat: "); display.println(state);
  display.drawLine(0, 26, 128, 26, WHITE);
  display.setCursor(0, 30);
  display.print("Ech: ");  display.println(echantillon);
  display.print("Type: "); display.println(analyse);
  if (progress > 0) {
    display.drawRect(0, 50, 128, 10, WHITE);
    int fillWidth = map(progress, 0, 100, 0, 124);
    display.fillRect(2, 52, fillWidth, 6, WHITE);
  } else {
    display.setCursor(0, 50);
    display.print("Res: "); display.print(resultat);
  }
  display.display();
}

void attenteActive(int tempsMs) {
  unsigned long debut = millis();
  while (millis() - debut < tempsMs) {
    server.handleClient();
    yield();
    delay(10);
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin(12, 14); // تأكد من الـ pins ديال الشاشة ديالك
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Erreur OLED");
    for (;;);
  }
  WiFi.mode(WIFI_AP);
  WiFi.softAP("CHU_Automate_Pro", "12345678");
  server.on("/",          handleRoot);
  server.on("/api/data",  handleData);
  server.on("/api/order", handleOrder);
  server.begin();
  updateOLED();
}

void loop() {
  server.handleClient();

  if (nouvelleCommande) {
    nouvelleCommande = false;

    // ── ETAPE 2 : Réception
    etape = 2; state = "BIEN RECU";
    echantillon = cmdId; analyse = cmdType; resultat = "...";
    updateOLED();
    attenteActive(2000);

    // ── ETAPE 3 : Analyse
    etape = 3; state = "ANALYSE EN COURS";
    for (int i = 0; i <= 100; i += 5) {
      progress = i; updateOLED(); attenteActive(200);
    }

    // ── ETAPE 4 : Résultat (باستعمال القيمة اللي دخلها اليوزر)
    etape = 4; progress = 0; state = "RESULTAT PRET";
    resultat = cmdVal + " g/L"; // النتيجة اللي كتبتي فالتليفون
    updateOLED();

    // Envoi au Arduino (LIS) via Serial
    Serial.println("R|" + cmdId + "|" + cmdType + "|" + resultat);
    attenteActive(2000);

    // ── ETAPE 5 : Validation
    etape = 5; state = "VALIDE - LIS";
    updateOLED();
    attenteActive(3000);

    // ── Retour état initial
    etape = 1; state = "EN ATTENTE";
    echantillon = "--"; analyse = "--"; resultat = "--";
    updateOLED();
  }
}