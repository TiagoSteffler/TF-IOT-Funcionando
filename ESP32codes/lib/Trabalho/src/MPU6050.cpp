#include <Trabalho.hpp>

/// @brief Classe do acelerometro MPU6050
class MPU6050 {
    private:
        // pino e variaveis de controle
        Adafruit_MPU6050 mpu;
        int id, addr_pin, addr;
        MPU_read read;
        enum AccRange {A2G = MPU6050_RANGE_2_G, A4G = MPU6050_RANGE_4_G, A8G = MPU6050_RANGE_8_G, A16G = MPU6050_RANGE_16_G} acc_range = A8G;
        enum GyroRange {G250DPS = MPU6050_RANGE_250_DEG, G500DPS = MPU6050_RANGE_500_DEG, G1000DPS = MPU6050_RANGE_1000_DEG, G2000DPS = MPU6050_RANGE_2000_DEG} gyro_range = G500DPS;
        enum FilterBandwidth {B260HZ = MPU6050_BAND_260_HZ, B184HZ = MPU6050_BAND_184_HZ, B94HZ = MPU6050_BAND_94_HZ, B44HZ = MPU6050_BAND_44_HZ, B21HZ = MPU6050_BAND_21_HZ, B10HZ = MPU6050_BAND_10_HZ, B5HZ = MPU6050_BAND_5_HZ} filter_bandwidth = B5HZ;

    public:
        // contrutores/destrutores
        MPU6050(int id, int addr_pin = -1);
        ~MPU6050() {};

        // metodos publicos
        void setParamsMPU(AccRange acc_range, GyroRange gyro_range, FilterBandwidth filter_bandwidth);
        MPU_read getValues();
};

/// @brief Construtor do acelerometro MPU6050
/// @param id Identificador do sensor
/// @param sda Pino SDA
/// @param scl Pino SCL
/// @param addr_pin Pino de endereco (endereco padrao: 0x68)
MPU6050::MPU6050(int id, int addr_pin = -1) {
    this->id = id;
    this->addr_pin = addr_pin;

    if (addr_pin == -1) this->addr = 0x68;
    else {
        pinMode(addr_pin, OUTPUT);
        digitalWrite(addr_pin, HIGH);
        this->addr = 0x69;
    }
    
    if (!this->mpu.begin(this->addr)) Serial.println("Falha para encontrar MPU6050");
    else if (DEBUGSENS) {
        Serial.println("[MPU6050 id");
        Serial.print(this->id);
        Serial.print("] conectado com sucesso > addr: ");
        Serial.println(this->addr, HEX);
    }
}


/// @brief Define os parametros do MPU6050
/// @param acc_range Intervalo de leitura do acelerometro
/// @param gyro_range Intervalo de leitura do giroscopio
/// @param filter_bandwidth Largura de banda do filtro passa-baixa
void MPU6050::setParamsMPU(AccRange acc_range, GyroRange gyro_range, FilterBandwidth filter_bandwidth) {
    this->acc_range = acc_range;
    this->gyro_range = gyro_range;
    this->filter_bandwidth = filter_bandwidth;
    
    if (DEBUGCOMM) return;
    mpu.setAccelerometerRange(static_cast<decltype(mpu.getAccelerometerRange())>(this->acc_range));
    mpu.setGyroRange(static_cast<decltype(mpu.getGyroRange())>(gyro_range));
    mpu.setFilterBandwidth(static_cast<decltype(mpu.getFilterBandwidth())>(filter_bandwidth));

    if (DEBUGSENS) {
        Serial.print("[MPU6050 id ");
        Serial.print(this->id);
        Serial.print("] ");

        Serial.print("parametros > alcance: ");
        if (mpu.getAccelerometerRange() == MPU6050_RANGE_2_G)       Serial.print("+-2G\t");
        else if (mpu.getAccelerometerRange() == MPU6050_RANGE_4_G)  Serial.print("+-4G\t");
        else if (mpu.getAccelerometerRange() == MPU6050_RANGE_8_G)  Serial.print("+-8G\t");
        else if (mpu.getAccelerometerRange() == MPU6050_RANGE_16_G) Serial.print("+-16G\t");

        Serial.print("gyro: ");
        if (mpu.getGyroRange() == MPU6050_RANGE_250_DEG)            Serial.print("+- 250 deg/s \t");
        else if (mpu.getGyroRange() == MPU6050_RANGE_500_DEG)       Serial.print("+- 500 deg/s \t");
        else if (mpu.getGyroRange() == MPU6050_RANGE_1000_DEG)      Serial.print("+- 1000 deg/s\t");
        else if (mpu.getGyroRange() == MPU6050_RANGE_2000_DEG)      Serial.print("+- 2000 deg/s\t");

        Serial.print("filtro: ");
        if (mpu.getFilterBandwidth() == MPU6050_BAND_260_HZ)        Serial.println("260 Hz");
        else if (mpu.getFilterBandwidth() == MPU6050_BAND_184_HZ)   Serial.println("184 Hz");
        else if (mpu.getFilterBandwidth() == MPU6050_BAND_94_HZ)    Serial.println("94 Hz ");
        else if (mpu.getFilterBandwidth() == MPU6050_BAND_44_HZ)    Serial.println("44 Hz ");
        else if (mpu.getFilterBandwidth() == MPU6050_BAND_21_HZ)    Serial.println("21 Hz ");
        else if (mpu.getFilterBandwidth() == MPU6050_BAND_10_HZ)    Serial.println("10 Hz ");
        else if (mpu.getFilterBandwidth() == MPU6050_BAND_5_HZ)     Serial.println("5 Hz  ");
    }

}


/// @brief Retorna os valores lidos do MPU6050
/// @return Tipo MPU_read com os valores lidos
MPU_read MPU6050::getValues() {
    sensors_event_t a, g, temp;
    MPU_read read;

    if (DEBUGCOMM) {
        read.x = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        read.y = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        read.z = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        read.gx = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        read.gy = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        read.gz = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        read.temp = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    } else {
        mpu.getEvent(&a, &g, &temp);
        read.x = a.acceleration.x;
        read.y = a.acceleration.y;
        read.z = a.acceleration.z;
        read.gx = g.gyro.x;
        read.gy = g.gyro.y;
        read.gz = g.gyro.z;
        read.temp = temp.temperature;
    }

    if (DEBUGSENS) {
        Serial.print("[MPU6050 id ");
        Serial.print(this->id);
        Serial.println("] leitura lida:");
        
        Serial.print("  Accel > X: ");
        Serial.print(read.x);
        Serial.print("\tY: ");
        Serial.print(read.y);
        Serial.print("\tZ: ");
        Serial.print(read.z);

        Serial.print("\tGyro > X: ");
        Serial.print(read.gx);
        Serial.print("\tY: ");
        Serial.print(read.gy);
        Serial.print("\tZ: ");
        Serial.print(read.gz);

        Serial.print("\tTemp > ");
        Serial.println(read.temp);
    }

    return read;
}