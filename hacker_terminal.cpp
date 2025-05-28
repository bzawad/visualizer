#include "hacker_terminal.h"
#include <GL/glew.h>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>

HackerTerminal::HackerTerminal() : audioAmplitude(0.0f), scrollPosition(0.0f), alertTimer(0.0f), hackingProgress(0.0f)
{
    std::random_device rd;
    rng.seed(rd());
    initializeContent();
}

HackerTerminal::~HackerTerminal() {}

void HackerTerminal::initialize(int width, int height)
{
    Visualizer::initialize(width, height);

    // Set background to black for terminal effect
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Initialize status bars
    statusBars = {
        {"ENCRYPT", 0.0f, 100.0f, {HEADER_COLOR[0], HEADER_COLOR[1], HEADER_COLOR[2]}},
        {"DEFENSE", 0.0f, 100.0f, {SUCCESS_COLOR[0], SUCCESS_COLOR[1], SUCCESS_COLOR[2]}},
        {"CPU", 0.0f, 100.0f, {WARNING_COLOR[0], WARNING_COLOR[1], WARNING_COLOR[2]}},
        {"THREAT", 0.0f, 100.0f, {ALERT_COLOR[0], ALERT_COLOR[1], ALERT_COLOR[2]}}};
}

void HackerTerminal::initializeContent()
{
    // More realistic code templates for hacking scenarios
    codeTemplates = {
        "import neural_defense as nd",
        "from quantum_encrypt import QuantumCipher",
        "class ThreatAnalyzer:",
        "    def __init__(self, threshold=0.85):",
        "        self.neural_net = nd.NeuralDefense()",
        "        self.cipher = QuantumCipher()",
        "        self.threat_level = 0",
        "    def analyze_packet(self, data):",
        "        signature = self.neural_net.extract_features(data)",
        "        if signature.threat_score > self.threshold:",
        "            self.escalate_countermeasures()",
        "            return True",
        "        return False",
        "def deploy_honeypot():",
        "    honeypot = VirtualSystem()",
        "    honeypot.mimic_vulnerable_service()",
        "    return honeypot.start_monitoring()",
        "encrypted_payload = cipher.encrypt(sensitive_data)",
        "secure_channel.transmit(encrypted_payload)",
        "if intrusion_detected:",
        "    firewall.adaptive_block(source_ip)",
        "    logger.log_incident(threat_vector)",
        "for node in compromised_nodes:",
        "    node.initiate_self_healing()",
        "    node.update_security_protocols()",
        "quantum_key = generate_quantum_key(256)",
        "tunnel = establish_secure_tunnel(quantum_key)",
        "result = neural_pattern_match(incoming_data)",
        "if result.confidence > 0.9:",
        "    trigger_emergency_protocol()",
        "subprocess.run(['iptables', '-A', 'INPUT', '-s', malicious_ip, '-j', 'DROP'])",
        "os.system(f'fail2ban-client set sshd banip {attacker_ip}')",
        "with open('/var/log/security.log', 'a') as f:",
        "    f.write(f'{timestamp}: BREACH_ATTEMPT from {source}\\n')",
        "def reverse_shell_detector(connection):",
        "    if connection.is_outbound and connection.port in SUSPICIOUS_PORTS:",
        "        return True",
        "crypto_hash = hashlib.sha256(payload).hexdigest()",
        "if crypto_hash in known_malware_signatures:",
        "    quarantine_file(payload)",
        "nmap_scan = subprocess.check_output(['nmap', '-sS', target_network])",
        "vulnerabilities = parse_nmap_output(nmap_scan)"};

    hackingTerms = {
        "neural_defense.py", "quantum_encrypt.py", "threat_analyzer.py",
        "honeypot_manager.py", "adaptive_firewall.py", "intrusion_detector.py",
        "secure_tunnel.py", "pattern_matcher.py", "emergency_protocol.py",
        "malware_scanner.py", "vulnerability_scanner.py", "crypto_utils.py"};

    systemMessages = {
        "Initializing quantum-resistant encryption protocols...",
        "Neural defense network: 47 nodes active, learning rate: 0.03",
        "Detected coordinated attack from botnet: 192.168.0.0/16",
        "Honeypot triggered: Attacker attempting SQL injection",
        "Adaptive firewall deployed: Blocking 23 malicious IPs",
        "Pattern recognition confidence: 94% - Threat signature matched",
        "Emergency protocol activated: Isolating compromised subnet",
        "Quantum tunnel established: 256-bit key exchange complete",
        "Self-healing initiated on nodes 7, 12, 15 - Estimated completion: 3min",
        "Vulnerability scan complete: 2 critical, 5 high, 12 medium threats",
        "Reverse shell attempt blocked from 203.0.113.42:4444",
        "Malware signature database updated: 47,293 new signatures",
        "Intrusion detection system: 99.7% uptime, 0 false positives",
        "Cryptographic hash verification: All system files intact",
        "Network segmentation active: DMZ isolated from internal network",
        "Behavioral analysis: Anomalous traffic pattern detected",
        "Zero-day exploit mitigation: Patching vulnerable service",
        "Threat intelligence feed: 156 new IOCs integrated",
        "Penetration test simulation: Red team exercise in progress",
        "Security orchestration: Automated response deployed"};

    alertMessages = {
        "CRITICAL: Advanced Persistent Threat detected in network segment 10.0.1.0/24",
        "WARNING: Brute force attack on SSH service - 247 failed attempts",
        "ALERT: Suspicious PowerShell execution detected on WORKSTATION-07",
        "URGENT: Lateral movement detected - Attacker pivoting through domain controller",
        "THREAT: Ransomware signature matched in email attachment",
        "BREACH: Unauthorized privilege escalation attempt on database server",
        "MALWARE: Trojan.Win32.Agent detected in memory dump analysis",
        "INTRUSION: Command and control communication to known bad domain",
        "EXPLOIT: Buffer overflow attempt targeting web application framework",
        "ATTACK: DNS tunneling detected - Possible data exfiltration in progress",
        "INCIDENT: Insider threat indicators - Unusual file access patterns",
        "COMPROMISE: Certificate authority private key potentially exposed",
        "VULNERABILITY: Unpatched RCE in Apache Struts framework",
        "PHISHING: Credential harvesting attempt via spoofed login portal",
        "BACKDOOR: Persistent access mechanism installed via DLL hijacking"};
}

