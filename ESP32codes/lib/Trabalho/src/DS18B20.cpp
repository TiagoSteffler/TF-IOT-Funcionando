class DS18B20{
private:
    int dataPin;

    public:
    DS18B20(int pin);
    ~DS18B20();

    float readTemperature();
};

DS18B20::DS18B20(int pin) {
    
}

DS18B20::~DS18B20() {}

float DS18B20::readTemperature() { 
    return 0.0f;
}
