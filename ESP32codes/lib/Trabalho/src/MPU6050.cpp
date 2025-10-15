#include "Trabalho.hpp"

class MPU6050 {
private:
    int id, acc, gyro, filter;
    MPU_read read;
    Adafruit_MPU6050 mpu;
public:
    typedef struct mpu_read_t{
        int x, y, z, gx, gy, gz, temp;
    } MPU_read;

    
    MPU6050(int id, int acc, int gyro, int filter, int addr_pin, int sda, int scl);
    ~MPU6050();
    void setParamsMPU(int acc_range, int gyro_range, int filter_bandwidth);
    MPU_read getValues();
};


MPU6050::MPU6050(int id, int acc, int gyro, int filter, int addr_pin, int sda = PIN_SDA, int scl = PIN_SCL) {
    Wire.begin(sda, scl);
    int addr;
    if (addr_pin == -1) {
        addr = 0x68;
    } else {
        pinMode(addr_pin, OUTPUT);
        digitalWrite(addr_pin, HIGH);
        addr = 0x69;
    }
    if (!this.mpu.begin(addr)) {
        Serial.println("Falha para encontrar MPU6050.");
        Serial.println("Reiniciando...");
        delay(10);
        ESP.restart();
    }
    Serial.println("MPU6050 Encontrado!");
}

MPU6050::~MPU6050() {

}

MPU6050::setParamsMPU(int acc_range, int gyro_range, int filter_bandwidth) {
    mpu.setAccelerometerRange(acc_range);
    Serial.print("Alcance definido para ");
    switch (mpu.getAccelerometerRange()) {
        case MPU6050_RANGE_2_G:
            Serial.println("+-2G");
            break;
        case MPU6050_RANGE_4_G:
            Serial.println("+-4G");
            break;
        case MPU6050_RANGE_8_G:
            Serial.println("+-8G");
            break;
        case MPU6050_RANGE_16_G:
            Serial.println("+-16G");
            break;
    }

    mpu.setGyroRange(gyro_range);
    Serial.print("Alcance do gyro definido para ");
    switch (mpu.getGyroRange()) {
        case MPU6050_RANGE_250_DEG:
            Serial.println("+- 250 deg/s");
            break;
        case MPU6050_RANGE_500_DEG:
            Serial.println("+- 500 deg/s");
            break;
        case MPU6050_RANGE_1000_DEG:
            Serial.println("+- 1000 deg/s");
            break;
        case MPU6050_RANGE_2000_DEG:
            Serial.println("+- 2000 deg/s");
            break;
    }

    mpu.setFilterBandwidth(filter_bandwidth);
    Serial.print("Filtro de banda definido para ");
    switch (mpu.getFilterBandwidth()) {
        case MPU6050_BAND_260_HZ:
            Serial.println("260 Hz");
            break;
        case MPU6050_BAND_184_HZ:
            Serial.println("184 Hz");
            break;
        case MPU6050_BAND_94_HZ:
            Serial.println("94 Hz");
            break;
        case MPU6050_BAND_44_HZ:
            Serial.println("44 Hz");
            break;
        case MPU6050_BAND_21_HZ:
            Serial.println("21 Hz");
            break;
        case MPU6050_BAND_10_HZ:
            Serial.println("10 Hz");
            break;
        case MPU6050_BAND_5_HZ:
            Serial.println("5 Hz");
            break;
    }
}


MPU6050::getValues(){
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    MPU_read read;
    
    read.x = a.acceleration.x;
    read.y = a.acceleration.y;
    read.z = a.acceleration.z;

    read.gx = g.gyro.x;
    read.gy = g.gyro.y;
    read.gz = g.gyro.z;

    read.temp = temp.temperature;
    return read;
}