void HackerTerminal::updateTerminal(float deltaTime)
{
    scrollPosition += deltaTime * SCROLL_SPEED * (1.0f + audioAmplitude);
    alertTimer += deltaTime;
    hackingProgress += deltaTime * 0.1f * (1.0f + audioAmplitude * 2.0f);

    // Generate new content based on audio intensity
    if (audioAmplitude > 0.2f && alertTimer > 0.5f)
    {
        generateCodeLine();
        alertTimer = 0.0f;
    }

    if (audioAmplitude > ALERT_THRESHOLD && alertTimer > 1.0f)
    {
        generateAlert();
        alertTimer = 0.0f;
    }

    if (alertTimer > 2.0f)
    {
        generateSystemMessage();
        alertTimer = 0.0f;
    }

    // Age and remove old lines
    for (auto it = terminalLines.begin(); it != terminalLines.end();)
    {
        it->age += deltaTime;
        if (it->age > 30.0f || terminalLines.size() > MAX_LINES)
        {
            it = terminalLines.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Age and remove old alerts
    for (auto it = alerts.begin(); it != alerts.end();)
    {
        it->timeRemaining -= deltaTime;
        if (it->timeRemaining <= 0.0f || alerts.size() > MAX_ALERTS)
        {
            it = alerts.erase(it);
        }
        else
        {
            ++it;
        }
    }

    updateStatusBars();
}

void HackerTerminal::generateCodeLine()
{
    std::uniform_int_distribution<> dis(0, codeTemplates.size() - 1);
    std::string line = codeTemplates[dis(rng)];

    // Add some terminal-style prefixes occasionally
    std::uniform_int_distribution<> prefixDis(0, 10);
    if (prefixDis(rng) < 3)
    {
        std::vector<std::string> prefixes = {
            ">>> ", "$ ", "root@defender:~# ", "[DEBUG] ", "[INFO] "};
        std::uniform_int_distribution<> prefixChoice(0, prefixes.size() - 1);
        line = prefixes[prefixChoice(rng)] + line;
    }

    TerminalLine termLine;
    termLine.text = line;
    termLine.color[0] = TEXT_COLOR[0];
    termLine.color[1] = TEXT_COLOR[1];
    termLine.color[2] = TEXT_COLOR[2];
    termLine.age = 0.0f;
    termLine.isAlert = false;
    termLine.intensity = audioAmplitude;

    terminalLines.push_back(termLine);
}

void HackerTerminal::generateSystemMessage()
{
    std::uniform_int_distribution<> dis(0, systemMessages.size() - 1);
    std::string message = "[" + getCurrentTime() + "] " + systemMessages[dis(rng)];

    TerminalLine termLine;
    termLine.text = message;
    termLine.color[0] = DIM_TEXT_COLOR[0];
    termLine.color[1] = DIM_TEXT_COLOR[1];
    termLine.color[2] = DIM_TEXT_COLOR[2];
    termLine.age = 0.0f;
    termLine.isAlert = false;
    termLine.intensity = audioAmplitude;

    terminalLines.push_back(termLine);
}

void HackerTerminal::generateAlert()
{
    std::uniform_int_distribution<> dis(0, alertMessages.size() - 1);
    std::uniform_int_distribution<> urgentDis(0, 1);

    SystemAlert alert;
    alert.message = alertMessages[dis(rng)];
    alert.timeRemaining = 5.0f + audioAmplitude * 5.0f;
    alert.isUrgent = urgentDis(rng) == 1 || audioAmplitude > 0.7f;

    if (alert.isUrgent)
    {
        alert.color[0] = ALERT_COLOR[0];
        alert.color[1] = ALERT_COLOR[1];
        alert.color[2] = ALERT_COLOR[2];
    }
    else
    {
        alert.color[0] = WARNING_COLOR[0];
        alert.color[1] = WARNING_COLOR[1];
        alert.color[2] = WARNING_COLOR[2];
    }

    alerts.push_back(alert);
}

void HackerTerminal::updateStatusBars()
{
    // Update status bars based on audio and time
    statusBars[0].value = std::min(100.0f, statusBars[0].value + audioAmplitude * 20.0f); // ENCRYPT
    statusBars[1].value = 80.0f + audioAmplitude * 20.0f;                                 // DEFENSE
    statusBars[2].value = 60.0f + audioAmplitude * 40.0f;                                 // CPU
    statusBars[3].value = audioAmplitude * 100.0f;                                        // THREAT
}

std::string HackerTerminal::getCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    ss << ":" << std::setfill('0') << std::setw(2) << ms.count() / 10;
    return ss.str();
}

void HackerTerminal::renderTerminalContent()
{
    float lineHeight = 0.04f;
    float startY = 0.8f;
    float charWidth = 0.012f;

    int lineIndex = 0;
    for (const auto &line : terminalLines)
    {
        float y = startY - lineIndex * lineHeight - scrollPosition * 0.02f;

        if (y < -1.0f || y > 1.0f)
            continue;

        // Render each character as a small rectangle to simulate text
        float x = -0.95f;
        for (size_t i = 0; i < line.text.length() && x < 0.95f; i++)
        {
            char c = line.text[i];

            // Skip spaces
            if (c == ' ')
            {
                x += charWidth;
                continue;
            }

            // Color intensity based on audio and character
            float intensity = 0.3f + line.intensity * 0.7f;
            if (c >= '0' && c <= '9')
                intensity *= 1.2f; // Numbers brighter
            if (c == '{' || c == '}' || c == '(' || c == ')')
                intensity *= 1.1f; // Brackets brighter

            glColor3f(line.color[0] * intensity,
                      line.color[1] * intensity,
                      line.color[2] * intensity);

            // Draw character as small filled rectangle
            float charHeight = lineHeight * 0.6f;
            glBegin(GL_QUADS);
            glVertex2f(x, y - charHeight * 0.5f);
            glVertex2f(x + charWidth * 0.8f, y - charHeight * 0.5f);
            glVertex2f(x + charWidth * 0.8f, y + charHeight * 0.5f);
            glVertex2f(x, y + charHeight * 0.5f);
            glEnd();

            // Add some random "pixel noise" for authentic terminal look
            if (audioAmplitude > 0.5f && (i + lineIndex) % 7 == 0)
            {
                glColor3f(line.color[0] * 0.3f, line.color[1] * 0.3f, line.color[2] * 0.3f);
                glBegin(GL_POINTS);
                glVertex2f(x + charWidth * 0.4f, y + charHeight * 0.2f);
                glEnd();
            }

            x += charWidth;
        }

        lineIndex++;
    }
}

void HackerTerminal::renderHeader()
{
    // Top status bar with cyberpunk styling
    glColor3f(HEADER_COLOR[0], HEADER_COLOR[1], HEADER_COLOR[2]);

    // Header background
    glBegin(GL_QUADS);
    glVertex2f(-1.0f, 0.92f);
    glVertex2f(1.0f, 0.92f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();

    // Header text simulation (CODER: SURVIVOR-03, timestamp, STATUS: SECURING)
    float headerY = 0.96f;
    float charWidth = 0.015f;

    // Left side: "CODER: SURVIVOR-03"
    std::string leftText = "CODER: SURVIVOR-03";
    float x = -0.95f;
    for (char c : leftText)
    {
        if (c != ' ')
        {
            glBegin(GL_QUADS);
            glVertex2f(x, headerY - 0.015f);
            glVertex2f(x + charWidth * 0.7f, headerY - 0.015f);
            glVertex2f(x + charWidth * 0.7f, headerY + 0.015f);
            glVertex2f(x, headerY + 0.015f);
            glEnd();
        }
        x += charWidth;
    }

    // Center: timestamp
    std::string timeText = getCurrentTime() + " | 28-05-2025";
    x = -0.15f;
    for (char c : timeText)
    {
        if (c != ' ')
        {
            glBegin(GL_QUADS);
            glVertex2f(x, headerY - 0.015f);
            glVertex2f(x + charWidth * 0.7f, headerY - 0.015f);
            glVertex2f(x + charWidth * 0.7f, headerY + 0.015f);
            glVertex2f(x, headerY + 0.015f);
            glEnd();
        }
        x += charWidth;
    }

    // Right side: "STATUS: SECURING"
    glColor3f(SUCCESS_COLOR[0], SUCCESS_COLOR[1], SUCCESS_COLOR[2]);
    std::string rightText = "STATUS: SECURING";
    x = 0.5f;
    for (char c : rightText)
    {
        if (c != ' ')
        {
            glBegin(GL_QUADS);
            glVertex2f(x, headerY - 0.015f);
            glVertex2f(x + charWidth * 0.7f, headerY - 0.015f);
            glVertex2f(x + charWidth * 0.7f, headerY + 0.015f);
            glVertex2f(x, headerY + 0.015f);
            glEnd();
        }
        x += charWidth;
    }

    // Tab indicators
    glColor3f(HEADER_COLOR[0] * 0.8f, HEADER_COLOR[1] * 0.8f, HEADER_COLOR[2] * 0.8f);
    std::vector<std::string> tabs = {"countermeasure.js", "survival_protocol.ts", "neural_defense.py"};
    float tabY = 0.87f;
    x = -0.95f;

    for (const auto &tab : tabs)
    {
        // Tab background
        glBegin(GL_QUADS);
        glVertex2f(x, tabY - 0.02f);
        glVertex2f(x + tab.length() * 0.012f, tabY - 0.02f);
        glVertex2f(x + tab.length() * 0.012f, tabY + 0.02f);
        glVertex2f(x, tabY + 0.02f);
        glEnd();

        // Tab text
        float tabX = x + 0.01f;
        for (char c : tab)
        {
            if (c != ' ')
            {
                glBegin(GL_QUADS);
                glVertex2f(tabX, tabY - 0.01f);
                glVertex2f(tabX + 0.008f, tabY - 0.01f);
                glVertex2f(tabX + 0.008f, tabY + 0.01f);
                glVertex2f(tabX, tabY + 0.01f);
                glEnd();
            }
            tabX += 0.01f;
        }

        x += tab.length() * 0.012f + 0.05f;
    }
}

void HackerTerminal::renderAlerts()
{
    // Command console area (right side)
    glColor3f(0.0f, 0.2f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.25f, 0.85f);
    glVertex2f(0.98f, 0.85f);
    glVertex2f(0.98f, -0.4f);
    glVertex2f(0.25f, -0.4f);
    glEnd();

    // Console header
    glColor3f(HEADER_COLOR[0], HEADER_COLOR[1], HEADER_COLOR[2]);
    std::string consoleHeader = "COMMAND CONSOLE";
    float x = 0.3f;
    float y = 0.8f;
    for (char c : consoleHeader)
    {
        if (c != ' ')
        {
            glBegin(GL_QUADS);
            glVertex2f(x, y - 0.01f);
            glVertex2f(x + 0.01f, y - 0.01f);
            glVertex2f(x + 0.01f, y + 0.01f);
            glVertex2f(x, y + 0.01f);
            glEnd();
        }
        x += 0.012f;
    }

    // Threat level indicator
    glColor3f(ALERT_COLOR[0], ALERT_COLOR[1], ALERT_COLOR[2]);
    std::string threatText = "THREAT LEVEL: ELEVATED";
    x = 0.7f;
    for (char c : threatText)
    {
        if (c != ' ')
        {
            glBegin(GL_QUADS);
            glVertex2f(x, y - 0.01f);
            glVertex2f(x + 0.008f, y - 0.01f);
            glVertex2f(x + 0.008f, y + 0.01f);
            glVertex2f(x, y + 0.01f);
            glEnd();
        }
        x += 0.01f;
    }

    // Alert messages
    float alertY = 0.7f;
    int alertIndex = 0;

    for (const auto &alert : alerts)
    {
        float y = alertY - alertIndex * 0.06f;
        if (y < -0.3f)
            break;

        // Timestamp
        std::string timestamp = "[" + getCurrentTime() + "]";
        float x = 0.3f;

        glColor3f(DIM_TEXT_COLOR[0], DIM_TEXT_COLOR[1], DIM_TEXT_COLOR[2]);
        for (char c : timestamp)
        {
            if (c != ' ')
            {
                glBegin(GL_QUADS);
                glVertex2f(x, y - 0.008f);
                glVertex2f(x + 0.008f, y - 0.008f);
                glVertex2f(x + 0.008f, y + 0.008f);
                glVertex2f(x, y + 0.008f);
                glEnd();
            }
            x += 0.009f;
        }

        // Alert message
        x += 0.02f;
        float intensity = alert.isUrgent ? (0.5f + 0.5f * std::sin(alertTimer * 8.0f)) : 1.0f;
        glColor3f(alert.color[0] * intensity, alert.color[1] * intensity, alert.color[2] * intensity);

        for (size_t i = 0; i < alert.message.length() && x < 0.95f; i++)
        {
            char c = alert.message[i];
            if (c != ' ')
            {
                glBegin(GL_QUADS);
                glVertex2f(x, y - 0.008f);
                glVertex2f(x + 0.008f, y - 0.008f);
                glVertex2f(x + 0.008f, y + 0.008f);
                glVertex2f(x, y + 0.008f);
                glEnd();
            }
            x += 0.009f;
        }

        alertIndex++;
    }
}

void HackerTerminal::renderStatusBars()
{
    // System monitor area (bottom right)
    glColor3f(0.0f, 0.15f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.25f, -0.45f);
    glVertex2f(0.98f, -0.45f);
    glVertex2f(0.98f, -0.98f);
    glVertex2f(0.25f, -0.98f);
    glEnd();

    // System monitor header
    glColor3f(HEADER_COLOR[0], HEADER_COLOR[1], HEADER_COLOR[2]);
    std::string sysHeader = "SYSTEM MONITOR";
    float x = 0.3f;
    float y = -0.5f;
    for (char c : sysHeader)
    {
        if (c != ' ')
        {
            glBegin(GL_QUADS);
            glVertex2f(x, y - 0.01f);
            glVertex2f(x + 0.01f, y - 0.01f);
            glVertex2f(x + 0.01f, y + 0.01f);
            glVertex2f(x, y + 0.01f);
            glEnd();
        }
        x += 0.012f;
    }

    // CPU percentage
    std::string cpuText = "CPU: 70%";
    x = 0.85f;
    for (char c : cpuText)
    {
        if (c != ' ')
        {
            glBegin(GL_QUADS);
            glVertex2f(x, y - 0.01f);
            glVertex2f(x + 0.008f, y - 0.01f);
            glVertex2f(x + 0.008f, y + 0.01f);
            glVertex2f(x, y + 0.01f);
            glEnd();
        }
        x += 0.01f;
    }

    // Status bars with labels
    std::vector<std::string> labels = {"QUANTUM-ENCRYPTED", "DEFENSE", "CPU", "THREAT"};
    float barY = -0.6f;

    for (size_t i = 0; i < statusBars.size(); i++)
    {
        float y = barY - i * 0.08f;

        // Label
        glColor3f(DIM_TEXT_COLOR[0], DIM_TEXT_COLOR[1], DIM_TEXT_COLOR[2]);
        x = 0.3f;
        for (char c : labels[i])
        {
            if (c != ' ')
            {
                glBegin(GL_QUADS);
                glVertex2f(x, y - 0.008f);
                glVertex2f(x + 0.007f, y - 0.008f);
                glVertex2f(x + 0.007f, y + 0.008f);
                glVertex2f(x, y + 0.008f);
                glEnd();
            }
            x += 0.008f;
        }

        // Progress bar background
        float barX = 0.65f;
        float barWidth = 0.25f;
        float barHeight = 0.02f;

        glColor3f(0.1f, 0.1f, 0.1f);
        glBegin(GL_QUADS);
        glVertex2f(barX, y - barHeight);
        glVertex2f(barX + barWidth, y - barHeight);
        glVertex2f(barX + barWidth, y + barHeight);
        glVertex2f(barX, y + barHeight);
        glEnd();

        // Progress bar fill
        float fillWidth = barWidth * (statusBars[i].value / statusBars[i].maxValue);
        glColor3fv(statusBars[i].color);

        // Add pulsing effect based on audio
        float pulse = 1.0f + audioAmplitude * 0.3f * std::sin(alertTimer * 5.0f);
        glColor3f(statusBars[i].color[0] * pulse,
                  statusBars[i].color[1] * pulse,
                  statusBars[i].color[2] * pulse);

        glBegin(GL_QUADS);
        glVertex2f(barX, y - barHeight);
        glVertex2f(barX + fillWidth, y - barHeight);
        glVertex2f(barX + fillWidth, y + barHeight);
        glVertex2f(barX, y + barHeight);
        glEnd();

        // Percentage text
        std::string percentText = std::to_string(static_cast<int>(statusBars[i].value)) + "%";
        x = barX + barWidth + 0.02f;
        glColor3f(statusBars[i].color[0], statusBars[i].color[1], statusBars[i].color[2]);
        for (char c : percentText)
        {
            if (c != ' ')
            {
                glBegin(GL_QUADS);
                glVertex2f(x, y - 0.008f);
                glVertex2f(x + 0.007f, y - 0.008f);
                glVertex2f(x + 0.007f, y + 0.008f);
                glVertex2f(x, y + 0.008f);
                glEnd();
            }
            x += 0.008f;
        }
    }
}

void HackerTerminal::renderScanlines()
{
    // Matrix-style digital rain effect
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Subtle scanlines
    glColor4f(0.0f, 1.0f, 0.0f, 0.05f);
    glBegin(GL_LINES);
    for (float y = -1.0f; y <= 1.0f; y += 0.004f)
    {
        glVertex2f(-1.0f, y);
        glVertex2f(1.0f, y);
    }
    glEnd();

    // Random "digital noise" pixels
    if (audioAmplitude > 0.3f)
    {
        glColor4f(0.0f, 1.0f, 0.0f, 0.3f);
        glBegin(GL_POINTS);
        for (int i = 0; i < static_cast<int>(audioAmplitude * 50); i++)
        {
            float x = -1.0f + (rand() % 1000) / 500.0f;
            float y = -1.0f + (rand() % 1000) / 500.0f;
            glVertex2f(x, y);
        }
        glEnd();
    }

    glDisable(GL_BLEND);
}

float HackerTerminal::calculateAudioAmplitude(const std::vector<float> &audioData, size_t position)
{
    const size_t windowSize = 1024;
    size_t end = std::min(position + windowSize, audioData.size());

    float sum = 0.0f;
    for (size_t i = position; i < end; i++)
    {
        sum += std::fabs(audioData[i]);
    }

    float average = 0.0f;
    size_t count = end - position;
    if (count > 0)
        average = sum / static_cast<float>(count);

    return std::min(1.0f, average * 8.0f); // Amplify for dramatic effect
}

std::string HackerTerminal::generateRandomHex(int length)
{
    const char *hexChars = "0123456789ABCDEF";
    std::string result;
    std::uniform_int_distribution<> dis(0, 15);

    for (int i = 0; i < length; i++)
    {
        result += hexChars[dis(rng)];
    }
    return result;
}

std::string HackerTerminal::generateRandomIP()
{
    std::uniform_int_distribution<> dis(1, 255);
    return std::to_string(dis(rng)) + "." +
           std::to_string(dis(rng)) + "." +
           std::to_string(dis(rng)) + "." +
           std::to_string(dis(rng));
}

std::string HackerTerminal::generateRandomHash()
{
    return generateRandomHex(8) + "-" + generateRandomHex(4) + "-" +
           generateRandomHex(4) + "-" + generateRandomHex(12);
}

void HackerTerminal::renderFrame(const std::vector<float> &audioData,
                                 double * /* fftInputBuffer */,
                                 fftw_complex * /* fftOutputBuffer */,
                                 fftw_plan & /* fftPlan */,
                                 float timeSeconds)
{
    size_t sampleIndex = static_cast<size_t>(timeSeconds * 44100);
    audioAmplitude = calculateAudioAmplitude(audioData, sampleIndex);

    glClear(GL_COLOR_BUFFER_BIT);

    // Set up 2D rendering
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    updateTerminal(1.0f / 60.0f);

    renderHeader();
    renderTerminalContent();
    renderAlerts();
    renderStatusBars();
    renderScanlines();
}

void HackerTerminal::renderLiveFrame(const std::vector<float> &audioData,
                                     double * /* fftInputBuffer */,
                                     fftw_complex * /* fftOutputBuffer */,
                                     fftw_plan & /* fftPlan */,
                                     size_t currentPosition)
{
    audioAmplitude = calculateAudioAmplitude(audioData, currentPosition);

    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    updateTerminal(1.0f / 60.0f);

    renderHeader();
    renderTerminalContent();
    renderAlerts();
    renderStatusBars();
    renderScanlines();
}