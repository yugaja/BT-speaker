#include <Arduino.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include "CST820.h"

// ==========================================================
// ============ DEBUG MACRO CONTROL =========================
// ==========================================================
#define CST820_DEBUG 0  // <-- Set to 0 to disable all debug output

#if CST820_DEBUG
  #define DEBUG_BEGIN(baud)        Serial.begin(baud)
  #define DEBUG_PRINT(x)           Serial.print(x)
  #define DEBUG_PRINTLN(x)         Serial.println(x)
  #define DEBUG_PRINTF(...)        Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_BEGIN(baud)
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif
// ==========================================================

CST820::CST820(int8_t sda_pin, int8_t scl_pin, int8_t rst_pin, int8_t int_pin)
{
    _sda = sda_pin;
    _scl = scl_pin;
    _rst = rst_pin;
    _int = int_pin;
}

void CST820::begin(void)
{
 //   DEBUG_BEGIN(115200);
    DEBUG_PRINTLN("[CST820] Begin initialization...");

    // Initialize I2C
    if (_sda != -1 && _scl != -1)
    {
        DEBUG_PRINTF("[CST820] Using custom SDA=%d, SCL=%d\n", _sda, _scl);
        Wire.begin(_sda, _scl);
        Wire.setClock(50000);  // 50 kHz – sporije, stabilnije
    }
    else
    {
        DEBUG_PRINTLN("[CST820] Using default I2C pins");
        Wire.begin();
    }

    // Int Pin Configuration
    if (_int != -1)
    {
        DEBUG_PRINTF("[CST820] Config INT pin=%d\n", _int);
        pinMode(_int, OUTPUT);
        digitalWrite(_int, HIGH);
        delay(1);
        digitalWrite(_int, LOW);
        delay(1);
    }

    // Reset Pin Configuration
    if (_rst != -1)
    {
        DEBUG_PRINTF("[CST820] Config RESET pin=%d\n", _rst);
        pinMode(_rst, OUTPUT);
        digitalWrite(_rst, LOW);
        delay(10);
        digitalWrite(_rst, HIGH);
        delay(300);
    }

    // Initialize Touch
    DEBUG_PRINTLN("[CST820] Writing init command (0xFE -> 0xFF)");
    i2c_write(0xFE, 0XFF);
    DEBUG_PRINTLN("[CST820] Initialization complete");
}


/**
 * @brief Mirror X and/or Y axes
 */
void CST820::setMirrorXY(bool mirrorX, bool mirrorY)
{
    _mirrorX = mirrorX;
    _mirrorY = mirrorY;
    DEBUG_PRINT("[CST820] Mirror set: X=");
    DEBUG_PRINT(_mirrorX);
    DEBUG_PRINT(" Y=");
    DEBUG_PRINTLN(_mirrorY);
}

/**
 * @brief Swap X and Y axes
 */
void CST820::setSwapXY(bool swap)
{
    _swapXY = swap;
    DEBUG_PRINT("[CST820] SwapXY set to ");
    DEBUG_PRINTLN(_swapXY);
}

/**
 * @brief Set maximum valid touch coordinates
 */
void CST820::setMaxCoordinates(uint16_t maxX, uint16_t maxY)
{
    _maxX = maxX;
    _maxY = maxY;
    DEBUG_PRINT("[CST820] Max coords set: ");
    DEBUG_PRINT(_maxX);
    DEBUG_PRINT("x");
    DEBUG_PRINTLN(_maxY);
}

