#include "board.hpp"
#include <nlohmann/json.hpp>

// RPI GPIO FUNCTIONS
void pullGPIO(unsigned pin, unsigned high)
{
    GPIO_PULL = high << pin;
}

void initGPIOInput(unsigned pin)
{
    INP_GPIO(pin);
}

void initGPIOOutput(unsigned pin)
{
    INP_GPIO(pin);
    OUT_GPIO(pin);
    pullGPIO(pin, 0);
}

void init(int sensorType)
{
    initGPIOInput(DATA0);
    initGPIOInput(DATA1);
    initGPIOInput(DATA2);
    initGPIOInput(DATA3);
    initGPIOInput(DATA4);
    initGPIOInput(DATA5);
    initGPIOInput(DATA6);
    initGPIOInput(DATA7);

    initGPIOInput(TCO);
    initGPIOInput(PI_OK);
    initGPIOInput(FAIL);
    initGPIOInput(BUTTON);

    // initGPIOOutput(PI_ON);
    initGPIOOutput(CLOCK);
    initGPIOOutput(RESET);

    // Initial Reset
    setGPIOLow(CLOCK);
    setGPIOHigh(RESET);

    // SPI Communication
    // // Select Sensor type
    BoardInterface::get()->setBridge(sensorType);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // // Start Measurement
    BoardInterface::get()->setEnableADmes(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    BoardInterface::get()->setEnableADmes(1);
}

void shutdown()
{
    // Reset Clock
    setGPIOLow(CLOCK);
    // Stop Measurement
    BoardInterface::get()->setEnableADmes(0);
}

void setGPIOHigh(unsigned pin)
{
    GPIO_SET = 1 << pin;
}

void setGPIOLow(unsigned pin)
{
    GPIO_CLR = 1 << pin;
}

static const uint32_t ALL_32_BITS_ON = 0xFFFFFFFF; // (2^32)-1 - ALL BCM_PINS
void resetAllGPIO()
{
    GPIO_CLR = ALL_32_BITS_ON;
}

void sleep55ns()
{
    readAllGPIO();
}

void sleep8ns()
{
    setGPIOHigh(10); // ANY UNUSED PIN!!!
}

unsigned int readAllGPIO()
{
    return (*(gpio + 13) & ALL_32_BITS_ON); 
}

static std::mutex boardMtx;

BoardEvents readBoardEvents()
{
    std::lock_guard<std::mutex> lock(boardMtx);
    BoardEvents ret;
    std::string data;
    if (BoardInterface::get()->getEvents(data) && !data.empty()) {
        if (data[data.length()-1] == 0xa ) data = data.substr(0, data.size()-1);

        if (data.empty()) return ret;

        try {
            auto j = nlohmann::json::parse(data);
            auto it_btn = j.find("Button");
            if (it_btn != j.end() && it_btn->is_boolean() && it_btn->get<bool>()) {
                auto it_cnt = j.find("ButtonStateCnt");
                if (it_cnt != j.end() && it_cnt->is_number()) {
                    ret.button = true;
                    ret.buttonCounter = it_cnt->get<unsigned>();
                }
            }
        }
        catch (nlohmann::json::parse_error& e)
        {
            // output exception information
            std::cerr << "readBoardEvents: json parse failed data:" << data << "error:" << e.what() << '\n';
        }
    }
    return ret;
}

std::string readBoardGetSettings(const std::string& request, std::string& error) {
    std::lock_guard<std::mutex> lock(boardMtx);
    return BoardInterface::get()->getGetSettings(request, error);
}

std::string readBoardSetSettings(const std::string& request, std::string& error) {
    std::lock_guard<std::mutex> lock(boardMtx);
    return BoardInterface::get()->getSetSettings(request, error);
}

bool BoardStartPWM(uint8_t num, uint32_t frequency, uint32_t high, uint32_t low, uint32_t repeats, float duty_cycle) {
    std::lock_guard<std::mutex> lock(boardMtx);
    return BoardInterface::get()->startPWM(num, frequency, high, low, repeats, duty_cycle);
}

bool BoardStopPWM(uint8_t num) {
    std::lock_guard<std::mutex> lock(boardMtx);
    return BoardInterface::get()->stopPWM(num);
}

bool BoardGetPWM(uint8_t num, bool& active, uint32_t& frequency, uint32_t& high, uint32_t& low, uint32_t& repeats, float& duty_cycle) {
    std::lock_guard<std::mutex> lock(boardMtx);
    return BoardInterface::get()->getPWM(num, active, frequency, high, low, repeats, duty_cycle);
}

void BoardTraceSPI(bool val) {
    BoardInterface::trace_spi = val;
}
