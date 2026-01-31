#pragma once
#include <libfreenect_sync.h>
#include <cstdint>
#include <algorithm>
#include <termios.h>
#include <csignal>
#include <cstdio>
#include <unistd.h>
#include <string>
#include "pca9685.hpp"
#include "vl53l0x.hpp"

// Dimensions de la matrice de points pour le scénario MATRIX
#define stX 12
#define stY 12

//

const std::string runnable_scenario[] = {
    "calibrage",
    "matrix",
    "vl53l0x",
    "pca9685",
    "tiltkinect",
    "kinect_sync",
    "kinect_async",
    "mixed",
    ""};

enum ScenarioType
{
    SCENARIO_CALIBRAGE,
    SCENARIO_MATRIX,
    SCENARIO_VL53L0X,
    SCENARIO_PCA9685,
    SCENARIO_TILTKINECT,
    SCENARIO_KINECT_SYNC,
    SCENARIO_KINECT_ASYNC,
    SCENARIO_MIXED,
    SCENARIO_UNKNOWN = -1,
};

class Test
{
private:
    ScenarioType scenario;
    PCA9685 *pca9685;
    int scenario_calibrage();
    int scenario_matrix();
    int scenario_vl53l0x();
    int scenario_pca9685(uint8_t a = 0x40);
    int scenario_tiltkinect();
    int scenario_kinect_async();
    int scenario_kinect_sync();
    int scenario_mixed();

public:
    static volatile sig_atomic_t should_exit;
    Test(int argc, char **argv);
    ~Test();
    int run();
};