bool CST820::getTouch(uint16_t *x, uint16_t *y, uint8_t *gesture)
{
    bool FingerIndex = false;
    FingerIndex = (bool)i2c_read(0x02);
    uint16_t rawX;  // primer
    uint16_t rawY;  // primer

    *gesture = i2c_read(0x01);
    // if (!(*gesture == SlideUp || *gesture == SlideDown))
    // {
    //     *gesture = None;
    // }

    uint8_t data[4];
    i2c_read_continuous(0x03, data, 4);

    rawX = ((data[0] & 0x0f) << 8) | data[1];
    rawY = ((data[2] & 0x0f) << 8) | data[3];

      // Apply axis swap
    uint16_t procX = _swapXY ? rawY : rawX;
    uint16_t procY = _swapXY ? rawX : rawY;

    // Apply mirror (flip)
    if (_mirrorX) procX = _maxX - procX;
    if (_mirrorY) procY = _maxY - procY;

    *x = procX;
    *y = procY;

    DEBUG_PRINTF("[CST820] Touch=%d X=%u Y=%u Gesture=%u\n", FingerIndex, *x, *y, *gesture);
    return FingerIndex;
}

uint8_t CST820::i2c_read(uint8_t addr)
{
    uint8_t rdData = 0;
    uint8_t rdDataCount = 0;

    DEBUG_PRINTF("[I2C_READ] Addr=0x%02X -> ", addr);
    do
    {
        Wire.beginTransmission(I2C_ADDR_CST820);
        Wire.write(addr);
        uint8_t txStatus = Wire.endTransmission(false); // Restart
        if (txStatus != 0)
        {
            DEBUG_PRINTF("endTransmission failed=%d\n", txStatus);
        }

        rdDataCount = Wire.requestFrom((uint16_t)I2C_ADDR_CST820, (uint8_t)1);
        if (rdDataCount == 0)
        {
            DEBUG_PRINTLN("No data, retrying...");
            delay(10);
        }
    } while (rdDataCount == 0);

    while (Wire.available())
    {
        rdData = Wire.read();
    }

    DEBUG_PRINTF("Read=0x%02X\n", rdData);
    return rdData;
}

uint8_t CST820::i2c_read_continuous(uint8_t addr, uint8_t *data, uint32_t length)
{
    DEBUG_PRINTF("[I2C_READ_CONT] Addr=0x%02X Len=%lu\n", addr, (unsigned long)length);

    Wire.beginTransmission(I2C_ADDR_CST820);
    Wire.write(addr);
    uint8_t txStatus = Wire.endTransmission(true);
    if (txStatus != 0)
    {
        DEBUG_PRINTF("endTransmission failed=%d\n", txStatus);
        return -1;
    }

    uint8_t reqCount = Wire.requestFrom((uint16_t)I2C_ADDR_CST820, (uint8_t)length);
    if (reqCount != length)
    {
        DEBUG_PRINTF("requestFrom failed (got %d / expected %lu)\n", reqCount, (unsigned long)length);
        return -1;
    }

    for (uint32_t i = 0; i < length; i++)
    {
        data[i] = Wire.read();
        DEBUG_PRINTF("  Data[%lu] = 0x%02X\n", (unsigned long)i, data[i]);
    }

    return 0;
}

void CST820::i2c_write(uint8_t addr, uint8_t data)
{
    DEBUG_PRINTF("[I2C_WRITE] Addr=0x%02X Data=0x%02X\n", addr, data);
    Wire.beginTransmission(I2C_ADDR_CST820);
    Wire.write(addr);
    Wire.write(data);
    uint8_t txStatus = Wire.endTransmission();
    if (txStatus != 0)
    {
        DEBUG_PRINTF("endTransmission failed=%d\n", txStatus);
    }
}

uint8_t CST820::i2c_write_continuous(uint8_t addr, const uint8_t *data, uint32_t length)
{
    DEBUG_PRINTF("[I2C_WRITE_CONT] Addr=0x%02X Len=%lu\n", addr, (unsigned long)length);

    Wire.beginTransmission(I2C_ADDR_CST820);
    Wire.write(addr);
    for (uint32_t i = 0; i < length; i++)
    {
        Wire.write(data[i]);
        DEBUG_PRINTF("  Write[%lu] = 0x%02X\n", (unsigned long)i, data[i]);
    }

    uint8_t txStatus = Wire.endTransmission(true);
    if (txStatus != 0)
    {
        DEBUG_PRINTF("endTransmission failed=%d\n", txStatus);
        return -1;
    }

    return 0;
}
