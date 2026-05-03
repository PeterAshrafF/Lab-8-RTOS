#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h>

// ── Pin definitions ──────────────────────────────────────────────────────────
#define LM35_PIN      A0
#define MOTOR_IN1     5
#define MOTOR_IN2     6
#define BUTTON_PIN    2

// ── Threshold ────────────────────────────────────────────────────────────────
#define TEMP_THRESHOLD 25.0f   // °C

// ── Motor states ─────────────────────────────────────────────────────────────
typedef enum { MOTOR_STOP = 0, MOTOR_FORWARD } MotorState;

// ── RTOS handles ─────────────────────────────────────────────────────────────
static QueueHandle_t    xMotorQueue;   // passes MotorState from tasks to motor task
static SemaphoreHandle_t xUartMutex;  // protects Serial output

// ── Helper: drive motor pins ─────────────────────────────────────────────────
static void driveMotor(MotorState state)
{
    if (state == MOTOR_FORWARD) {
        digitalWrite(MOTOR_IN1, HIGH);
        digitalWrite(MOTOR_IN2, LOW);
    } else {
        digitalWrite(MOTOR_IN1, LOW);
        digitalWrite(MOTOR_IN2, LOW);
    }
}

// ── Helper: thread-safe UART print ───────────────────────────────────────────
static void uartPrint(const char *msg)
{
    xSemaphoreTake(xUartMutex, portMAX_DELAY);
    Serial.print(msg);
    xSemaphoreGive(xUartMutex);
}

// ── Task 1: Button (HIGH priority) ───────────────────────────────────────────
// Button open  (HIGH) → override: force MOTOR_STOP
// Button closed (LOW) → release override: return control to temperature task
void vButtonTask(void *pvParameters)
{
    MotorState state;
    bool lastOverride = false;

    for (;;)
    {
        bool buttonOpen = (digitalRead(BUTTON_PIN) == HIGH); // pullup: open=HIGH

        if (buttonOpen && !lastOverride) {
            // Transition to override
            state = MOTOR_STOP;
            xQueueOverwrite(xMotorQueue, &state);   // overwrite any pending state
            uartPrint("[BTN] Override ON  -> Motor STOP\r\n");
            lastOverride = true;

        } else if (!buttonOpen && lastOverride) {
            // Transition back to temp control — temp task will write next state
            uartPrint("[BTN] Override OFF -> Temp control\r\n");
            lastOverride = false;
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // 50 ms debounce / poll
    }
}

// ── Task 2: Temperature (LOW priority) ───────────────────────────────────────
// Reads LM35, decides motor state, sends to queue only when button is closed.
void vTempTask(void *pvParameters)
{
    char buf[64];
    MotorState state;

    for (;;)
    {
        // Only act when button is closed (not overriding)
        if (digitalRead(BUTTON_PIN) == LOW)
        {
            int   raw  = analogRead(LM35_PIN);
            float mV   = (raw / 1023.0f) * 5000.0f; // 5 V ref → mV
            float temp = mV / 10.0f;                 // LM35: 10 mV/°C

            state = (temp > TEMP_THRESHOLD) ? MOTOR_FORWARD : MOTOR_STOP;

            // Print temperature and intended state
            int tempInt  = (int)temp;
            int tempFrac = (int)((temp - tempInt) * 10);

            xSemaphoreTake(xUartMutex, portMAX_DELAY);
            snprintf(buf, sizeof(buf),
                    "[TEMP] %d.%d C -> Motor %s\r\n",
                    tempInt, tempFrac,
                    (state == MOTOR_FORWARD) ? "FORWARD" : "STOP");
            Serial.print(buf);
            xSemaphoreGive(xUartMutex);

            xQueueOverwrite(xMotorQueue, &state);
        }

        vTaskDelay(pdMS_TO_TICKS(500)); // sample every 500 ms
    }
}

// ── Task 3: Motor (LOW priority) ─────────────────────────────────────────────
// Waits for motor state on queue and drives the pins accordingly.
void vMotorTask(void *pvParameters)
{
    MotorState state;
    MotorState lastState = MOTOR_STOP;
    driveMotor(MOTOR_STOP);

    for (;;)
    {
        if (xQueueReceive(xMotorQueue, &state, portMAX_DELAY) == pdTRUE)
        {
            if (state != lastState) {
                driveMotor(state);
                uartPrint((state == MOTOR_FORWARD)
                          ? "[MOTOR] -> FORWARD\r\n"
                          : "[MOTOR] -> STOP\r\n");
                lastState = state;
            }
        }
    }
}

// ── setup ────────────────────────────────────────────────────────────────────
void setup()
{
    Serial.begin(9600);

    pinMode(MOTOR_IN1,  OUTPUT);
    pinMode(MOTOR_IN2,  OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    driveMotor(MOTOR_STOP);

    // Queue length = 1, overwrite semantics (xQueueOverwrite)
    xMotorQueue  = xQueueCreate(1, sizeof(MotorState));
    xUartMutex   = xSemaphoreCreateMutex();

    // Button task: higher priority (2) than temp/motor tasks (1)
    xTaskCreate(vButtonTask, "Button", 128, NULL, 2, NULL);
    xTaskCreate(vTempTask,   "Temp",   128, NULL, 1, NULL);
    xTaskCreate(vMotorTask,  "Motor",  128, NULL, 1, NULL);

    vTaskStartScheduler(); // never returns
}

void loop() { /* empty — FreeRTOS scheduler takes over */ }