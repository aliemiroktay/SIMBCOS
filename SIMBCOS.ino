#include <EEPROM.h>

unsigned long previousMillis = 0;
const long interval = 100;  // Interval for task scheduling
bool ledState = false;       // Controls LED blinking state
int currentTask = 1;         // 1 = Shell, 2 = Uptime

bool shellMode;
bool upMode;

// Shell task variables
bool shellPromptActive = true;

// Uptime task variables
unsigned long lastUptimeMillis = 0;
const long uptimeInterval = 1000;

// LED task variables
unsigned long lastLedMillis = 0;
const long ledInterval = 1000;

#define FAT_TABLE_START 0        // FAT table starts at EEPROM address 0
#define FAT_TABLE_SIZE  256      // FAT table size (each byte represents a cluster)
#define DATA_START      256      // Data area starts after the FAT table
#define DATA_SIZE       512      // Data area size

// Function to format FAT8 (sets up FAT table properly)
  void formatFAT8() {
      // Initialize FAT table by marking all clusters as FREE (0x00)
      for (int i = FAT_TABLE_START; i < FAT_TABLE_START + FAT_TABLE_SIZE; i++) {
          EEPROM.write(i, 0x00);  // 0x00 means free cluster
      }

      // Optional: Mark first cluster as system reserved (if needed)
      EEPROM.write(FAT_TABLE_START, 0xFF);  // Reserved for FAT8 system metadata

      Serial.println("FAT8 formatted: FAT table initialized.");
  }

// Function to write a file
  bool writeFile(uint8_t startCluster, const String &data) {
      if (startCluster >= FAT_TABLE_SIZE) return false; // Invalid cluster

      int dataAddress = DATA_START + (startCluster * 8); // Compute data location
      uint8_t dataSize = data.length();
      if (dataSize > 8) return false; // Ensure it fits in one cluster

    //   Check if space is free
      if (EEPROM.read(FAT_TABLE_START + startCluster) != 0x00) {
          return false; // Cluster is occupied
      }

      EEPROM.write(startCluster, 0xFF);

    //   Write data to EEPROM
      for (int i = 0; i < dataSize; i++) {
        EEPROM.write(dataAddress + i, data[i]);
      }

    //   Mark cluster as used in FAT table
      EEPROM.write(FAT_TABLE_START + startCluster, 0xFF); // 0xFF means end of file
      return true;
  }


  int findFreeCluster() {
      for (int i = 0; i < FAT_TABLE_SIZE; i++) {
          if (EEPROM.read(FAT_TABLE_START + i) == 0x00) {
              return i;
          }
      }
      return -1; // No free clusters
  }

  // Function to read a file (returns a String)
  String readFile(uint8_t startCluster, uint8_t dataSize) {
      if (startCluster >= FAT_TABLE_SIZE) return ""; // Invalid cluster

      int dataAddress = DATA_START + (startCluster * 8);
      if (EEPROM.read(FAT_TABLE_START + startCluster) == 0x00) {
          return ""; // Cluster is empty
      }

      String fileContents = "";
      for (int i = 0; i < dataSize; i++) {
          fileContents += (char)EEPROM.read(dataAddress + i);
      }
      return fileContents;
  }

  // Debugging function to print EEPROM data
  void printFATTable() {
      Serial.println("FAT8 Table:");
      for (int i = 0; i < FAT_TABLE_SIZE; i++) {
          Serial.print(EEPROM.read(FAT_TABLE_START + i), HEX);
          Serial.print(" ");
          if ((i + 1) % 16 == 0) Serial.println();
      }
      Serial.println();
  }

void taskShell() {
  if(shellMode){
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
            Serial.print(millis()/1000);
            Serial.println(" Seconds");
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
        else if (command.startsWith("task ")) {
            int task = command.substring(4).toInt();
            if (task >= 1 && task <= 2) {
                currentTask = task;
                Serial.print("Switched to task ");
                Serial.println(task);
            }
        }else if (command == "fquit"){
          Serial.println("!WARNING: fquit does not quit, it just goes into an empty loop.");
          while(1){

          }
        }else if(command == "stop"){
          digitalWrite(8, HIGH);
          digitalWrite(8, LOW);
          Serial.print("Stopping Ledloop . . . ");
          ledState = false;
          Serial.println("Done.");
          Serial.print("Turning led off . . . ");
          digitalWrite(LED_BUILTIN, LOW);
          Serial.println("Done.");
          Serial.print("Disabling shell . . . ");
          shellMode = false;
          Serial.println("Done.");
          Serial.print("Disabling uptime . . . ");
          upMode = false;
          Serial.println("Done.");
          Serial.println("-----=====> It is now safe to power off or reset via hardware. <=====-----");
          while(1){
            
          }
        }else if (command.startsWith("diskThing ")){
          String action = command.substring(10);
          if(action == "clean"){
            Serial.println("WARNING: formatting builtin EEPROM disk!");
            Serial.print("EEPROM disk size (bytes): ");
            Serial.println(EEPROM.length());
            for(int i = 0; i <= EEPROM.length(); i++){
              Serial.print("Updating byte ");
              Serial.print(i);
              Serial.print(" to 0 from ");
              Serial.println(EEPROM.read(i));
              EEPROM.update(i, 0);
            }
          }else if(action == "format"){
            Serial.println("!WARNING: this does NOT clean the disk. You should run with clean first, then this to make it properly work.");
            formatFAT8();
            Serial.println("Done.");
          }else if(action == "print"){
            printFATTable();
            Serial.println("Done.");
          }else if(action.startsWith("write ")){
            action = action.substring(6);
            writeFile(findFreeCluster(), action);
            Serial.println("Done writing.");
          }else if(action.startsWith("read ")){
            Serial.println(readFile(action.substring(5).toInt(), 8));
            Serial.println("Done reading.");
          }else{
            Serial.println("Use \"clean\" to set all bytes of EEPROM disk to 0. Use \"print\" to print the whole file system. Use \"format\" to format the EEPROM to FAT8. Use \"write\" to write a file. Use \"read\" with a number to read a file with the number.");
          }
        }
        else {
            Serial.println("Unknown command");
        }
    }
  }
}

void taskUptime() {
  if(upMode){
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
    shellMode = true;
    upMode = true;
    Serial.begin(115200);
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
