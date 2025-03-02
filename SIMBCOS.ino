unsigned long previousMillis = 0;
const long interval = 100;  // Interval for task scheduling
bool ledState = false;       // Controls LED blinking state
int currentTask = 1;         // 1 = Shell, 2 = Uptime

// Shell task variables
bool shellPromptActive = true;

// Uptime task variables
unsigned long lastUptimeMillis = 0;
const long uptimeInterval = 1000;

// LED task variables
unsigned long lastLedMillis = 0;
const long ledInterval = 1000;

void taskShell() {
    if (shellPromptActive) {
        Serial.print("sh> ");
        shellPromptActive = false;
    }
    
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        shellPromptActive = true;  // Reset prompt for next command

        if (command.startsWith("led ")) {
            String state = command.substring(4);
            ledState = false;  // Stop blinking
            
            if (state == "on") {
                digitalWrite(LED_BUILTIN, HIGH);
                Serial.println("LED turned on");
            } else if (state == "off") {
                digitalWrite(LED_BUILTIN, LOW);
                Serial.println("LED turned off");
            } else {
                Serial.println("Invalid LED command. Use 'on' or 'off'.");
            }
        }
        else if (command == "status") {
            Serial.println("System status: OK");
        }
        else if (command == "exit") {
            currentTask = 2;  // Switch to uptime
            Serial.println("Entering background mode");
        }
        else if (command == "uptime") {
            currentTask = 2;
            Serial.println("Switching to uptime monitoring");
        }
        else if (command.startsWith("ledloop ")) {
            String action = command.substring(8);
            if (action == "enable") {
                ledState = true;
                Serial.println("LED blinking enabled");
            } else if (action == "disable") {
                ledState = false;
                digitalWrite(LED_BUILTIN, LOW);  // Turn off LED when disabling
                Serial.println("LED blinking disabled");
            } else {
                Serial.println("Invalid argument for 'ledloop'. Use 'enable' or 'disable'.");
            }
        }
        else if (command == "reboot") {
            asm volatile ("jmp 0");
        }
        else if (command.startsWith("task")) {
            int task = command.substring(4).toInt();
            if (task >= 1 && task <= 2) {
                currentTask = task;
                Serial.print("Switched to task ");
                Serial.println(task);
            }
        }
        else {
            Serial.println("Unknown command");
        }
    }
}

void taskUptime() {
    unsigned long currentMillis = millis();
    
    // Update uptime display
    if (currentMillis - lastUptimeMillis >= uptimeInterval) {
        lastUptimeMillis = currentMillis;
        Serial.print("Uptime: ");
        Serial.print(currentMillis / 1000);
        Serial.println(" seconds");
    }
    
    // Check for return command
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        if (input == "task1") {
            currentTask = 1;
            Serial.println("Returning to shell");
        }
    }
}

void taskLED() {
    if (ledState) {
        unsigned long currentMillis = millis();
        if (currentMillis - lastLedMillis >= ledInterval) {
            lastLedMillis = currentMillis;
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        }
    }
}

void setup() {
    Serial.begin(2000000);
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.println("System ready - Type 'task1' for shell, 'task2' for uptime");
}

void loop() {
    unsigned long currentMillis = millis();
    
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        
        // Run active foreground task
        switch (currentTask) {
            case 1: taskShell(); break;
            case 2: taskUptime(); break;
        }
        
        // Always run background LED task
        taskLED();
    }
    
    yield();  // Important for ESP-based boards
}
